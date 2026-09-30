// Read-only Sapling note listing and optional GUI regression checks.
#include "viewnotesdialog.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "chainparams.h"
#include "main.h"
#include "util.h"
#include "utiltime.h"
#include "wallet/wallet.h"
#include "zcash/NoteEncryption.hpp"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>

#include <sodium.h>
#include <cstdio>
#include <memory>
#include <stdexcept>

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

class NotesWallet : public CWallet
{
public:
    using CCryptoKeyStore::EncryptKeys;
};

static void checkNotes(bool gui, const QString& screenshot)
{
    NotesWallet wallet;
    OptionsModel options;
    WalletModel model(nullptr, &wallet, &options);
    for (auto* timer : model.findChildren<QTimer*>()) timer->stop();
    require(model.getSaplingNotes().empty(), "empty wallet returned notes");

    auto key = libzcash::SaplingExtendedSpendingKey::Master(HDSeed::Random(32));
    auto watchKey = libzcash::SaplingExtendedSpendingKey::Master(HDSeed::Random(32));
    const auto address = key.DefaultAddress();
    const auto watchAddress = watchKey.DefaultAddress();
    require(wallet.AddSaplingSpendingKey(key, address), "could not add spending key");
    require(wallet.AddSaplingFullViewingKey(watchKey.expsk.full_viewing_key(), watchAddress),
            "could not add viewing key");
    wallet.mapZAddressBook[address].name = "Savings";
    wallet.mapZAddressBook[watchAddress].name = "Watch only";

    CMutableTransaction tx;
    tx.nVersion = SAPLING_TX_VERSION;
    tx.fOverwintered = true;
    tx.nVersionGroupId = SAPLING_VERSION_GROUP_ID;
    for (int i = 0; i < 3; ++i) {
        const auto& recipient = i == 2 ? watchAddress : address;
        libzcash::SaplingNote note(recipient, (i + 1) * COIN);
        std::array<unsigned char, ZC_MEMO_SIZE> memo = {};
        const std::string message = "Note " + std::to_string(i + 1);
        std::copy(message.begin(), message.end(), memo.begin());
        auto encrypted = libzcash::SaplingNotePlaintext(note, memo).encrypt(recipient.pk_d);
        require(bool(encrypted), "note encryption failed");
        OutputDescription output;
        output.cm = note.cm().get();
        output.encCiphertext = encrypted->first;
        output.ephemeralKey = encrypted->second.get_epk();
        tx.vShieldedOutput.push_back(output);
    }
    CWalletTx wtx(&wallet, CTransaction(tx));
    const uint256 hash = wtx.GetHash();
    const uint256 nullifier = uint256S("01");
    wtx.mapSaplingNoteData.emplace(SaplingOutPoint(hash, 0),
                                 SaplingNoteData(key.expsk.full_viewing_key().in_viewing_key(), nullifier));
    wtx.mapSaplingNoteData.emplace(SaplingOutPoint(hash, 1),
                                 SaplingNoteData(key.expsk.full_viewing_key().in_viewing_key(), uint256S("02")));
    wtx.mapSaplingNoteData.emplace(SaplingOutPoint(hash, 2),
                                 SaplingNoteData(watchKey.expsk.full_viewing_key().in_viewing_key()));

    // A synthetic block proves inclusion; no proof generation, disk or network.
    CBlock block;
    block.vtx.push_back(wtx);
    block.hashMerkleRoot = block.BuildMerkleTree();
    CBlockIndex index(block);
    const auto blockHash = block.GetHash();
    mapBlockIndex.emplace(blockHash, &index);
    chainActive.SetTip(&index);
    struct ChainCleanup {
        uint256 hash;
        ~ChainCleanup() { chainActive.SetTip(nullptr); mapBlockIndex.erase(hash); mempool.clear(); }
    } cleanup{blockHash};
    wtx.SetMerkleBranch(block);
    wallet.mapWallet.emplace(hash, wtx);
    wallet.LockNote(SaplingOutPoint(hash, 1));

    auto entries = model.getSaplingNotes();
    require(entries.size() == 3, "confirmed, locked and watch-only notes should be listed");
    require(entries[0].amount == COIN && entries[0].label == "Savings" && entries[0].rawConfirmations == 1 &&
            entries[0].txid == QString::fromStdString(hash.ToString()) && entries[0].outputIndex == 0 &&
            entries[0].memo.startsWith("Note 1") && entries[0].hasSpendingKey && entries[0].spentStatusKnown,
            "note fields do not match wallet data");
    require(entries[1].locked && entries[1].amount == 2 * COIN, "locked note state lost");
    require(!entries[2].hasSpendingKey && !entries[2].spentStatusKnown, "watch-only state lost");

    CKeyingMaterial masterKey(32, 42);
    require(wallet.EncryptKeys(masterKey) && wallet.IsLocked(), "could not encrypt test wallet");
    require(model.getSaplingNotes().size() == 3 && wallet.IsLocked(), "viewing notes requires unlock");

    if (gui) {
        ViewNotesDialog dialog(&model);
        dialog.show();
        QApplication::processEvents();
        auto* tree = dialog.findChild<QTreeWidget*>("notes");
        auto* search = dialog.findChild<QLineEdit*>("noteSearch");
        auto* details = dialog.findChild<QPlainTextEdit*>("noteDetails");
        require(tree && search && details && tree->topLevelItemCount() == 3, "dialog did not display notes");
        require(tree->topLevelItem(0)->data(0, Qt::UserRole).toLongLong() == 3 * COIN, "amount sorting failed");
        tree->setCurrentItem(tree->topLevelItem(0));
        require(details->toPlainText().contains("Note 3"), "memo missing from details");
        search->setText("Savings");
        require(tree->topLevelItem(0)->isHidden() && !tree->topLevelItem(1)->isHidden(), "label filter failed");
        require(dialog.findChild<QLabel*>("noteSummary")->text().contains("2 of 3"), "filtered total count failed");
        require(details->toPlainText().isEmpty(), "filtered-out note retained details");
        search->clear();
        tree->setCurrentItem(tree->topLevelItem(1));
        QApplication::processEvents();
        if (!screenshot.isEmpty()) require(dialog.grab().save(screenshot), "could not save screenshot");
        dialog.findChild<QPushButton*>("refreshNotes")->click();
        require(tree->topLevelItemCount() == 3 && wallet.IsLocked(), "refresh changed wallet lock state");
    }

    // A mempool spend removes the spent note even before it is mined.
    CMutableTransaction spend;
    spend.nVersion = SAPLING_TX_VERSION;
    spend.fOverwintered = true;
    spend.nVersionGroupId = SAPLING_VERSION_GROUP_ID;
    SpendDescription input;
    input.nullifier = nullifier;
    spend.vShieldedSpend.push_back(input);
    CWalletTx spending(&wallet, CTransaction(spend));
    mempool.addUnchecked(spending.GetHash(), CTxMemPoolEntry(spending, 0, GetTime(), 0, 0, true, false, 0));
    wallet.AddToWallet(spending, true, nullptr);
    require(model.getSaplingNotes().size() == 2, "spent note remained in the listing");

    // The receive transaction is now unconfirmed, then absent from the mempool.
    chainActive.SetTip(nullptr);
    mempool.addUnchecked(hash, CTxMemPoolEntry(wtx, 0, GetTime(), 0, 0, true, false, 0));
    entries = model.getSaplingNotes();
    require(entries.size() == 2 && entries[0].rawConfirmations == 0, "unconfirmed notes missing");
    mempool.clear();
    require(model.getSaplingNotes().empty(), "conflicted/non-mempool notes should be excluded");
    std::puts("View Notes: fields, locked/watch-only/encrypted notes, spent and confirmation filtering passed");
}

int main(int argc, char** argv)
{
    bool gui = false;
    for (int i = 1; i < argc; ++i) if (std::string(argv[i]) == "--gui") gui = true;
    std::unique_ptr<QCoreApplication> app(gui ? new QApplication(argc, argv) : new QCoreApplication(argc, argv));
    QTemporaryDir settings;
    app->setOrganizationName("KmdClassicTests");
    app->setApplicationName("ViewNotes");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
    fPrintToDebugLog = false;
    fPrintToConsole = true;
    try {
        require(settings.isValid() && sodium_init() >= 0, "test initialization failed");
        SelectParams(CBaseChainParams::REGTEST);
        SetMockTime(1800000000);
        const int screenshotArg = app->arguments().indexOf("--screenshot");
        const QString screenshot = screenshotArg >= 0 ? app->arguments().value(screenshotArg + 1) : QString();
        checkNotes(gui, screenshot);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "View Notes regression: %s\n", e.what());
        return 1;
    }
    return 0;
}

// Regression coverage for address indexes and shielded key creation.
#include "addresstablemodel.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "zaddresstablemodel.h"
#include "chainparams.h"
#include "key_io.h"
#include "util.h"
#include "utiltime.h"
#include "wallet/wallet.h"
#include "ui_interface.h"

#include <QCoreApplication>
#include <QItemSelectionModel>
#include <QPersistentModelIndex>
#include <QSortFilterProxyModel>
#include <QSettings>
#include <QTemporaryDir>

#include <sodium.h>

#include <cstdio>
#include <stdexcept>

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

template <typename Model>
static void checkAddressIndexes(const char* name)
{
    // An in-memory wallet: no keys, files, network or real wallet are needed.
    CWallet wallet;
    Model model(nullptr, &wallet);
    model.updateEntry("middle", "original label", true, "receive", CT_NEW);
    QPersistentModelIndex address(model.index(0, Model::Address, QModelIndex()));
    QPersistentModelIndex label(model.index(0, Model::Label, QModelIndex()));

    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.setFilterRole(Model::TypeRole);
    proxy.setFilterFixedString(Model::Receive);
    proxy.setSortRole(Qt::EditRole);
    proxy.sort(Model::Label);
    QItemSelectionModel selection(&proxy);
    selection.select(proxy.mapFromSource(address), QItemSelectionModel::Select);

    auto check = [&] {
        require(address.isValid() && label.isValid(), "retained index became invalid");
        // Check identity before dereferencing: old Qt 6 models leave dangling
        // QList element pointers in indexes whose row did not move.
        require(address == model.index(address.row(), Model::Address, QModelIndex()),
                "retained index no longer identifies the same address row");
        require(address.data(Qt::EditRole).toString() == "middle", "address changed");
        require(label.data(Qt::EditRole).toString() == "original label", "label changed");
        const auto selected = selection.selectedIndexes();
        require(selected.size() == 1 && selected.front().data(Qt::EditRole).toString() == "middle",
                "proxy selection no longer identifies the original address");
    };

    // Append enough rows to force reallocation, then insert before the selection.
    for (int i = 0; i < 128; ++i) {
        model.updateEntry(QString("z%1").arg(i, 4, 10, QChar('0')), "later", true, "receive", CT_NEW);
        check();
    }
    model.updateEntry("aaa", "earlier", true, "receive", CT_NEW);
    check();
    model.updateEntry("aaa", "", true, "receive", CT_DELETED);
    check();
    model.updateEntry("z0000", "", true, "receive", CT_DELETED);
    check();
    model.updateEntry("middle", "updated label", true, "receive", CT_UPDATED);
    require(label.data(Qt::EditRole).toString() == "updated label", "label update was lost");
    model.updateEntry("middle", "", true, "receive", CT_DELETED);
    require(!address.isValid() && !label.isValid(), "deleted row retained a valid index");
    std::printf("%s: retained indexes and proxy selection passed\n", name);
}

class TestWallet : public CWallet
{
public:
    using CCryptoKeyStore::EncryptKeys;
    bool unlockForTest(const CKeyingMaterial& key) { return CCryptoKeyStore::Unlock(key); }
    bool failSeedRead = false;
    bool GetHDSeed(HDSeed& seed) const override
    {
        return !failSeedRead && CCryptoKeyStore::GetHDSeed(seed);
    }
};

static void checkZAddressCreation()
{
    {
        CWallet wallet;
        ZAddressTableModel model(nullptr, &wallet);
        require(model.addRow(ZAddressTableModel::Receive, "", "").isEmpty(),
                "missing HD seed should fail address creation");
        require(model.getEditStatus() == ZAddressTableModel::KEY_GENERATION_FAILURE,
                "missing HD seed should report a key generation error");
        require(model.getEditError().contains("HD seed not found"), "failure reason was lost");
        require(!wallet.HaveHDSeed() && wallet.mapZAddressBook.empty(),
                "failure must not replace the seed or add an address");
        wallet.GenerateNewSeed();
        require(!model.addRow(ZAddressTableModel::Receive, "", "").isEmpty(),
                "unencrypted wallet failed to create an address");
        require(model.getEditStatus() == ZAddressTableModel::OK && model.getEditError().isEmpty(),
                "successful retry retained an earlier error");
    }

    TestWallet wallet;
    wallet.GenerateNewSeed();
    // Test-only key; all generated wallet data stays in memory.
    CKeyingMaterial key(32, 42);
    require(wallet.EncryptKeys(key) && wallet.IsLocked(), "failed to set up encrypted wallet");

    OptionsModel options;
    WalletModel walletModel(nullptr, &wallet, &options);
    ZAddressTableModel* model = walletModel.getZAddressTableModel();
    int unlockRequests = 0;
    bool allowUnlock = false;
    QObject::connect(&walletModel, &WalletModel::requireUnlock, [&] {
        ++unlockRequests;
        if (allowUnlock) require(wallet.unlockForTest(key), "test unlock failed");
    });

    require(model->addRow(ZAddressTableModel::Receive, "", "").isEmpty(),
            "cancelled unlock should fail address creation");
    require(unlockRequests == 1 && model->getEditStatus() == ZAddressTableModel::WALLET_UNLOCK_FAILURE,
            "cancelled unlock was not reported");
    require(wallet.IsLocked() && wallet.mapZAddressBook.empty(), "cancelled unlock changed wallet");

    allowUnlock = true;
    const QString first = model->addRow(ZAddressTableModel::Receive, "", "");
    require(!first.isEmpty() && model->getEditStatus() == ZAddressTableModel::OK,
            "encrypted wallet failed to create an address after unlock");
    require(unlockRequests == 2 && wallet.IsLocked(), "wallet was not relocked after key creation");
    const auto decoded = DecodePaymentAddress(first.toStdString());
    require(boost::get<libzcash::SaplingPaymentAddress>(&decoded) != nullptr &&
            wallet.mapZAddressBook.count(decoded) == 1, "Sapling address was not added to address book");

    require(wallet.unlockForTest(key), "test unlock failed");
    require(bool(boost::apply_visitor(GetSpendingKeyForPaymentAddress(&wallet), decoded)),
            "created address has no decryptable spending key");
    const QString second = model->addRow(ZAddressTableModel::Receive, "", "");
    require(!second.isEmpty() && second != first, "unlocked wallet failed to create a distinct address");
    require(unlockRequests == 2 && !wallet.IsLocked(), "already unlocked wallet changed lock state");

    require(wallet.Lock(), "test lock failed");
    wallet.failSeedRead = true;
    require(model->addRow(ZAddressTableModel::Receive, "", "").isEmpty() &&
            model->getEditStatus() == ZAddressTableModel::KEY_GENERATION_FAILURE,
            "key generation exception escaped or reported success");
    require(unlockRequests == 3 && wallet.IsLocked() && wallet.mapZAddressBook.size() == 2,
            "failed generation must relock the wallet without adding an address");
    std::puts("Z-address creation: missing seed, retry, cancelled unlock, unlock and relock on success/failure passed");
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir settingsDir;
    QCoreApplication::setOrganizationName("KmdClassicTests");
    QCoreApplication::setApplicationName("AddressModels");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());
    fPrintToDebugLog = false;
    fPrintToConsole = true;
    try {
        require(settingsDir.isValid(), "failed to create temporary settings directory");
        require(sodium_init() >= 0, "sodium initialization failed");
        SelectParams(CBaseChainParams::REGTEST);
        SetMockTime(1800000000); // Exercise Sapling independently of the wall clock.
        checkAddressIndexes<ZAddressTableModel>("Z-address model");
        checkAddressIndexes<AddressTableModel>("Address model");
        checkZAddressCreation();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Address model regression: %s\n", e.what());
        return 1;
    }
    return 0;
}

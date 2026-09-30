// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "viewnotesdialog.h"

#include "guiutil.h"
#include "komodounits.h"
#include "optionsmodel.h"
#include "walletmodel.h"

#include <QAction>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <exception>

namespace {
class NoteItem : public QTreeWidgetItem
{
public:
    bool operator<(const QTreeWidgetItem& other) const override
    {
        const int column = treeWidget()->sortColumn();
        if (data(column, Qt::UserRole).isValid())
            return data(column, Qt::UserRole).toLongLong() < other.data(column, Qt::UserRole).toLongLong();
        return QTreeWidgetItem::operator<(other);
    }
};

QString memoText(QByteArray memo)
{
    // Strip padding for text only; keep all original bytes for copying as hex.
    while (memo.endsWith('\0')) memo.chop(1);
    if (memo.isEmpty() || memo == QByteArray(1, char(0xf6))) return ViewNotesDialog::tr("No memo");
    const QString text = QString::fromUtf8(memo);
    if (static_cast<unsigned char>(memo.at(0)) < 0xf5 && text.toUtf8() == memo) return text;
    return ViewNotesDialog::tr("Binary memo (hex):") + "\n" + QString::fromLatin1(memo.toHex());
}
}

ViewNotesDialog::ViewNotesDialog(WalletModel* walletModel, QWidget* parent) :
    QDialog(parent), model(walletModel)
{
    setWindowTitle(tr("View Notes"));
    resize(1100, 620);
    auto* layout = new QVBoxLayout(this);
    auto* description = new QLabel(tr("Sapling notes in this wallet. Known spent notes are excluded."), this);
    description->setWordWrap(true);
    layout->addWidget(description);
    search = new QLineEdit(this);
    search->setObjectName("noteSearch");
    search->setPlaceholderText(tr("Search by address, label or transaction ID"));
    search->setClearButtonEnabled(true);
    layout->addWidget(search);

    notes = new QTreeWidget(this);
    notes->setObjectName("notes");
    notes->setHeaderLabels({tr("Amount"), tr("Label"), tr("Address"), tr("Confirmations"),
                           tr("Status"), tr("Change"), tr("Transaction ID"), tr("Output index")});
    notes->setRootIsDecorated(false);
    notes->setAlternatingRowColors(true);
    notes->setSelectionMode(QAbstractItemView::SingleSelection);
    notes->setSelectionBehavior(QAbstractItemView::SelectRows);
    notes->setEditTriggers(QAbstractItemView::NoEditTriggers);
    notes->setContextMenuPolicy(Qt::CustomContextMenu);
    notes->header()->setStretchLastSection(false);
    const int widths[] = {145, 120, 240, 110, 200, 75, 200, 90};
    for (int i = 0; i <= OutputIndex; ++i) notes->setColumnWidth(i, widths[i]);
    notes->headerItem()->setToolTip(Confirmations, tr("Confirmations adjusted for notarization, as in z_listunspent. Hover over a value to see block confirmations."));
    notes->headerItem()->setToolTip(Change, tr("The receiving address also spent notes in this transaction, as in z_listunspent."));
    notes->setSortingEnabled(true);
    notes->sortItems(Amount, Qt::DescendingOrder);
    layout->addWidget(notes, 1);

    summary = new QLabel(this);
    summary->setObjectName("noteSummary");
    layout->addWidget(summary);
    notice = new QLabel(this);
    notice->setTextFormat(Qt::PlainText);
    notice->setWordWrap(true);
    layout->addWidget(notice);
    details = new QPlainTextEdit(this);
    details->setObjectName("noteDetails");
    details->setReadOnly(true);
    details->setPlaceholderText(tr("Select a note to view its address, transaction and memo."));
    details->setMaximumHeight(145);
    layout->addWidget(details);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    auto* refreshButton = buttons->addButton(tr("Refresh"), QDialogButtonBox::ActionRole);
    refreshButton->setObjectName("refreshNotes");
    refreshButton->setAutoDefault(false);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(refreshButton, &QPushButton::clicked, this, &ViewNotesDialog::refresh);
    connect(search, &QLineEdit::textChanged, this, &ViewNotesDialog::filterNotes);
    connect(notes, &QTreeWidget::itemSelectionChanged, this, &ViewNotesDialog::updateDetails);
    connect(notes, &QTreeWidget::customContextMenuRequested, this, &ViewNotesDialog::showContextMenu);
    connect(model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &ViewNotesDialog::refresh);
    refresh();
}

void ViewNotesDialog::refresh()
{
    notes->clear();
    details->clear();
    notice->clear();
    const int unit = model->getOptionsModel()->getDisplayUnit();
    notes->headerItem()->setText(Amount, KomodoUnits::getAmountColumnTitle(unit));
    try {
        const auto entries = model->getSaplingNotes();
        bool unknownSpentStatus = false;
        notes->setSortingEnabled(false);
        for (const auto& entry : entries) {
            auto* item = new NoteItem;
            item->setText(Amount, KomodoUnits::format(unit, entry.amount));
            item->setData(Amount, Qt::UserRole, qlonglong(entry.amount));
            item->setTextAlignment(Amount, Qt::AlignRight | Qt::AlignVCenter);
            item->setText(Label, entry.label);
            item->setText(Address, entry.address);
            item->setText(Confirmations, QString::number(entry.confirmations));
            item->setData(Confirmations, Qt::UserRole, entry.confirmations);
            item->setToolTip(Confirmations, tr("Block confirmations: %1\nNotarization-adjusted confirmations: %2")
                            .arg(entry.rawConfirmations).arg(entry.confirmations));
            QStringList status;
            if (entry.rawConfirmations == 0) status << tr("Unconfirmed");
            if (!entry.hasSpendingKey) status << tr("Watch-only");
            if (entry.locked) status << tr("Locked");
            if (!entry.spentStatusKnown) {
                status << tr("Spent status unknown");
                unknownSpentStatus = true;
            }
            if (status.isEmpty()) status << tr("Unspent");
            item->setText(Status, status.join(", "));
            item->setText(Change, entry.hasSpendingKey ? (entry.change ? tr("Yes") : tr("No")) : tr("Unknown"));
            item->setText(TxId, entry.txid);
            item->setText(OutputIndex, QString::number(entry.outputIndex));
            item->setData(OutputIndex, Qt::UserRole, entry.outputIndex);
            item->setData(Address, Qt::UserRole + 1, entry.memo);
            for (int column : {Amount, Label, Address, Status, TxId}) item->setToolTip(column, item->text(column));
            notes->addTopLevelItem(item);
        }
        if (unknownSpentStatus)
            notice->setText(tr("Some notes have an unknown spent status and may already be spent. Their amounts are included in the listed total."));
    } catch (const std::exception& e) {
        notes->clear();
        notice->setText(tr("Could not load Sapling notes: %1").arg(QString::fromUtf8(e.what())));
    } catch (...) {
        notes->clear();
        notice->setText(tr("Could not load Sapling notes."));
    }
    notes->setSortingEnabled(true);
    notice->setVisible(!notice->text().isEmpty());
    filterNotes();
}

void ViewNotesDialog::filterNotes()
{
    const QString query = search->text().trimmed();
    int count = 0;
    CAmount total = 0;
    for (int i = 0; i < notes->topLevelItemCount(); ++i) {
        auto* item = notes->topLevelItem(i);
        const bool match = item->text(Address).contains(query, Qt::CaseInsensitive) ||
                           item->text(Label).contains(query, Qt::CaseInsensitive) ||
                           item->text(TxId).contains(query, Qt::CaseInsensitive);
        item->setHidden(!match);
        if (match) {
            ++count;
            total += item->data(Amount, Qt::UserRole).toLongLong();
        }
    }
    summary->setText(tr("Notes: %1 of %2 | Listed amount: %3").arg(count).arg(notes->topLevelItemCount())
                     .arg(KomodoUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), total)));
    if (notes->topLevelItemCount() == 0 && notice->text().isEmpty())
        summary->setText(tr("No unspent Sapling notes found."));
    updateDetails();
}

void ViewNotesDialog::updateDetails()
{
    auto* item = notes->currentItem();
    if (!item || item->isHidden()) {
        details->clear();
        return;
    }
    details->setPlainText(tr("Address: %1\nTransaction: %2\nOutput index: %3\nMemo:\n%4")
                         .arg(item->text(Address), item->text(TxId), item->text(OutputIndex),
                              memoText(item->data(Address, Qt::UserRole + 1).toByteArray())));
}

void ViewNotesDialog::showContextMenu(const QPoint& position)
{
    auto* item = notes->itemAt(position);
    if (!item) return;
    notes->setCurrentItem(item);
    QMenu menu(this);
    auto addCopy = [&](const QString& title, const QString& text) {
        connect(menu.addAction(title), &QAction::triggered, this, [text] { GUIUtil::setClipboard(text); });
    };
    addCopy(tr("Copy address"), item->text(Address));
    addCopy(tr("Copy transaction ID"), item->text(TxId));
    addCopy(tr("Copy note ID"), item->text(TxId) + ":" + item->text(OutputIndex));
    addCopy(tr("Copy amount"), item->text(Amount));
    addCopy(tr("Copy memo (hex)"), QString::fromLatin1(item->data(Address, Qt::UserRole + 1).toByteArray().toHex()));
    menu.exec(notes->viewport()->mapToGlobal(position));
}

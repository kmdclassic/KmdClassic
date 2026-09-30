// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "donatedialog.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

DonateDialog::DonateDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Donate"));
    resize(920, 440);
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    auto* heading = new QLabel(tr("Support KMD Classic"), this);
    QFont headingFont = heading->font();
    headingFont.setBold(true);
    heading->setFont(headingFont);
    layout->addWidget(heading);

    auto* explanation = new QLabel(
        tr("KMD Classic needs community support to maintain its infrastructure — full nodes, the block explorer and Electrum servers — and to fund software development.") +
        "\n\n" + tr("Unfortunately, without community support, the project will be unable to continue and will have to shut down."), this);
    explanation->setTextFormat(Qt::PlainText);
    explanation->setWordWrap(true);
    layout->addWidget(explanation);

    struct DonationAddress { const char* coin; const char* address; };
    static const DonationAddress donationAddresses[] = {
        {"KMDCL", "zs195kaay3f4wavekfgemaxze2sf02qm2daztkgg9duh4fqlaakxagn4t6lasats3ch87f3gvpxq8y"},
        {"ZEC", "u1fvnrmpzhl0wj5asgxffk6a9lgz6rdqdym94f4da5k0jnnp70mep63n3uz8hc7tp0gpvfc7a86lefnjfejdpw9r64rqg50sdgayfx4aht"},
        {"XMR", "87FGNYDRQK8iqpJW7fnNsTVnrt7gtYGwwGc7JQ9PiRxLCvjxQmF34855Py9xZ9ysC5ZSBLDaLUN77i8ngwRuNR8CLnCKGKf"}
    };
    addresses = new QTableWidget(3, 2, this);
    addresses->setObjectName("donationAddresses");
    addresses->setHorizontalHeaderLabels({tr("Coin"), tr("Donation address")});
    addresses->verticalHeader()->hide();
    addresses->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    addresses->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    addresses->setSelectionBehavior(QAbstractItemView::SelectRows);
    addresses->setSelectionMode(QAbstractItemView::SingleSelection);
    addresses->setEditTriggers(QAbstractItemView::NoEditTriggers);
    addresses->setAlternatingRowColors(true);
    addresses->setWordWrap(true);
    addresses->setTextElideMode(Qt::ElideNone);
    addresses->setContextMenuPolicy(Qt::CustomContextMenu);
    addresses->setMinimumHeight(150);
    for (int row = 0; row < 3; ++row) {
        addresses->setItem(row, 0, new QTableWidgetItem(QString::fromLatin1(donationAddresses[row].coin)));
        auto* address = new QTableWidgetItem(QString::fromLatin1(donationAddresses[row].address));
        address->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        address->setToolTip(address->text());
        addresses->setItem(row, 1, address);
    }
    connect(addresses->horizontalHeader(), &QHeaderView::sectionResized,
            addresses, &QTableWidget::resizeRowsToContents);
    connect(addresses, &QTableWidget::customContextMenuRequested, this, &DonateDialog::showContextMenu);
    addresses->resizeRowsToContents();
    addresses->selectRow(0);
    layout->addWidget(addresses, 1);

    auto* thanks = new QLabel(tr("Thank you for supporting KMD Classic. Every contribution helps keep the services running and makes continued development possible. Your support is essential to the project's future."), this);
    thanks->setTextFormat(Qt::PlainText);
    thanks->setWordWrap(true);
    QFont thanksFont = thanks->font();
    if (thanksFont.pointSizeF() > 0) thanksFont.setPointSizeF(qMax(8.0, thanksFont.pointSizeF() - 1.0));
    thanks->setFont(thanksFont);
    layout->addWidget(thanks);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    auto* copy = buttons->addButton(tr("Copy address"), QDialogButtonBox::ActionRole);
    copy->setObjectName("copyDonationAddress");
    copy->setAutoDefault(false);
    connect(copy, &QPushButton::clicked, this, &DonateDialog::copyAddress);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    auto* copyShortcut = new QAction(tr("Copy address"), this);
    copyShortcut->setShortcut(QKeySequence::Copy);
    copyShortcut->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(copyShortcut);
    connect(copyShortcut, &QAction::triggered, this, &DonateDialog::copyAddress);
}

void DonateDialog::copyAddress()
{
    const auto* address = addresses->item(addresses->currentRow(), 1);
    if (address) QApplication::clipboard()->setText(address->text());
}

void DonateDialog::showContextMenu(const QPoint& position)
{
    const auto* item = addresses->itemAt(position);
    if (!item) return;
    addresses->selectRow(item->row());
    QMenu menu(this);
    connect(menu.addAction(tr("Copy address")), &QAction::triggered, this, &DonateDialog::copyAddress);
    menu.exec(addresses->viewport()->mapToGlobal(position));
}

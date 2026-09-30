// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KOMODO_QT_VIEWNOTESDIALOG_H
#define KOMODO_QT_VIEWNOTESDIALOG_H

#include <QDialog>

class WalletModel;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QTreeWidget;

/** Read-only view of the wallet's unspent Sapling notes. */
class ViewNotesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ViewNotesDialog(WalletModel* model, QWidget* parent = nullptr);

private Q_SLOTS:
    void refresh();
    void filterNotes();
    void updateDetails();
    void showContextMenu(const QPoint& position);

private:
    enum Column { Amount, Label, Address, Confirmations, Status, Change, TxId, OutputIndex };
    WalletModel* model;
    QLineEdit* search;
    QTreeWidget* notes;
    QLabel* summary;
    QLabel* notice;
    QPlainTextEdit* details;
};

#endif // KOMODO_QT_VIEWNOTESDIALOG_H

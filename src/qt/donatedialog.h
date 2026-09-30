// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KOMODO_QT_DONATEDIALOG_H
#define KOMODO_QT_DONATEDIALOG_H

#include <QDialog>

class QTableWidget;

class DonateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DonateDialog(QWidget* parent = nullptr);

private Q_SLOTS:
    void copyAddress();
    void showContextMenu(const QPoint& position);

private:
    QTableWidget* addresses;
};

#endif // KOMODO_QT_DONATEDIALOG_H

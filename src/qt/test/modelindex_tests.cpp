// Regression coverage for indexes retained across address-list mutations.
#include "addresstablemodel.h"
#include "zaddresstablemodel.h"
#include "wallet/wallet.h"
#include "ui_interface.h"

#include <QCoreApplication>
#include <QItemSelectionModel>
#include <QPersistentModelIndex>
#include <QSortFilterProxyModel>

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

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    try {
        checkAddressIndexes<ZAddressTableModel>("Z-address model");
        checkAddressIndexes<AddressTableModel>("Address model");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Model index regression: %s\n", e.what());
        return 1;
    }
    return 0;
}

#ifndef BITCOIN_QT_EVENTSLIST_H
#define BITCOIN_QT_EVENTSLIST_H

#include <primitives/transaction.h>
#include <sync.h>
#include <util.h>

#include <evo/deterministicmns.h>

#include <QMenu>
#include <QTimer>
#include <QWidget>

#define EVENTSLIST_UPDATE_SECONDS 3
#define EVENTSLIST_FILTER_COOLDOWN_SECONDS 3

namespace Ui
{
class EventsList;
}

class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Eventslist Manager page widget */
class EventsList : public QWidget
{
    Q_OBJECT

public:
    explicit EventsList(QWidget* parent = 0);
    ~EventsList();

    enum {
        COLUMN_EVENT,
        COLUMN_PRIORITY,
        COLUMN_STATUS,
        COLUMN_REGISTERED,
        COLUMN_SUPPORTING_ADDRESS,
        COLUMN_SUPPORTED_AMOUNT,
        COLUMN_MASTER_ADDRESS,
        COLUMN_COMPANY_ADDRESS,
        COLUMN_PROTX_HASH,
    };

    void setClientModel(ClientModel* clientModel);
    void setWalletModel(WalletModel* walletModel);

private:
    QMenu* contextMenuDIP3;
    int64_t nTimeFilterUpdatedDIP3;
    int64_t nTimeUpdatedDIP3;
    bool fFilterUpdatedDIP3;

    QTimer* timer;
    Ui::EventsList* ui;
    ClientModel* clientModel;
    WalletModel* walletModel;

    // Protects tableWidgetEventssDIP3
    CCriticalSection cs_dip3list;

    QString strCurrentFilterDIP3;

    bool mnListChanged;

    CDeterministicMNCPtr GetSelectedDIP3MN();

    void updateDIP3List();

Q_SIGNALS:
    void doubleClicked(const QModelIndex&);

private Q_SLOTS:
    void showContextMenuDIP3(const QPoint&);
    void on_filterLineEditDIP3_textChanged(const QString& strFilterIn);
    void on_checkBoxMyEventsOnly_stateChanged(int state);

    void extraInfoDIP3_clicked();
    void copyProTxHash_clicked();
    void copyCollateralOutpoint_clicked();

    void handleEventsListChanged();
    void updateDIP3ListScheduled();
};
#endif // BITCOIN_QT_EVENTSLIST_H

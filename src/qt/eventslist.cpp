#include <qt/eventslist.h>
#include <qt/smartnodelist.h>
#include <qt/forms/ui_eventslist.h>
//#include <qt/forms/ui_smartnodelist.h>

#include <qt/clientmodel.h>
#include <clientversion.h>
#include <coins.h>
#include <qt/guiutil.h>
#include <netbase.h>
#include <qt/walletmodel.h>
#include <validation.h>

#include <univalue.h>

#include <QMessageBox>
#include <QTableWidgetItem>
#include <QtGui/QClipboard>



template <typename T>
class CEventsListWidgetItem : public QTableWidgetItem
{
    T itemData;

public:
    explicit CEventsListWidgetItem(const QString& text, const T& data, int type = Type) :
        QTableWidgetItem(text, type),
        itemData(data) {}

    bool operator<(const QTableWidgetItem& other) const
    {
        return itemData < ((CEventsListWidgetItem*)&other)->itemData;
    }
};

EventsList::EventsList(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::EventsList),
    clientModel(0),
    walletModel(0),
    fFilterUpdatedDIP3(true),
    nTimeFilterUpdatedDIP3(0),
    nTimeUpdatedDIP3(0),
    mnListChanged(true)
{
    ui->setupUi(this);

    GUIUtil::setFont({ui->label_count_2,
                      ui->countLabelDIP3
                     }, GUIUtil::FontWeight::Bold, 14);
    GUIUtil::setFont({ui->label_filter_2}, GUIUtil::FontWeight::Normal, 15);

    int columnEventWidth = 200;
    int columnPriorityWidth = 80;
    int columnStatusWidth = 80;
    int columnRegisteredWidth = 80;
    int columnSupportingAddressWidth = 80;
    int columnSupportedAmountWidth = 100;
    int columnMasterAddressWidth = 130;
    int columnCompanyAddressWidth = 130;


    ui->tableWidgetEventsDIP3->setColumnWidth(COLUMN_EVENT, columnEventWidth);
    ui->tableWidgetEventsDIP3->setColumnWidth(COLUMN_PRIORITY, columnPriorityWidth);
    ui->tableWidgetEventsDIP3->setColumnWidth(COLUMN_STATUS, columnStatusWidth);
    ui->tableWidgetEventsDIP3->setColumnWidth(COLUMN_REGISTERED, columnRegisteredWidth);
    ui->tableWidgetEventsDIP3->setColumnWidth(COLUMN_SUPPORTING_ADDRESS, columnSupportingAddressWidth);
    ui->tableWidgetEventsDIP3->setColumnWidth(COLUMN_SUPPORTED_AMOUNT, columnSupportedAmountWidth);
    ui->tableWidgetEventsDIP3->setColumnWidth(COLUMN_MASTER_ADDRESS, columnMasterAddressWidth);
    ui->tableWidgetEventsDIP3->setColumnWidth(COLUMN_COMPANY_ADDRESS, columnCompanyAddressWidth);


    // dummy column for proTxHash
    // TODO use a proper table model for the MN list
    ui->tableWidgetEventsDIP3->insertColumn(COLUMN_PROTX_HASH);
    ui->tableWidgetEventsDIP3->setColumnHidden(COLUMN_PROTX_HASH, true);

    ui->tableWidgetEventsDIP3->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->filterLineEditDIP3->setPlaceholderText(tr("Filter by any property (e.g. event or company)"));

    QAction* copyProTxHashAction = new QAction(tr("Copy evoTx Hash"), this);
    QAction* copyCollateralOutpointAction = new QAction(tr("Copy Collateral Outpoint"), this);
    contextMenuDIP3 = new QMenu(this);
    contextMenuDIP3->addAction(copyProTxHashAction);
    contextMenuDIP3->addAction(copyCollateralOutpointAction);
    connect(ui->tableWidgetEventsDIP3, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenuDIP3(const QPoint&)));
    connect(ui->tableWidgetEventsDIP3, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(extraInfoDIP3_clicked()));
    connect(copyProTxHashAction, SIGNAL(triggered()), this, SLOT(copyProTxHash_clicked()));
    connect(copyCollateralOutpointAction, SIGNAL(triggered()), this, SLOT(copyCollateralOutpoint_clicked()));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateDIP3ListScheduled()));
    timer->start(1000);

    GUIUtil::updateFonts();
}

EventsList::~EventsList()
{
    delete ui;
}

void EventsList::setClientModel(ClientModel* model)
{
    this->clientModel = model;
    if (model) {
        // try to update list when events count changes
        connect(clientModel, SIGNAL(eventsListChanged()), this, SLOT(handleEventsListChanged()));
    }
}

void EventsList::setWalletModel(WalletModel* model)
{
    this->walletModel = model;
}

void EventsList::showContextMenuDIP3(const QPoint& point)
{
    QTableWidgetItem* item = ui->tableWidgetEventsDIP3->itemAt(point);
    if (item) contextMenuDIP3->exec(QCursor::pos());
}

void EventsList::handleEventsListChanged()
{
    LOCK(cs_dip3list);
    mnListChanged = true;
}

void EventsList::updateDIP3ListScheduled()
{
    TRY_LOCK(cs_dip3list, fLockAcquired);
    if (!fLockAcquired) return;

    if (!clientModel || clientModel->node().shutdownRequested()) {
        return;
    }

    // To prevent high cpu usage update only once in EVENTSLIST_FILTER_COOLDOWN_SECONDS seconds
    // after filter was last changed unless we want to force the update.
    if (fFilterUpdatedDIP3) {
        int64_t nSecondsToWait = nTimeFilterUpdatedDIP3 - GetTime() + EVENTSLIST_FILTER_COOLDOWN_SECONDS;
        ui->countLabelDIP3->setText(tr("Please wait...") + " " + QString::number(nSecondsToWait));

        if (nSecondsToWait <= 0) {
            updateDIP3List();
            fFilterUpdatedDIP3 = false;
        }
    } else if (mnListChanged) {
        int64_t nMnListUpdateSecods = clientModel->smartnodeSync().isBlockchainSynced() ? EVENTSLIST_UPDATE_SECONDS : EVENTSLIST_UPDATE_SECONDS * 10;
        int64_t nSecondsToWait = nTimeUpdatedDIP3 - GetTime() + nMnListUpdateSecods;

        if (nSecondsToWait <= 0) {
            updateDIP3List();
            mnListChanged = false;
        }
    }
}

void EventsList::updateDIP3List()
{
    if (!clientModel || clientModel->node().shutdownRequested()) {
        return;
    }

    auto mnList = clientModel->getEventsList();
    std::map<uint256, CTxDestination> mapCollateralDests;

    {
        // Get all UTXOs for each MN collateral in one go so that we can reduce locking overhead for cs_main
        // We also do this outside of the below Qt list update loop to reduce cs_main locking time to a minimum
        mnList.ForEachMN(false, [&](const CDeterministicMNCPtr& dmn) {
            CTxDestination collateralDest;
            Coin coin;
            if (clientModel->node().getUnspentOutput(dmn->collateralOutpoint, coin) && ExtractDestination(coin.out.scriptPubKey, collateralDest)) {
                mapCollateralDests.emplace(dmn->proTxHash, collateralDest);
            }
        });
    }

    LOCK(cs_dip3list);

    QString strToFilter;
    ui->countLabelDIP3->setText(tr("Updating..."));
    ui->tableWidgetEventsDIP3->setSortingEnabled(false);
    ui->tableWidgetEventsDIP3->clearContents();
    ui->tableWidgetEventsDIP3->setRowCount(0);

    nTimeUpdatedDIP3 = GetTime();

    auto projectedPayees = mnList.GetProjectedMNPayees(mnList.GetValidMNsCount());
    std::map<uint256, int> nextPayments;
    for (size_t i = 0; i < projectedPayees.size(); i++) {
        const auto& dmn = projectedPayees[i];
        nextPayments.emplace(dmn->proTxHash, mnList.GetHeight() + (int)i + 1);
    }

    std::set<COutPoint> setOutpts;
    if (walletModel && ui->checkBoxMyEventsOnly->isChecked()) {
        std::vector<COutPoint> vOutpts;
        walletModel->wallet().listProTxCoins(vOutpts);
        for (const auto& outpt : vOutpts) {
            setOutpts.emplace(outpt);
        }
    }

    mnList.ForEachMN(false, [&](const CDeterministicMNCPtr& dmn) {
        if (walletModel && ui->checkBoxMyEventsOnly->isChecked()) {
            bool fMyEvents = setOutpts.count(dmn->collateralOutpoint) ||
                walletModel->wallet().isSpendable(dmn->pdmnState->keyIDOwner) ||
                walletModel->wallet().isSpendable(dmn->pdmnState->keyIDVoting) ||
                walletModel->wallet().isSpendable(dmn->pdmnState->scriptPayout) ||
                walletModel->wallet().isSpendable(dmn->pdmnState->scriptOperatorPayout);
            if (!fMyEvents) return;
        }
        // populate list
        // Address, Protocol, Status, Active Seconds, Last Seen, Pub Key
        auto addr_key = dmn->pdmnState->addr.GetKey();
        QByteArray addr_ba(reinterpret_cast<const char*>(addr_key.data()), addr_key.size());
        Coin coin;
        //should this be call directly or use pcoinsTip->GetCoin(outpoint, coin) without locking cs_main
        bool isValidUtxo = GetUTXOCoin(dmn->collateralOutpoint, coin);
        SmartnodeCollaterals collaterals = Params().GetConsensus().nCollaterals;
        int nHeight = chainActive.Tip() == nullptr ? 0 : chainActive.Tip()->nHeight;
        QTableWidgetItem* collateralAmountItem = new QTableWidgetItem(!isValidUtxo ? tr("Invalid") : QString::number(coin.out.nValue / COIN));
        QTableWidgetItem* addressItem = new CEventsListWidgetItem<QByteArray>(QString::fromStdString(dmn->pdmnState->addr.ToString()), addr_ba);
        QTableWidgetItem* statusItem = new QTableWidgetItem(mnList.IsMNValid(dmn) ? tr("ENABLED") : (mnList.IsMNPoSeBanned(dmn) ? tr("POSE_BANNED") : tr("UNKNOWN")));
        QTableWidgetItem* PoSeScoreItem = new CEventsListWidgetItem<int>(QString::number(dmn->pdmnState->nPoSePenalty), dmn->pdmnState->nPoSePenalty);
        QTableWidgetItem* registeredItem = new CEventsListWidgetItem<int>(QString::number(dmn->pdmnState->nRegisteredHeight), dmn->pdmnState->nRegisteredHeight);
        QTableWidgetItem* lastPaidItem = new CEventsListWidgetItem<int>(QString::number(dmn->pdmnState->nLastPaidHeight), dmn->pdmnState->nLastPaidHeight);

        QString strNextPayment = "UNKNOWN";
        int nNextPayment = 0;
        if (nextPayments.count(dmn->proTxHash)) {
            nNextPayment = nextPayments[dmn->proTxHash];
            strNextPayment = QString::number(nNextPayment);
        }
        QTableWidgetItem* nextPaymentItem = new CEventsListWidgetItem<int>(strNextPayment, nNextPayment);

        CTxDestination payeeDest;
        QString payeeStr = tr("UNKNOWN");
        if (ExtractDestination(dmn->pdmnState->scriptPayout, payeeDest)) {
            payeeStr = QString::fromStdString(EncodeDestination(payeeDest));
        }
        QTableWidgetItem* payeeItem = new QTableWidgetItem(payeeStr);

        QString operatorRewardStr = tr("NONE");
        if (dmn->nOperatorReward) {
            operatorRewardStr = QString::number(dmn->nOperatorReward / 100.0, 'f', 2) + "% ";

            if (dmn->pdmnState->scriptOperatorPayout != CScript()) {
                CTxDestination operatorDest;
                if (ExtractDestination(dmn->pdmnState->scriptOperatorPayout, operatorDest)) {
                    operatorRewardStr += tr("to %1").arg(QString::fromStdString(EncodeDestination(operatorDest)));
                } else {
                    operatorRewardStr += tr("to UNKNOWN");
                }
            } else {
                operatorRewardStr += tr("but not claimed");
            }
        }
        QTableWidgetItem* operatorRewardItem = new CEventsListWidgetItem<uint16_t>(operatorRewardStr, dmn->nOperatorReward);

        QString collateralStr = tr("UNKNOWN");
        auto collateralDestIt = mapCollateralDests.find(dmn->proTxHash);
        if (collateralDestIt != mapCollateralDests.end()) {
            collateralStr = QString::fromStdString(EncodeDestination(collateralDestIt->second));
        }
        QTableWidgetItem* collateralItem = new QTableWidgetItem(collateralStr);

        QString ownerStr = QString::fromStdString(EncodeDestination(dmn->pdmnState->keyIDOwner));
        QTableWidgetItem* ownerItem = new QTableWidgetItem(ownerStr);

        QString votingStr = QString::fromStdString(EncodeDestination(dmn->pdmnState->keyIDVoting));
        QTableWidgetItem* votingItem = new QTableWidgetItem(votingStr);

        QTableWidgetItem* proTxHashItem = new QTableWidgetItem(QString::fromStdString(dmn->proTxHash.ToString()));

        if (strCurrentFilterDIP3 != "") {
            strToFilter = addressItem->text() + " " +
                          statusItem->text() + " " +
                          PoSeScoreItem->text() + " " +
                          registeredItem->text() + " " +
                          lastPaidItem->text() + " " +
                          nextPaymentItem->text() + " " +
                          payeeItem->text() + " " +
                          operatorRewardItem->text() + " " +
                          collateralItem->text() + " " +
                          collateralAmountItem->text() + " " +
                          ownerItem->text() + " " +
                          votingItem->text() + " " +
                          proTxHashItem->text();
            if (!strToFilter.contains(strCurrentFilterDIP3)) return;
        }

        ui->tableWidgetEventsDIP3->insertRow(0);
        ui->tableWidgetEventsDIP3->setItem(0, COLUMN_EVENT, addressItem);
        ui->tableWidgetEventsDIP3->setItem(0, COLUMN_PRIORITY, statusItem);
        ui->tableWidgetEventsDIP3->setItem(0, COLUMN_STATUS, PoSeScoreItem);
        ui->tableWidgetEventsDIP3->setItem(0, COLUMN_REGISTERED, registeredItem);
        ui->tableWidgetEventsDIP3->setItem(0, COLUMN_SUPPORTING_ADDRESS, lastPaidItem);
        ui->tableWidgetEventsDIP3->setItem(0, COLUMN_SUPPORTED_AMOUNT, nextPaymentItem);
        ui->tableWidgetEventsDIP3->setItem(0, COLUMN_MASTER_ADDRESS, payeeItem);
        ui->tableWidgetEventsDIP3->setItem(0, COLUMN_COMPANY_ADDRESS, operatorRewardItem);
        ui->tableWidgetEventsDIP3->setItem(0, COLUMN_PROTX_HASH, proTxHashItem);
    });
                        
     

    ui->countLabelDIP3->setText(QString::number(ui->tableWidgetEventsDIP3->rowCount()));
    ui->tableWidgetEventsDIP3->setSortingEnabled(true);
}

void EventsList::on_filterLineEditDIP3_textChanged(const QString& strFilterIn)
{
    strCurrentFilterDIP3 = strFilterIn;
    nTimeFilterUpdatedDIP3 = GetTime();
    fFilterUpdatedDIP3 = true;
    ui->countLabelDIP3->setText(tr("Please wait...") + " " + QString::number(EVENTSLIST_FILTER_COOLDOWN_SECONDS));
}



void EventsList::on_checkBoxMyEventsOnly_stateChanged(int state)
{
    // no cooldown
    nTimeFilterUpdatedDIP3 = GetTime() - EVENTSLIST_FILTER_COOLDOWN_SECONDS;
    fFilterUpdatedDIP3 = true;
}


CDeterministicMNCPtr EventsList::GetSelectedDIP3MN()
{
    if (!clientModel) {
        return nullptr;
    }

    std::string strProTxHash;
    {
        LOCK(cs_dip3list);

        QItemSelectionModel* selectionModel = ui->tableWidgetEventsDIP3->selectionModel();
        QModelIndexList selected = selectionModel->selectedRows();

        if (selected.count() == 0) return nullptr;

        QModelIndex index = selected.at(0);
        int nSelectedRow = index.row();
        strProTxHash = ui->tableWidgetEventsDIP3->item(nSelectedRow, COLUMN_PROTX_HASH)->text().toStdString();
    }

    uint256 proTxHash;
    proTxHash.SetHex(strProTxHash);

    auto mnList = clientModel->getEventsList();
    return mnList.GetMN(proTxHash);
}

void EventsList::extraInfoDIP3_clicked()
{
    auto dmn = GetSelectedDIP3MN();
    if (!dmn) {
        return;
    }

    UniValue json(UniValue::VOBJ);
    dmn->ToJson(json);

    // Title of popup window
    QString strWindowtitle = tr("Additional information for DIP3 Events %1").arg(QString::fromStdString(dmn->proTxHash.ToString()));
    QString strText = QString::fromStdString(json.write(2));

    QMessageBox::information(this, strWindowtitle, strText);
}

void EventsList::copyProTxHash_clicked()
{
    auto dmn = GetSelectedDIP3MN();
    if (!dmn) {
        return;
    }

    QApplication::clipboard()->setText(QString::fromStdString(dmn->proTxHash.ToString()));
}

void EventsList::copyCollateralOutpoint_clicked()
{
    auto dmn = GetSelectedDIP3MN();
    if (!dmn) {
        return;
    }

    QApplication::clipboard()->setText(QString::fromStdString(dmn->collateralOutpoint.ToStringShort()));
}

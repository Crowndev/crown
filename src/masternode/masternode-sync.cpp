// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrman.h>
#include <crown/spork.h>
#include <masternode/activemasternode.h>
#include <masternode/masternode-budget.h>
#include <masternode/masternode-payments.h>
#include <masternode/masternode-sync.h>
#include <masternode/masternode.h>
#include <masternode/masternodeman.h>
#include <netfulfilledman.h>
#include <netmessagemaker.h>
#include <node/context.h>
#include <node/ui_interface.h>
#include <rpc/blockchain.h>
#include <shutdown.h>
#include <util/system.h>
#include <util/time.h>

class CMasternodeSync;
CMasternodeSync masternodeSync;

CMasternodeSync::CMasternodeSync()
{
    Reset();
}

bool CMasternodeSync::IsSynced()
{
    return RequestedMasternodeAssets == MASTERNODE_SYNC_FINISHED;
}

bool CMasternodeSync::IsBlockchainSynced()
{
    if (::ChainstateActive().IsInitialBlockDownload())
        return false;
        
    CBlockIndex* pindex = ::ChainActive().Tip();

    if (!gArgs.GetBoolArg("-jumpstart", false) && pindex->nTime + 60*60 < GetTime())
        return false;

    return true;
}

void CMasternodeSync::Reset()
{   
    lastMasternodeList = 0;
    lastMasternodeWinner = 0;
    lastBudgetItem = 0;
    mapSeenSyncMNB.clear();
    mapSeenSyncMNW.clear();
    mapSeenSyncBudget.clear();
    lastFailure = 0;
    nCountFailures = 0;
    sumMasternodeList = 0;
    sumMasternodeWinner = 0;
    sumBudgetItemProp = 0;
    sumBudgetItemFin = 0;
    countMasternodeList = 0;
    countMasternodeWinner = 0;
    countBudgetItemProp = 0;
    countBudgetItemFin = 0;
    RequestedMasternodeAssets = MASTERNODE_SYNC_INITIAL;
    RequestedMasternodeAttempt = 0;
    nAssetSyncStarted = GetTime();
}

void CMasternodeSync::AddedMasternodeList(uint256 hash)
{
    if(mnodeman.mapSeenMasternodeBroadcast.count(hash)) {
        if(mapSeenSyncMNB[hash] < MASTERNODE_SYNC_THRESHOLD) {
            lastMasternodeList = GetTime();
            mapSeenSyncMNB[hash]++;
        }
    } else {
        lastMasternodeList = GetTime();
        mapSeenSyncMNB.insert(make_pair(hash, 1));
    }
}

void CMasternodeSync::AddedMasternodeWinner(uint256 hash)
{
    if(masternodePayments.mapMasternodePayeeVotes.count(hash)) {
        if(mapSeenSyncMNW[hash] < MASTERNODE_SYNC_THRESHOLD) {
            lastMasternodeWinner = GetTime();
            mapSeenSyncMNW[hash]++;
        }
    } else {
        lastMasternodeWinner = GetTime();
        mapSeenSyncMNW.insert(make_pair(hash, 1));
    }
}

void CMasternodeSync::AddedBudgetItem(uint256 hash)
{
    if(budget.HasItem(hash)) {
        if(mapSeenSyncBudget[hash] < MASTERNODE_SYNC_THRESHOLD) {
            lastBudgetItem = GetTime();
            mapSeenSyncBudget[hash]++;
        }
    } else {
        lastBudgetItem = GetTime();
        mapSeenSyncBudget.insert(make_pair(hash, 1));
    }
}

bool CMasternodeSync::IsBudgetPropEmpty()
{
    return sumBudgetItemProp==0 && countBudgetItemProp>0;
}

bool CMasternodeSync::IsBudgetFinEmpty()
{
    return sumBudgetItemFin==0 && countBudgetItemFin>0;
}

void CMasternodeSync::GetNextAsset()
{
    switch(RequestedMasternodeAssets)
    {
        case(MASTERNODE_SYNC_INITIAL):
        case(MASTERNODE_SYNC_FAILED):
            ClearFulfilledRequest();
            RequestedMasternodeAssets = MASTERNODE_SYNC_SPORKS;
            break;
        case(MASTERNODE_SYNC_SPORKS):
            RequestedMasternodeAssets = MASTERNODE_SYNC_LIST;
            break;
        case(MASTERNODE_SYNC_LIST):
            RequestedMasternodeAssets = MASTERNODE_SYNC_MNW;
            break;
        case(MASTERNODE_SYNC_MNW):
            RequestedMasternodeAssets = MASTERNODE_SYNC_BUDGET;
            break;
        case(MASTERNODE_SYNC_BUDGET):
            LogPrintf("CMasternodeSync::GetNextAsset - Sync has finished\n");
            RequestedMasternodeAssets = MASTERNODE_SYNC_FINISHED;
            uiInterface.NotifyAdditionalDataSyncProgressChanged(1);
            break;
    }
    RequestedMasternodeAttempt = 0;
    nAssetSyncStarted = GetTime();
}

std::string CMasternodeSync::GetSyncStatus()
{
    switch (masternodeSync.RequestedMasternodeAssets) {
        case MASTERNODE_SYNC_INITIAL: return ("Synchronization pending...");
        case MASTERNODE_SYNC_SPORKS: return ("Synchronizing masternode sporks...");
        case MASTERNODE_SYNC_LIST: return ("Synchronizing masternodes...");
        case MASTERNODE_SYNC_MNW: return ("Synchronizing masternode winners...");
        case MASTERNODE_SYNC_BUDGET: return ("Synchronizing masternode budgets...");
        case MASTERNODE_SYNC_FAILED: return ("Masternode synchronization failed");
        case MASTERNODE_SYNC_FINISHED: return ("Masternode synchronization finished");
    }
    return "";
}

void CMasternodeSync::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv)
{
    if (strCommand == "ssc") {

        if (IsSynced())
            return;

        int nItemID;
        int nCount;
        vRecv >> nItemID >> nCount;

        //this means we will receive no further communication
        switch (nItemID) {
            case(MASTERNODE_SYNC_LIST):
                if(nItemID != RequestedMasternodeAssets) return;
                sumMasternodeList += nCount;
                countMasternodeList++;
                break;
            case(MASTERNODE_SYNC_MNW):
                if(nItemID != RequestedMasternodeAssets) return;
                sumMasternodeWinner += nCount;
                countMasternodeWinner++;
                break;
            case(MASTERNODE_SYNC_BUDGET_PROP):
                if(RequestedMasternodeAssets != MASTERNODE_SYNC_BUDGET) return;
                sumBudgetItemProp += nCount;
                countBudgetItemProp++;
                break;
            case(MASTERNODE_SYNC_BUDGET_FIN):
                if(RequestedMasternodeAssets != MASTERNODE_SYNC_BUDGET) return;
                sumBudgetItemFin += nCount;
                countBudgetItemFin++;
                break;
        }
        
        LogPrintf("CMasternodeSync:ProcessMessage - ssc - got inventory count %d %d\n", nItemID, nCount);
    }
}

void CMasternodeSync::ClearFulfilledRequest()
{
    g_connman->ForEachNode([](CNode* pnode) {
        netfulfilledman.RemoveFulfilledRequest(pnode->addr, "getspork");
        netfulfilledman.RemoveFulfilledRequest(pnode->addr, "mnsync");
        netfulfilledman.RemoveFulfilledRequest(pnode->addr, "mnwsync");
        netfulfilledman.RemoveFulfilledRequest(pnode->addr, "busync");
    });
}

void CMasternodeSync::Process()
{
    const CNetMsgMaker msgMaker(PROTOCOL_VERSION);
    static int tick = 0;

    if (tick++ % MASTERNODE_SYNC_TIMEOUT != 0)
        return;

    if (IsSynced()) {
        return;
    }

    //try syncing again
    if (RequestedMasternodeAssets == MASTERNODE_SYNC_FAILED && lastFailure + (1*60) < GetTime()) {
        Reset();
    } else if (RequestedMasternodeAssets == MASTERNODE_SYNC_FAILED) {
        return;
    }

    // Calculate "progress" for LOG reporting / GUI notification
    double nSyncProgress = double(RequestedMasternodeAttempt + (RequestedMasternodeAssets - 1) * 8) / (8*4);
    uiInterface.NotifyAdditionalDataSyncProgressChanged(nSyncProgress);
    LogPrintf("CMasternodeSync::ProcessTick -- nTick %d nRequestedMasternodeAssets %d nRequestedMasternodeAttempt %d nSyncProgress %f\n", tick, RequestedMasternodeAssets, RequestedMasternodeAttempt, nSyncProgress);

    if (!IsBlockchainSynced() && RequestedMasternodeAssets > MASTERNODE_SYNC_SPORKS)
        return;

    if (RequestedMasternodeAssets == MASTERNODE_SYNC_INITIAL)
        GetNextAsset();

    std::vector<CNode*> vNodesCopy = g_connman->CopyNodeVector();
    for (auto& pnode : vNodesCopy) {
        if (RequestedMasternodeAssets == MASTERNODE_SYNC_SPORKS) {
            if (netfulfilledman.HasFulfilledRequest(pnode->addr, "getspork"))
                continue;
            netfulfilledman.AddFulfilledRequest(pnode->addr, "getspork");
            g_connman->PushMessage(pnode, CNetMsgMaker(pnode->GetCommonVersion()).Make(NetMsgType::GETSPORKS));
            if (RequestedMasternodeAttempt >= 2)
                GetNextAsset();
            RequestedMasternodeAttempt++;
            return;
        }

        if (pnode->nVersion >= masternodePayments.GetMinMasternodePaymentsProto()) {
            if (RequestedMasternodeAssets == MASTERNODE_SYNC_LIST) {
                LogPrintf("CMasternodeSync::Process() - lastMasternodeList %lld (GetTime() - MASTERNODE_SYNC_TIMEOUT) %lld\n", lastMasternodeList, GetTime() - MASTERNODE_SYNC_TIMEOUT);
                if (lastMasternodeList > 0 && lastMasternodeList < GetTime() - MASTERNODE_SYNC_TIMEOUT * 2 && RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD) {
                    GetNextAsset();
                    return;
                }

                if (netfulfilledman.HasFulfilledRequest(pnode->addr, "mnsync"))
                    continue;
                netfulfilledman.AddFulfilledRequest(pnode->addr, "mnsync");

                // timeout
                if (lastMasternodeList == 0 && (RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD * 3 || GetTime() - nAssetSyncStarted > MASTERNODE_SYNC_TIMEOUT * 5)) {
                    if (IsSporkActive(SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT)) {
                        LogPrintf("CMasternodeSync::Process - ERROR - Sync has failed, will retry later\n");
                        RequestedMasternodeAssets = MASTERNODE_SYNC_FAILED;
                        RequestedMasternodeAttempt = 0;
                        lastFailure = GetTime();
                        nCountFailures++;
                    } else {
                        GetNextAsset();
                    }
                    return;
                }

                if (RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD * 3)
                    return;

                mnodeman.DsegUpdate(pnode);
                RequestedMasternodeAttempt++;
                return;
            }

            if (RequestedMasternodeAssets == MASTERNODE_SYNC_MNW) {
                if (lastMasternodeWinner > 0 && lastMasternodeWinner < GetTime() - MASTERNODE_SYNC_TIMEOUT * 2 && RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD) { //hasn't received a new item in the last five seconds, so we'll move to the
                    GetNextAsset();
                    return;
                }

                if (netfulfilledman.HasFulfilledRequest(pnode->addr, "mnwsync"))
                    continue;
                netfulfilledman.AddFulfilledRequest(pnode->addr, "mnwsync");

                // timeout
                if (lastMasternodeWinner == 0 && (RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD * 3 || GetTime() - nAssetSyncStarted > MASTERNODE_SYNC_TIMEOUT * 5)) {
                    if (IsSporkActive(SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT)) {
                        LogPrintf("CMasternodeSync::Process - ERROR - Sync has failed, will retry later\n");
                        RequestedMasternodeAssets = MASTERNODE_SYNC_FAILED;
                        RequestedMasternodeAttempt = 0;
                        lastFailure = GetTime();
                        nCountFailures++;
                    } else {
                        GetNextAsset();
                    }
                    return;
                }

                if (RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD * 3)
                    return;

                int nMnCount = mnodeman.CountEnabled();
                g_connman->PushMessage(pnode, msgMaker.Make("mnget", nMnCount)); //sync payees
                RequestedMasternodeAttempt++;

                return;
            }
        }

        if (RequestedMasternodeAssets == MASTERNODE_SYNC_BUDGET) {
            //we'll start rejecting votes if we accidentally get set as synced too soon
            if (lastBudgetItem > 0 && lastBudgetItem < GetTime() - MASTERNODE_SYNC_TIMEOUT * 2 && RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD) { //hasn't received a new item in the last five seconds, so we'll move to the
                // Hasn't received a new item in the last five seconds, so we'll move to the
                GetNextAsset();

                //try to activate our masternode if possible
                activeMasternode.ManageStatus();

                return;
            }

            // timeout
            if (lastBudgetItem == 0 && (RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD * 3 || GetTime() - nAssetSyncStarted > MASTERNODE_SYNC_TIMEOUT * 5)) {
                // maybe there is no budgets at all, so just finish syncing
                GetNextAsset();
                activeMasternode.ManageStatus();
                return;
            }

            if (netfulfilledman.HasFulfilledRequest(pnode->addr, "busync"))
                continue;
            netfulfilledman.AddFulfilledRequest(pnode->addr, "busync");

            if (RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD * 3)
                return;

            uint256 n = uint256();
            g_connman->PushMessage(pnode, CNetMsgMaker(pnode->GetCommonVersion()).Make(NetMsgType::BUDGETVOTESYNC, n));
            RequestedMasternodeAttempt++;

            return;
        }
    }
}

void ThreadCheckMasternode(CConnman& connman)
{
    static bool fOneThread;
    if(fOneThread) return;
    fOneThread = true;

    // Make this thread recognisable as the PrivateSend thread
    RenameThread("crown-privsend");

    // Don't enter the loop until we are out of IBD
    while (true) {
        UninterruptibleSleep(std::chrono::seconds{10});

        if (fReindex) continue;
        if (!::ChainstateActive().IsInitialBlockDownload()) break;
        if (ShutdownRequested()) break;
    }

    unsigned int nTick = 0;

    while (true)
    {
        UninterruptibleSleep(std::chrono::seconds{10});

        // try to sync from all available nodes, one step at a time
        masternodeSync.Process();

        if(masternodeSync.IsBlockchainSynced() && !ShutdownRequested()) {

            nTick++;

            // make sure to check all masternodes first
            mnodeman.Check();

            // check if we should activate or ping every few minutes,
            // start right after sync is considered to be done
            if (nTick % MASTERNODE_PING_SECONDS == 0)
                activeMasternode.ManageStatus();

            if(nTick % 60 == 0) {
                mnodeman.CheckAndRemove();
                masternodePayments.CheckAndRemove();
            }
        }
    }
}


// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crown/nodesync.h>

#include <crown/instantx.h>
#include <index/txindex.h>
#include <init.h>
#include <masternode/masternodeman.h>
#include <node/context.h>
#include <rpc/blockchain.h>
#include <rpc/util.h>
#include <script/sign.h>
#include <shutdown.h>
#include <systemnode/systemnodeman.h>
#include <util/message.h>
#include <util/system.h>
#include <util/time.h>
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>

#include <algorithm>

std::string currentSyncStatus()
{
    static int64_t lastStatusTime;
    static std::string lastStatusMessage;

    if (lastStatusTime != GetTime()) {
        lastStatusTime = GetTime();
        if (lastStatusMessage != systemnodeSync.GetSyncStatus()) {
            lastStatusMessage = systemnodeSync.GetSyncStatus();
        } else {
            lastStatusMessage = masternodeSync.GetSyncStatus();
        }
        return lastStatusMessage;
    }

    return lastStatusMessage;
}

void ThreadNodeSync(CConnman& connman)
{
    util::ThreadRename("crown-nodesync");

    if (::ChainstateActive().IsInitialBlockDownload())
        return;
    if (ShutdownRequested())
        return;

    static unsigned int c1 = 0;
    static unsigned int c2 = 0;

    // try to sync from all available nodes, one step at a time
    masternodeSync.Process();
    systemnodeSync.Process();

    {

        c1++;

        //mnodeman.Check();

        // check if we should activate or ping every few minutes,
        // start right after sync is considered to be done
        if (c1 % MASTERNODE_PING_SECONDS == 15)
            activeMasternode.ManageStatus();

        if (c1 % 60 == 0) {
            mnodeman.CheckAndRemove();
            mnodeman.ProcessMasternodeConnections();
            masternodePayments.CheckAndRemove();
            instantSend.CheckAndRemove();
        }
    }

    {

        c2++;

        //snodeman.Check();

        // check if we should activate or ping every few minutes,
        // start right after sync is considered to be done
        if (c2 % SYSTEMNODE_PING_SECONDS == 15)
            activeSystemnode.ManageStatus();

        if (c2 % 60 == 0) {
            snodeman.CheckAndRemove();
            snodeman.ProcessSystemnodeConnections();
            systemnodePayments.CheckAndRemove();
            instantSend.CheckAndRemove();
        }
    }
}

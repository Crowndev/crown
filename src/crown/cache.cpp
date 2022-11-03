// Copyright (c) 2014-2020 The Crown developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crown/cache.h>

void DumpCaches()
{
    CFlatDB<CMasternodeMan> flatdb1(fs::PathFromString("mncache.dat"), "magicMasternodeCache");
    flatdb1.Dump(mnodeman);
    CFlatDB<CSystemnodeMan> flatdb2(fs::PathFromString("sncache.dat"), "magicSystemnodeCache");
    flatdb2.Dump(snodeman);
    CFlatDB<CMasternodePayments> flatdb3(fs::PathFromString("mnpayments.dat"), "magicMasternodePaymentsCache");
    flatdb3.Dump(masternodePayments);
    CFlatDB<CSystemnodePayments> flatdb4(fs::PathFromString("snpayments.dat"), "magicSystemnodePaymentsCache");
    flatdb4.Dump(systemnodePayments);
    CFlatDB<CBudgetManager> flatdb5(fs::PathFromString("budget.dat"), "magicBudgetCache");
    flatdb5.Dump(budget);
    CFlatDB<CNetFulfilledRequestManager> flatdb6(fs::PathFromString("netfulfilled.dat"), "magicFulfilledCache");
    flatdb6.Dump(netfulfilledman);
}

bool LoadCaches()
{
    std::string strDBName;
    fs::path pathDB = GetDataDir();

    strDBName = "mncache.dat";
    uiInterface.InitMessage("Loading masternode cache...");
    CFlatDB<CMasternodeMan> flatdb1(fs::PathFromString(strDBName), "magicMasternodeCache");
    if (!flatdb1.Load(mnodeman)) {
        LogPrintf("Failed to load masternode cache.");
        return false;
    }

    strDBName = "sncache.dat";
    uiInterface.InitMessage("Loading systemnode cache...");
    CFlatDB<CSystemnodeMan> flatdb2(fs::PathFromString(strDBName), "magicSystemnodeCache");
    if (!flatdb2.Load(snodeman)) {
        LogPrintf("Failed to load systemnode cache.");
        return false;
    }

    if (mnodeman.size()) {
        strDBName = "mnpayments.dat";
        uiInterface.InitMessage("Loading masternode payment cache...");
        CFlatDB<CMasternodePayments> flatdb3(fs::PathFromString(strDBName), "magicMasternodePaymentsCache");
        if (!flatdb3.Load(masternodePayments)) {
            LogPrintf("Failed to load masternode payments cache.");
            return false;
        }
    }

    if (snodeman.size()) {
        strDBName = "snpayments.dat";
        uiInterface.InitMessage("Loading systemnode payment cache...");
        CFlatDB<CSystemnodePayments> flatdb4(fs::PathFromString(strDBName), "magicSystemnodePaymentsCache");
        if (!flatdb4.Load(systemnodePayments)) {
            LogPrintf("Failed to load systemnode payments cache.");
            return false;
        }
    }

    strDBName = "budget.dat";
    uiInterface.InitMessage("Loading budget cache...");
    CFlatDB<CBudgetManager> flatdb5(fs::PathFromString(strDBName), "magicBudgetCache");
    if (!flatdb5.Load(budget)) {
        LogPrintf("Failed to load budget cache.");
        return false;
    }

    strDBName = "netfulfilled.dat";
    uiInterface.InitMessage("Loading fulfilled requests cache...");
    CFlatDB<CNetFulfilledRequestManager> flatdb6(fs::PathFromString(strDBName), "magicFulfilledCache");
    if (!flatdb6.Load(netfulfilledman)) {
        LogPrintf("Failed to load fulfilled requests cache.");
        return false;
    }

    return true;
}

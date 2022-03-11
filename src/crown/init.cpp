// Copyright (c) 2014-2020 The Crown developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crown/init.h>

void loadNodeConfiguration()
{
    masternodeConfig.clear();
    systemnodeConfig.clear();

    //mnodeman.nodeDiag = gArgs.GetBoolArg("-diagnode", DEFAULT_NODEDIAG);
    //snodeman.nodeDiag = gArgs.GetBoolArg("-diagnode", DEFAULT_NODEDIAG);

    // parse masternode.conf
    std::string strErr;
    if (!masternodeConfig.read(strErr)) {
        LogPrintf("Error reading masternode configuration file: %s\n", strErr.c_str());
    }

    // parse systemnode.conf
    if (!systemnodeConfig.read(strErr)) {
        LogPrintf("Error reading systemnode configuration file: %s\n", strErr.c_str());
    }
}

bool setupNodeConfiguration()
{
    fMasterNode = gArgs.GetBoolArg("-masternode", false);
    fSystemNode = gArgs.GetBoolArg("-systemnode", false);

    if (fMasterNode && fSystemNode) {
        return InitError(strprintf(_("Masternode and Systemnode cannot run together")));
    }

    std::shared_ptr<CWallet> pwallet = GetMainWallet();

    if (fMasterNode) {
        LogPrintf("IS MASTERNODE\n");
        strMasterNodeAddr = gArgs.GetArg("-masternodeaddr", "");
        LogPrintf(" addr %s\n", strMasterNodeAddr.c_str());
        if (!strMasterNodeAddr.empty()) {
            CService addrTest = CService(LookupNumeric(strMasterNodeAddr.c_str()));
            if (!addrTest.IsValid()) {
                return InitError(strprintf(_("Invalid -masternodeaddr address")));
            }
        }

        strMasterNodePrivKey = gArgs.GetArg("-masternodeprivkey", "");
        if (!strMasterNodePrivKey.empty()) {
            std::string errorMessage;
            CKey key;
            CPubKey pubkey;
            if (!legacySigner.SetKey(strMasterNodePrivKey, key, pubkey)) {
                return InitError(strprintf(_("Invalid masternodeprivkey. Please see documentation.")));
            }
            activeMasternode.pubKeyMasternode = pubkey;
        } else {
            return InitError(_("You must specify a masternodeprivkey in the configuration. Please see documentation for help."));
        }
    }

    if (fSystemNode) {
        LogPrintf("IS SYSTEMNODE\n");
        strSystemNodeAddr = gArgs.GetArg("-systemnodeaddr", "");
        LogPrintf(" addr %s\n", strSystemNodeAddr.c_str());
        if (!strSystemNodeAddr.empty()) {
            CService addrTest = CService(LookupNumeric(strSystemNodeAddr.c_str()));
            if (!addrTest.IsValid()) {
                return InitError(strprintf(_("Invalid -systemnodeaddr address")));
            }
        }

        strSystemNodePrivKey = gArgs.GetArg("-systemnodeprivkey", "");
        if (!strSystemNodePrivKey.empty()) {
            std::string errorMessage;
            CKey key;
            CPubKey pubkey;
            if (!legacySigner.SetKey(strSystemNodePrivKey, key, pubkey)) {
                return InitError(strprintf(_("Invalid systemnodeprivkey. Please see documentation.")));
            }
            activeSystemnode.pubKeySystemnode = pubkey;
        } else {
            return InitError(_("You must specify a systemnodeprivkey in the configuration. Please see documentation for help."));
        }
    }

    strBudgetMode = gArgs.GetArg("-budgetvotemode", "auto");

    if (gArgs.GetBoolArg("-mnconflock", true) && masternodeConfig.getCount() > 0)
    {
        LogPrintf("Locking Masternodes:\n");
        uint256 mnTxHash;
        for (CNodeEntry mne : masternodeConfig.getEntries()) {
            LogPrintf("  %s %s\n", mne.getTxHash(), mne.getOutputIndex());
            mnTxHash.SetHex(mne.getTxHash());
            COutPoint outpoint = COutPoint(mnTxHash, boost::lexical_cast<unsigned int>(mne.getOutputIndex()));
            if (pwallet) {
                LOCK(pwallet->cs_wallet);
                pwallet->LockCoin(outpoint);
            }
        }
    }

    if (gArgs.GetBoolArg("-snconflock", true) && systemnodeConfig.getCount() > 0)
    {
        LogPrintf("Locking Systemnodes:\n");
        uint256 mnTxHash;
        for (CNodeEntry sne : systemnodeConfig.getEntries()) {
            LogPrintf("  %s %s\n", sne.getTxHash(), sne.getOutputIndex());
            mnTxHash.SetHex(sne.getTxHash());
            COutPoint outpoint = COutPoint(mnTxHash, boost::lexical_cast<unsigned int>(sne.getOutputIndex()));
            if (pwallet) {
                LOCK(pwallet->cs_wallet);
                pwallet->LockCoin(outpoint);
            }
        }
    }

    fEnableInstantX = gArgs.GetBoolArg("-enableinstantx", fEnableInstantX);
    nInstantXDepth = gArgs.GetArg("-instantxdepth", nInstantXDepth);
    nInstantXDepth = std::min(std::max(nInstantXDepth, 0), 60);

    LogPrintf("nInstantXDepth %d\n", nInstantXDepth);
    LogPrintf("Budget Mode %s\n", strBudgetMode.c_str());

    legacySigner.InitCollateralAddress();

    return true;
}

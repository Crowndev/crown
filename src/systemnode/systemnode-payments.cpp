// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrman.h>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <crown/legacysigner.h>
#include <crown/spork.h>
#include <key_io.h>
#include <masternode/masternode-budget.h>
#include <net_processing.h>
#include <netfulfilledman.h>
#include <netmessagemaker.h>
#include <sync.h>
#include <systemnode/systemnode-payments.h>
#include <systemnode/systemnode-sync.h>
#include <systemnode/systemnodeman.h>
#include <util/moneystr.h>
#include <util/system.h>

/** Object for who's going to get paid on which blocks */
CSystemnodePayments systemnodePayments;

RecursiveMutex cs_vecSNPayments;
RecursiveMutex cs_mapSystemnodeBlocks;
RecursiveMutex cs_mapSystemnodePayeeVotes;

bool SNIsBlockPayeeValid(const CAmount& nValueCreated, const CTransaction& txNew, int nBlockHeight, const uint32_t& nTime, const uint32_t& nTimePrevBlock)
{
    if (!systemnodeSync.IsSynced()) { //there is no budget data to use to check anything -- find the longest chain
        if (gArgs.GetBoolArg("-debug", false))
            LogPrint(BCLog::SYSTEMNODE, "Client not synced, skipping block payee checks\n");
        return true;
    }

    //check if it's a budget block
    if (IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS) || Params().NetworkIDString() == CBaseChainParams::TESTNET) {
        if (budget.IsBudgetPaymentBlock(nBlockHeight)) {
            if (budget.IsTransactionValid(txNew, nBlockHeight)) {
                return true;
            } else {
                LogPrint(BCLog::SYSTEMNODE, "Invalid budget payment detected %s\n", txNew.ToString().c_str());
                if (IsSporkActive(SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT)) {
                    return false;
                } else {
                    LogPrint(BCLog::SYSTEMNODE, "Budget enforcement is disabled, accepting block\n");
                    return true;
                }
            }
        }
    }

    //check for systemnode payee
    if (systemnodePayments.IsTransactionValid(nValueCreated, txNew, nBlockHeight)) {
        return true;
    } else if (nTime - nTimePrevBlock > (unsigned int)Params().ChainStallDuration()) {
        // The chain has stalled, allow the first block to have no payment to winners
        LogPrint(BCLog::SYSTEMNODE, "%s: Chain stall, time between blocks=%d\n", __func__, nTime - nTimePrevBlock);
        return true;
    } else {
        LogPrint(BCLog::SYSTEMNODE, "Invalid mn payment detected \n");
        if (IsSporkActive(SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT)) {
            return false;
        } else {
            LogPrint(BCLog::SYSTEMNODE, "Systemnode payment enforcement is disabled, accepting block\n");
            return true;
        }
    }

    return false;
}

bool CSystemnodePayments::IsTransactionValid(const CAmount& nValueCreated, const CTransaction& txNew, int nBlockHeight)
{
    LOCK(cs_mapSystemnodeBlocks);

    if (mapSystemnodeBlocks.count(nBlockHeight)) {
        return mapSystemnodeBlocks[nBlockHeight].IsTransactionValid(txNew, nValueCreated);
    }

    return true;
}

int CSystemnodePayments::GetMinSystemnodePaymentsProto() const
{
    return IsSporkActive(SPORK_15_SYSTEMNODE_DONT_PAY_OLD_NODES)
        ? MIN_SYSTEMNODE_PAYMENT_PROTO_VERSION_CURR
        : MIN_SYSTEMNODE_PAYMENT_PROTO_VERSION_PREV;
}

void CSystemnodePayments::ProcessMessageSystemnodePayments(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman)
{
    if (!systemnodeSync.IsBlockchainSynced())
        return;

    if (strCommand == NetMsgType::GETSNWINNERS) {
        int nCountNeeded;
        vRecv >> nCountNeeded;

        if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
            if (netfulfilledman.HasFulfilledRequest(pfrom->addr, NetMsgType::GETSNWINNERS)) {
                LogPrint(BCLog::SYSTEMNODE, "snget - peer already asked me for the list\n");
                Misbehaving(pfrom->GetId(), 20);
                return;
            }
        }
        netfulfilledman.AddFulfilledRequest(pfrom->addr, NetMsgType::GETSNWINNERS);
        systemnodePayments.Sync(pfrom, nCountNeeded, *connman);
        LogPrint(BCLog::SYSTEMNODE, "snget - Sent Systemnode winners to %s\n", pfrom->addr.ToString().c_str());
    }

    if (strCommand == NetMsgType::SNWINNER) {
        CSystemnodePaymentWinner winner;
        vRecv >> winner;

        if (pfrom->nVersion < MIN_MNW_PEER_PROTO_VERSION)
            return;

        int nHeight;
        {
            TRY_LOCK(cs_main, locked);
            if (!locked || ::ChainActive().Tip() == nullptr)
                return;
            nHeight = ::ChainActive().Tip()->nHeight;
        }

        if (systemnodePayments.mapSystemnodePayeeVotes.count(winner.GetHash())) {
            LogPrint(BCLog::SYSTEMNODE, "snw - Already seen - %s bestHeight %d\n", winner.GetHash().ToString().c_str(), nHeight);
            systemnodeSync.AddedSystemnodeWinner(winner.GetHash());
            return;
        }

        int nFirstBlock = nHeight - (snodeman.CountEnabled() * 1.25);
        if (winner.nBlockHeight < nFirstBlock || winner.nBlockHeight > nHeight + 20) {
            LogPrint(BCLog::SYSTEMNODE, "snw - winner out of range - FirstBlock %d Height %d bestHeight %d\n", nFirstBlock, winner.nBlockHeight, nHeight);
            return;
        }

        std::string strError = "";
        if (!winner.IsValid(pfrom, strError, *connman)) {
            if (strError != "")
                LogPrint(BCLog::SYSTEMNODE, "snw - invalid message - %s\n", strError);
            return;
        }

        if(!systemnodePayments.CanVote(winner.vinSystemnode.prevout, winner.nBlockHeight)){
            LogPrint(BCLog::SYSTEMNODE, "snw - systemnode already voted - %s\n", winner.vinSystemnode.prevout.ToStringShort());
            return;
        }

        if (!winner.SignatureValid()) {
            LogPrint(BCLog::SYSTEMNODE, "snw - invalid signature\n");
            if (systemnodeSync.IsSynced())
                Misbehaving(pfrom->GetId(), 20);
            // it could just be a non-synced systemnode
            snodeman.AskForSN(pfrom, winner.vinSystemnode, *connman);
            return;
        }

        LogPrint(BCLog::SYSTEMNODE, "snw - winning vote - Addr %s Height %d bestHeight %d - %s\n", winner.payee.ToString(), winner.nBlockHeight, nHeight, winner.vinSystemnode.prevout.ToStringShort());

        if (systemnodePayments.AddWinningSystemnode(winner)) {
            winner.Relay(*connman);
            systemnodeSync.AddedSystemnodeWinner(winner.GetHash());
        }
    }
}

bool CSystemnodePayments::CanVote(COutPoint outSystemnode, int nBlockHeight)
{
    LOCK(cs_mapSystemnodePayeeVotes);

    if (mapSystemnodesLastVote.count(outSystemnode) && mapSystemnodesLastVote[outSystemnode] == nBlockHeight) {
        return false;
    }

    //record this masternode voted
    mapSystemnodesLastVote[outSystemnode] = nBlockHeight;
    return true;
}

void SNFillBlockPayee(CMutableTransaction& txNew, int64_t nFees, bool &hasMNPayment)
{
    CBlockIndex* pindexPrev = ::ChainActive().Tip();
    if (!pindexPrev)
        return;
    if(Params().NetworkIDString() == CBaseChainParams::TESTNET && budget.IsBudgetPaymentBlock(pindexPrev->nHeight+1)){
        // Doesn't pay systemnode, miners get the full amount on these blocks
    } else if (IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS) && budget.IsBudgetPaymentBlock(pindexPrev->nHeight + 1)) {
        // Doesn't pay systemnode, miners get the full amount on these blocks
    } else {
        systemnodePayments.FillBlockPayee(txNew, nFees, hasMNPayment);
    }
}

std::string SNGetRequiredPaymentsString(int nBlockHeight)
{
    if(Params().NetworkIDString() == CBaseChainParams::TESTNET && budget.IsBudgetPaymentBlock(nBlockHeight)){
        return budget.GetRequiredPaymentsString(nBlockHeight);    
    }
    if (IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS) && budget.IsBudgetPaymentBlock(nBlockHeight)) {
        return budget.GetRequiredPaymentsString(nBlockHeight);
    } else {
        return systemnodePayments.GetRequiredPaymentsString(nBlockHeight);
    }
}

void CSystemnodePayments::FillBlockPayee(CMutableTransaction& txNew, int64_t nFees, bool &hasMNPayment)
{
    CBlockIndex* pindexPrev = ::ChainActive().Tip();
    if(!pindexPrev) return;

    bool hasPayment = true;
    CScript payee;

    //spork
    if (!systemnodePayments.GetBlockPayee(pindexPrev->nHeight + 1, payee)) {
        //no systemnode detected
        CSystemnode* winningNode = snodeman.GetCurrentSystemNode(1);
        if (winningNode) {
            payee = GetScriptForDestination(PKHash(winningNode->pubkey));
        } else {
            LogPrint(BCLog::SYSTEMNODE, "CreateNewBlock: Failed to detect systemnode to pay\n");
            hasPayment = false;
        }
    }
    CAmount blockValue = GetBlockValue(pindexPrev->nHeight+1, nFees);

    CAmount systemnodePayment = GetSystemnodePayment(pindexPrev->nHeight+1, blockValue);

    // This is already done in masternode
    //txNew.vout[0].nValue = blockValue;

    if (hasPayment) {
        if(txNew.nVersion >= TX_ELE_VERSION){
            txNew.vpout.resize(3);
            // [0] is for miner, [1] masternode, [2] systemnode
            txNew.vpout[2].scriptPubKey = payee;
            txNew.vpout[2].nValue = systemnodePayment;
            txNew.vpout[2].nAsset = Params().GetConsensus().subsidy_asset;
            txNew.vpout[0].nValue -= systemnodePayment;
        }
        else{
            txNew.vout.resize(3);
            // [0] is for miner, [1] masternode, [2] systemnode
            txNew.vout[2].scriptPubKey = payee;
            txNew.vout[2].nValue = systemnodePayment;
            txNew.vout[0].nValue -= systemnodePayment;
        }

        CTxDestination address1;
        ExtractDestination(payee, address1);
        
        LogPrint(BCLog::NET, "Systemnode payment to %s\n", EncodeDestination(address1));
    }
    
    if(!hasMNPayment && hasPayment){
        if(txNew.nVersion >= TX_ELE_VERSION){
            txNew.vpout[1].scriptPubKey = payee;
            txNew.vpout[1].nValue = 0;
            txNew.vpout[1].nAsset = Params().GetConsensus().subsidy_asset;
        }
        else{
            txNew.vout[1].scriptPubKey = payee;
            txNew.vout[1].nValue = 0;
        }	
	}
}

std::string CSystemnodeBlockPayees::GetRequiredPaymentsString()
{
    LOCK(cs_vecPayments);

    std::string ret = "Unknown";

    for (auto& payee : vecPayments)
    {
        CTxDestination address1;
        ExtractDestination(payee.scriptPubKey, address1);

        if(ret != "Unknown") {
            ret += ", " + EncodeDestination(address1) + ":" + boost::lexical_cast<std::string>(payee.nVotes);
        } else {
            ret = EncodeDestination(address1) + ":" + boost::lexical_cast<std::string>(payee.nVotes);
        }
    }

    return ret;
}

std::string CSystemnodePayments::GetRequiredPaymentsString(int nBlockHeight)
{
    LOCK(cs_mapSystemnodeBlocks);

    if (mapSystemnodeBlocks.count(nBlockHeight)) {
        return mapSystemnodeBlocks[nBlockHeight].GetRequiredPaymentsString();
    }

    return "Unknown";
}

bool CSystemnodeBlockPayees::IsTransactionValid(const CTransaction& txNew, const CAmount& nValueCreated)
{
    LOCK(cs_vecPayments);

    int nMaxSignatures = 0;
    std::string strPayeesPossible = "";

    CAmount systemnodePayment = GetSystemnodePayment(nBlockHeight, nValueCreated);

    //require at least 6 signatures

    for (auto& payee : vecPayments)
        if(payee.nVotes >= nMaxSignatures && payee.nVotes >= SNPAYMENTS_SIGNATURES_REQUIRED)
            nMaxSignatures = payee.nVotes;

    // if we don't have at least 6 signatures on a payee, approve whichever is the longest chain
    if(nMaxSignatures < SNPAYMENTS_SIGNATURES_REQUIRED) return true;

    for (auto& payee : vecPayments)
    {
        bool found = false;
        int pos = -1;
        for (unsigned int k = 0; k <  (txNew.nVersion >= TX_ELE_VERSION ? txNew.vpout.size() : txNew.vout.size()) ; k++){
            CTxOutAsset txout = (txNew.nVersion >= TX_ELE_VERSION ? txNew.vpout[k] : txNew.vout[k]);
            if(payee.scriptPubKey == txout.scriptPubKey && systemnodePayment == txout.nValue){
                pos = k;
                found = true;
                break;
            }
        }

        if(payee.nVotes >= SNPAYMENTS_SIGNATURES_REQUIRED){
            if(found) {
                //When proof of stake is active, enforce specific payment positions
                if (nBlockHeight >= Params().PoSStartHeight() && pos != SN_PMT_SLOT)
                    return error("%s: Systemnode payment is not in coinbase.vout[%d]", __func__, SN_PMT_SLOT);
                return true;
            }
            CTxDestination address1;
            ExtractDestination(payee.scriptPubKey, address1);

            if(strPayeesPossible == ""){
                strPayeesPossible += EncodeDestination(address1);
            } else {
                strPayeesPossible += "," + EncodeDestination(address1);
            }
        }
    }

    LogPrint(BCLog::SYSTEMNODE, "CSystemnodePayments::IsTransactionValid - Missing required payment - %s\n", strPayeesPossible.c_str());
    return false;
}

bool CSystemnodePayments::GetBlockPayee(int nBlockHeight, CScript& payee)
{
    if (mapSystemnodeBlocks.count(nBlockHeight)) {
        return mapSystemnodeBlocks[nBlockHeight].GetPayee(payee);
    }

    return false;
}

void CSystemnodePayments::CheckAndRemove()
{
    LOCK2(cs_mapSystemnodePayeeVotes, cs_mapSystemnodeBlocks);

    int nHeight;
    {
        TRY_LOCK(cs_main, locked);
        if (!locked || ::ChainActive().Tip() == nullptr)
            return;
        nHeight = ::ChainActive().Tip()->nHeight;
    }

    //keep up to five cycles for historical sake
    int nLimit = std::max(int(snodeman.size() * 1.25), 1000);

    std::map<uint256, CSystemnodePaymentWinner>::iterator it = mapSystemnodePayeeVotes.begin();
    while (it != mapSystemnodePayeeVotes.end()) {
        CSystemnodePaymentWinner winner = (*it).second;

        if (nHeight - winner.nBlockHeight > nLimit) {
            LogPrint(BCLog::SYSTEMNODE, "CSystemnodePayments::CleanPaymentList - Removing old Systemnode payment - block %d\n", winner.nBlockHeight);
            systemnodeSync.mapSeenSyncSNW.erase((*it).first);
            mapSystemnodePayeeVotes.erase(it++);
            mapSystemnodeBlocks.erase(winner.nBlockHeight);
        } else {
            ++it;
        }
    }
}

bool CSystemnodePaymentWinner::IsValid(CNode* pnode, std::string& strError, CConnman& connman)
{
    if (IsReferenceNode(vinSystemnode))
        return true;

    CSystemnode* psn = snodeman.Find(vinSystemnode);

    if (!psn) {
        strError = strprintf("Unknown Systemnode %s", vinSystemnode.prevout.ToStringShort());
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodePaymentWinner::IsValid - %s\n", strError);
        snodeman.AskForSN(pnode, vinSystemnode, connman);
        return false;
    }

    if (psn->protocolVersion < MIN_MNW_PEER_PROTO_VERSION) {
        strError = strprintf("Systemnode protocol too old %d - req %d", psn->protocolVersion, MIN_MNW_PEER_PROTO_VERSION);
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodePaymentWinner::IsValid - %s\n", strError);
        return false;
    }

    int n = snodeman.GetSystemnodeRank(vinSystemnode, nBlockHeight - 100, MIN_MNW_PEER_PROTO_VERSION);

    if (n > MNPAYMENTS_SIGNATURES_TOTAL) {
        //It's common to have systemnodes mistakenly think they are in the top 10
        // We don't want to print all of these messages, or punish them unless they're way off
        if (n > MNPAYMENTS_SIGNATURES_TOTAL * 2) {
            strError = strprintf("Systemnode not in the top %d (%d)", MNPAYMENTS_SIGNATURES_TOTAL, n);
            LogPrint(BCLog::SYSTEMNODE, "CSystemnodePaymentWinner::IsValid - %s\n", strError);
            if (systemnodeSync.IsSynced())
                Misbehaving(pnode->GetId(), 20);
        }
        return false;
    }

    return true;
}

bool CSystemnodePayments::ProcessBlock(int nBlockHeight, CConnman& connman)
{
    if (!fSystemNode)
        return false;

    //reference node - hybrid mode

    if (!IsReferenceNode(activeSystemnode.vin)) {
        int n = snodeman.GetSystemnodeRank(activeSystemnode.vin, nBlockHeight - 100, MIN_MNW_PEER_PROTO_VERSION);

        if (n == -1) {
            LogPrint(BCLog::SYSTEMNODE, "CSystemnodePayments::ProcessBlock - Unknown Systemnode\n");
            return false;
        }

        if (n > SNPAYMENTS_SIGNATURES_TOTAL) {
            LogPrint(BCLog::SYSTEMNODE, "CSystemnodePayments::ProcessBlock - Systemnode not in the top %d (%d)\n", MNPAYMENTS_SIGNATURES_TOTAL, n);
            return false;
        }
    }

    if (nBlockHeight <= nLastBlockHeight)
        return false;

    CSystemnodePaymentWinner newWinner(activeSystemnode.vin);

    LogPrint(BCLog::SYSTEMNODE, "CSystemnodePayments::ProcessBlock() Start nHeight %d - vin %s. \n", nBlockHeight, activeSystemnode.vin.ToString().c_str());
    // pay to the oldest MN that still had no payment but its input is old enough and it was active long enough
    int nCount = 0;
    CSystemnode* psn = snodeman.GetNextSystemnodeInQueueForPayment(nBlockHeight, true, nCount);

    if (psn) {
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodePayments::ProcessBlock() Found by FindOldestNotInVec \n");

        newWinner.nBlockHeight = nBlockHeight;

        CScript payee = GetScriptForDestination(PKHash(psn->pubkey));
        newWinner.AddPayee(payee);

        LogPrint(BCLog::SYSTEMNODE, "CSystemnodePayments::ProcessBlock() Winner payee %s nHeight %d. \n", payee.ToString(), newWinner.nBlockHeight);
    } else {
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodePayments::ProcessBlock() Failed to find systemnode to pay\n");
    }

    std::string errorMessage;
    CPubKey pubKeySystemnode;
    CKey keySystemnode;

    if (!legacySigner.SetKey(strSystemNodePrivKey, keySystemnode, pubKeySystemnode)) {
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodePayments::ProcessBlock() - Error upon calling SetKey: %s\n", errorMessage.c_str());
        return false;
    }

    LogPrint(BCLog::SYSTEMNODE, "CSystemnodePayments::ProcessBlock() - Signing Winner\n");
    if (newWinner.Sign(keySystemnode, pubKeySystemnode)) {
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodePayments::ProcessBlock() - AddWinningSystemnode\n");

        if (AddWinningSystemnode(newWinner)) {
            newWinner.Relay(connman);
            nLastBlockHeight = nBlockHeight;
            return true;
        }
    }

    return false;
}

bool CSystemnodePayments::AddWinningSystemnode(CSystemnodePaymentWinner& winnerIn)
{
    uint256 blockHash = uint256();
    if (!GetBlockHash(blockHash, winnerIn.nBlockHeight - 100)) {
        return false;
    }

    {
        LOCK2(cs_mapSystemnodePayeeVotes, cs_mapSystemnodeBlocks);

        if (mapSystemnodePayeeVotes.count(winnerIn.GetHash())) {
            return false;
        }

        mapSystemnodePayeeVotes[winnerIn.GetHash()] = winnerIn;

        if (!mapSystemnodeBlocks.count(winnerIn.nBlockHeight)) {
            CSystemnodeBlockPayees blockPayees(winnerIn.nBlockHeight);
            mapSystemnodeBlocks[winnerIn.nBlockHeight] = blockPayees;
        }
    }

    int n = 1;
    if (IsReferenceNode(winnerIn.vinSystemnode))
        n = 100;
    mapSystemnodeBlocks[winnerIn.nBlockHeight].AddPayee(winnerIn.payee, n);

    return true;
}

void CSystemnodePaymentWinner::Relay(CConnman& connman)
{
    CInv inv(MSG_SYSTEMNODE_WINNER, GetHash());
    connman.RelayInv(inv);
}

bool CSystemnodePaymentWinner::SignatureValid()
{

    CSystemnode* psn = snodeman.Find(vinSystemnode);

    if (psn) {
        std::string strMessage = vinSystemnode.prevout.ToStringShort() + boost::lexical_cast<std::string>(nBlockHeight) + payee.ToString();

        std::string errorMessage = "";
        if (!legacySigner.VerifyMessage(psn->pubkey2, vchSig, strMessage, errorMessage)) {
            return error("CSystemnodePaymentWinner::SignatureValid() - Got bad Systemnode address signature %s \n", vinSystemnode.ToString().c_str());
        }

        return true;
    }

    return false;
}

void CSystemnodePayments::Sync(CNode* node, int nCountNeeded, CConnman& connman)
{
    LOCK(cs_mapSystemnodePayeeVotes);

    int nHeight;
    {
        TRY_LOCK(cs_main, locked);
        if (!locked || ::ChainActive().Tip() == nullptr)
            return;
        nHeight = ::ChainActive().Tip()->nHeight;
    }

    int nCount = (snodeman.CountEnabled() * 1.25);
    if (nCountNeeded > nCount)
        nCountNeeded = nCount;

    int nInvCount = 0;
    std::map<uint256, CSystemnodePaymentWinner>::iterator it = mapSystemnodePayeeVotes.begin();
    while (it != mapSystemnodePayeeVotes.end()) {
        CSystemnodePaymentWinner winner = (*it).second;
        if (winner.nBlockHeight >= nHeight - nCountNeeded && winner.nBlockHeight <= nHeight + 20) {
            node->PushInventory(CInv(MSG_SYSTEMNODE_WINNER, winner.GetHash()));
            nInvCount++;
        }
        ++it;
    }

    const CNetMsgMaker msgMaker(PROTOCOL_VERSION);
    connman.PushMessage(node, msgMaker.Make("snssc", SYSTEMNODE_SYNC_SNW, nInvCount));
}

// Is this systemnode scheduled to get paid soon?
// -- Only look ahead up to 8 blocks to allow for propagation of the latest 2 winners
bool CSystemnodePayments::IsScheduled(CSystemnode& sn, int nNotBlockHeight)
{
    LOCK(cs_mapSystemnodeBlocks);

    int nHeight;
    {
        TRY_LOCK(cs_main, locked);
        if (!locked || ::ChainActive().Tip() == nullptr)
            return false;
        nHeight = ::ChainActive().Tip()->nHeight;
    }

    CScript snpayee;
    snpayee = GetScriptForDestination(PKHash(sn.pubkey));

    CScript payee;
    for (int64_t h = nHeight; h <= nHeight + 8; h++) {
        if (h == nNotBlockHeight)
            continue;
        if (mapSystemnodeBlocks.count(h)) {
            if (mapSystemnodeBlocks[h].GetPayee(payee)) {
                if (snpayee == payee) {
                    return true;
                }
            }
        }
    }

    return false;
}

std::string CSystemnodePayments::ToString() const
{
    std::ostringstream info;

    info << "Votes: " << (int)mapSystemnodePayeeVotes.size() << ", Blocks: " << (int)mapSystemnodeBlocks.size();

    return info.str();
}

bool CSystemnodePaymentWinner::Sign(CKey& keySystemnode, CPubKey& pubKeySystemnode)
{
    std::string errorMessage;
    std::string strSystemNodeSignMessage;

    std::string strMessage = vinSystemnode.prevout.ToStringShort() + boost::lexical_cast<std::string>(nBlockHeight) + payee.ToString();

    if (!legacySigner.SignMessage(strMessage, vchSig, keySystemnode)) {
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodePing::Sign() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    if (!legacySigner.VerifyMessage(pubKeySystemnode, vchSig, strMessage, errorMessage)) {
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodePing::Sign() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    return true;
}

// Copyright (c) 2009-2015 The Dash developers
// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <consensus/validation.h>
#include <crown/instantx.h>
#include <crown/legacycalls.h>
#include <crown/legacysigner.h>
#include <crown/spork.h>
#include <key.h>
#include <masternode/activemasternode.h>
#include <masternode/masternodeman.h>
#include <net.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <node/context.h>
#include <mn_processing.h>
#include <protocol.h>
#include <rpc/blockchain.h>
#include <wallet/wallet.h>
#include <boost/lexical_cast.hpp>


CInstantSend instantSend;

//step 1.) Broadcast intention to lock transaction inputs, "txlreg", CTransaction
//step 2.) Top INSTANTX_SIGNATURES_TOTAL masternodes, open connect to top 1 masternode.
//         Send "txvote", CTransaction, Signature, Approve
//step 3.) Top 1 masternode, waits for INSTANTX_SIGNATURES_REQUIRED messages. Upon success, sends "txlock'

void CInstantSend::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman)
{
    if (!IsSporkActive(SPORK_2_INSTANTX))
        return;
    if (!masternodeSync.IsBlockchainSynced())
        return;

    //! instantsend
    if (strCommand == NetMsgType::IX) {
        CMutableTransaction tx;
        vRecv >> tx;

        uint256 txHash = tx.GetHash();
        CInv inv(MSG_TXLOCK_REQUEST, txHash);
        pfrom->AddInventoryKnown(inv);

        if (mapTxLockReq.count(txHash) || mapTxLockReqRejected.count(txHash))
            return;

        if (!IsIxTxValid(MakeTransactionRef(tx)))
            return;

        // Check if transaction is old for lock
        if (GetTransactionAge(txHash) > m_acceptedBlockCount)
            return;

        int64_t nBlockHeight = CreateNewLock(tx);
        if (nBlockHeight == 0)
            return;

        TxValidationState state;
        bool fAccepted = false;
        {
            LOCK(cs_main);
            fAccepted = AcceptToMemoryPool(*g_rpc_node->mempool, state, MakeTransactionRef(tx), nullptr, true);
        }

        if (fAccepted) {
            connman->RelayInv(inv);

            DoConsensusVote(tx, nBlockHeight, *connman);

            mapTxLockReq.insert(std::make_pair(txHash, tx));

            LogPrintf("ProcessMessageInstantX::ix - Transaction Lock Request: %s %s : accepted %s\n",
                pfrom->addr.ToString().c_str(), pfrom->cleanSubVer.c_str(),
                txHash.ToString().c_str());

            return;

        } else {
            mapTxLockReqRejected.insert(std::make_pair(txHash, tx));

            // can we get the conflicting transaction as proof?

            LogPrintf("ProcessMessageInstantX::ix - Transaction Lock Request: %s %s : rejected %s\n",
                pfrom->addr.ToString().c_str(), pfrom->cleanSubVer.c_str(),
                txHash.ToString().c_str());

            for (const auto& in : tx.vin) {
                if (!mapLockedInputs.count(in.prevout)) {
                    mapLockedInputs.insert(std::make_pair(in.prevout, txHash));
                }
            }

            // resolve conflicts
            std::map<uint256, CTransactionLock>::iterator i = mapTxLocks.find(txHash);
            if (i != mapTxLocks.end()) {
                //we only care if we have a complete tx lock
                if ((*i).second.CountSignatures() >= INSTANTX_SIGNATURES_REQUIRED) {
                    if (!CheckForConflictingLocks(tx)) {
                        LogPrintf("ProcessMessageInstantX::ix - Found Existing Complete IX Lock\n");

                        //reprocess the last 15 blocks
                        ReprocessBlocks(15);
                        mapTxLockReq.insert(std::make_pair(txHash, tx));
                    }
                }
            }

            return;
        }
    }

    //! instantx lock vote
    if (strCommand == NetMsgType::IXLOCKVOTE) {

        CConsensusVote ctx;
        vRecv >> ctx;

        uint256 ctxHash = ctx.GetHash();
        CInv inv(MSG_TXLOCK_VOTE, ctxHash);
        pfrom->AddInventoryKnown(inv);

        if (mapTxLockVote.find(ctx.GetHash()) != mapTxLockVote.end())
            return;

        // Check if transaction is old for lock
        if (GetTransactionAge(ctx.txHash) > m_acceptedBlockCount)
        {
            LogPrintf("InstantSend::ProcessMessage - Old transaction lock request is received. TxId - %s\n", ctx.txHash.ToString());
            return;
        }

        mapTxLockVote.insert(std::pair(ctx.GetHash(), ctx));

        if (ProcessConsensusVote(pfrom, ctx, *connman)) {
            /*
                Masternodes will sometimes propagate votes before the transaction is known to the client.
                This tracks those messages and allows it at the same rate of the rest of the network, if
                a peer violates it, it will simply be ignored
            */
            if (!mapTxLockReq.count(ctx.txHash) && !mapTxLockReqRejected.count(ctx.txHash)) {
                if (!mapUnknownVotes.count(ctx.vinMasternode.prevout.hash)) {
                    mapUnknownVotes[ctx.vinMasternode.prevout.hash] = GetTime() + (60 * 10);
                }

                if(mapUnknownVotes[ctx.vinMasternode.prevout.hash] > GetTime() &&
                    mapUnknownVotes[ctx.vinMasternode.prevout.hash] - GetAverageVoteTime() > 60*10){
                        LogPrintf("ProcessMessageInstantX::ix - masternode is spamming transaction votes: %s %s\n",
                            ctx.vinMasternode.ToString().c_str(),
                            ctx.txHash.ToString().c_str()
                        );
                        return;
                } else {
                    mapUnknownVotes[ctx.vinMasternode.prevout.hash] = GetTime()+(60*10);
                }
            }
            connman->RelayInv(inv);
        }
        return;
    }

    //! instantx lock list
    if (strCommand == NetMsgType::IXLOCKLIST) {
        std::map<uint256, CConsensusVote>::const_iterator it = mapTxLockVote.begin();
        for (; it != mapTxLockVote.end(); ++it) {
            CInv inv(MSG_TXLOCK_VOTE, it->second.GetHash());
            pfrom->AddInventoryKnown(inv);

            connman->RelayInv(inv);
        }
    }
}

bool CInstantSend::IsIxTxValid(const CTransactionRef& txCollateral) const
{
    if(txCollateral->nVersion >= TX_ELE_VERSION){
        if (txCollateral->vpout.size() < 1) return false;
    }else{
        if (txCollateral->vout.size() < 1) return false;
	}

    if (txCollateral->nLockTime != 0)
        return false;

    int64_t nValueIn = 0;
    int64_t nValueOut = 0;
    bool missingTx = false;

    for (unsigned int k = 0; k <  (txCollateral->nVersion >= TX_ELE_VERSION ? txCollateral->vpout.size() : txCollateral->vout.size()) ; k++){
        CTxOutAsset o = (txCollateral->nVersion >= TX_ELE_VERSION ? txCollateral->vpout[k] : txCollateral->vout[k]);
        nValueOut += o.nValue;
    }

    uint256 hash;
    for (const CTxIn& i : txCollateral->vin) {
        CTransactionRef tx2 = GetTransaction(::ChainActive().Tip(), nullptr, i.prevout.hash, Params().GetConsensus(), hash);
        if (!tx2) {
            missingTx = true;
        } else {
            if (tx2->nVersion >= TX_ELE_VERSION && tx2->vpout.size() > i.prevout.n) {
                nValueIn += tx2->vpout[i.prevout.n].nValue;
            }
            else if(tx2->nVersion < TX_ELE_VERSION && tx2->vout.size() > i.prevout.n){
                nValueIn += tx2->vout[i.prevout.n].nValue;
            } else {
                missingTx = true;
            }
        }
    }

    if (nValueOut > GetSporkValue(SPORK_5_MAX_VALUE) * COIN) {
        LogPrintf("IsIxTxValid - Transaction value too high - %s\n", txCollateral->GetHash().ToString().c_str());
        return false;
    }

    if (missingTx) {
        LogPrintf("IsIxTxValid - Unknown inputs in IX transaction - %s\n", txCollateral->GetHash().ToString().c_str());
        /*
            This happens sometimes for an unknown reason, so we'll return that it's a valid transaction.
            If someone submits an invalid transaction it will be rejected by the network anyway and this isn't
            very common, but we don't want to block IX just because the client can't figure out the fee.
        */
        return true;
    }

    return true;
}

int64_t CInstantSend::CreateNewLock(const CMutableTransaction& tx)
{
    LOCK(cs);

    int64_t nTxAge = 0;
    uint256 txHash = tx.GetHash();
    for (const auto& i : tx.vin) {
        nTxAge = GetUTXOConfirmations(i.prevout);
        if (nTxAge < 5) //1 less than the "send IX" gui requires, incase of a block propagating the network at the time
        {
            LogPrintf("CreateNewLock - Transaction not found / too new: %d / %s\n", nTxAge, txHash.ToString().c_str());
            return 0;
        }
    }

    /*
        Use a blockheight newer than the input.
        This prevents attackers from using transaction mallibility to predict which masternodes
        they'll use.
    */
    int nBlockHeight = (::ChainActive().Tip()->nHeight - nTxAge) + 4;

    if (!mapTxLocks.count(txHash)) {
        LogPrintf("CreateNewLock - New Transaction Lock %s !\n", txHash.ToString().c_str());

        CTransactionLock newLock;
        newLock.nBlockHeight = nBlockHeight;
        newLock.txHash = txHash;
        mapTxLocks.insert(std::pair(txHash, newLock));
    } else {
        mapTxLocks[txHash].nBlockHeight = nBlockHeight;
        LogPrintf("CreateNewLock - Transaction Lock Exists %s !\n", txHash.ToString().c_str());
    }

    mapTxLockReq.insert(std::make_pair(txHash, tx));
    return nBlockHeight;
}

// check if we need to vote on this transaction
void CInstantSend::DoConsensusVote(const CMutableTransaction& tx, int64_t nBlockHeight, CConnman& connman)
{
    LOCK(cs);
    if (!fMasterNode)
        return;

    int n = mnodeman.GetMasternodeRank(activeMasternode.vin, nBlockHeight, MIN_INSTANTX_PROTO_VERSION);

    if (n == -1) {
        LogPrintf("InstantX::DoConsensusVote - Unknown Masternode\n");
        return;
    }

    if (n > INSTANTX_SIGNATURES_TOTAL) {
        LogPrintf("InstantX::DoConsensusVote - Masternode not in the top %d (%d)\n", INSTANTX_SIGNATURES_TOTAL, n);
        return;
    }
    /*
        nBlockHeight calculated from the transaction is the authoritive source
    */

    LogPrintf("InstantX::DoConsensusVote - In the top %d (%d)\n", INSTANTX_SIGNATURES_TOTAL, n);

    CConsensusVote ctx;
    ctx.vinMasternode = activeMasternode.vin;
    ctx.txHash = tx.GetHash();
    ctx.nBlockHeight = nBlockHeight;
    if (!ctx.Sign()) {
        LogPrintf("InstantX::DoConsensusVote - Failed to sign consensus vote\n");
        return;
    }
    if (!ctx.SignatureValid()) {
        LogPrintf("InstantX::DoConsensusVote - Signature invalid\n");
        return;
    }

    uint256 ctxHash = ctx.GetHash();
    mapTxLockVote[ctxHash] = ctx;

    CInv inv(MSG_TXLOCK_VOTE, ctxHash);
    connman.RelayInv(inv);
}

//received a consensus vote
bool CInstantSend::ProcessConsensusVote(CNode* pnode, const CConsensusVote& ctx, CConnman& connman)
{
    LOCK(cs);
    uint256 ctxHash = ctx.GetHash();
    int n = mnodeman.GetMasternodeRank(ctx.vinMasternode, ctx.nBlockHeight, MIN_INSTANTX_PROTO_VERSION);

    CMasternode* pmn = mnodeman.Find(ctx.vinMasternode);
    if (pmn)
        LogPrintf("InstantX::ProcessConsensusVote - Masternode ADDR %s %d\n", pmn->addr.ToString().c_str(), n);

    if (n == -1) {
        //can be caused by past versions trying to vote with an invalid protocol
        LogPrintf("InstantX::ProcessConsensusVote - Unknown Masternode\n");
        mnodeman.AskForMN(pnode, ctx.vinMasternode, connman);
        return false;
    }

    if (n > INSTANTX_SIGNATURES_TOTAL) {
        LogPrintf("InstantX::ProcessConsensusVote - Masternode not in the top %d (%d) - %s\n", INSTANTX_SIGNATURES_TOTAL, n, ctxHash.ToString().c_str());
        return false;
    }

    if (!ctx.SignatureValid()) {
        LogPrintf("InstantX::ProcessConsensusVote - Signature invalid\n");
        // don't ban, it could just be a non-synced masternode
        mnodeman.AskForMN(pnode, ctx.vinMasternode, connman);
        return false;
    }

    if (!mapTxLocks.count(ctx.txHash)) {
        LogPrintf("InstantX::ProcessConsensusVote - New Transaction Lock %s !\n", ctx.txHash.ToString().c_str());

        CTransactionLock newLock;
        newLock.nBlockHeight = 0;
        newLock.txHash = ctx.txHash;
        mapTxLocks.insert(std::make_pair(ctx.txHash, newLock));
    } else
        LogPrintf("InstantX::ProcessConsensusVote - Transaction Lock Exists %s !\n", ctx.txHash.ToString().c_str());

    //compile consessus vote
    std::map<uint256, CTransactionLock>::iterator i = mapTxLocks.find(ctx.txHash);
    if (i != mapTxLocks.end()) {
        (*i).second.AddSignature(ctx);


        LogPrintf("InstantX::ProcessConsensusVote - Transaction Lock Votes %d - %s !\n", (*i).second.CountSignatures(), ctxHash.ToString().c_str());

        if ((*i).second.CountSignatures() >= INSTANTX_SIGNATURES_REQUIRED) {
            LogPrintf("InstantX::ProcessConsensusVote - Transaction Lock Is Complete \n");
            LogPrintf("InstantX::ProcessConsensusVote - Transaction Lock Is Complete %s !\n", (*i).second.GetHash().ToString().c_str());

            CMutableTransaction& tx = mapTxLockReq[ctx.txHash];
            if (!CheckForConflictingLocks(tx)) {

                if (mapTxLockReq.count(ctx.txHash)) {
                    for (const auto& in : tx.vin) {
                        if (!mapLockedInputs.count(in.prevout)) {
                            mapLockedInputs.insert(std::make_pair(in.prevout, ctx.txHash));
                        }
                    }
                }

                // resolve conflicts

                //if this tx lock was rejected, we need to remove the conflicting blocks
                if (mapTxLockReqRejected.count((*i).second.txHash)) {
                    //reprocess the last 15 blocks
                    ReprocessBlocks(15);
                }
            }
        }
        return true;
    }

    return false;
}

bool CInstantSend::CheckForConflictingLocks(const CMutableTransaction& tx)
{
    LOCK(cs);
    /*
        It's possible (very unlikely though) to get 2 conflicting transaction locks approved by the network.
        In that case, they will cancel each other out.

        Blocks could have been rejected during this time, which is OK. After they cancel out, the client will
        rescan the blocks and find they're acceptable and then take the chain with the most work.
    */
    uint256 txHash = tx.GetHash();
    for (const auto& in : tx.vin) {
        if (mapLockedInputs.count(in.prevout)) {
            if (mapLockedInputs[in.prevout] != txHash) {
                LogPrintf("InstantX::CheckForConflictingLocks - found two complete conflicting locks - removing both. %s %s",
                    txHash.ToString().c_str(), mapLockedInputs[in.prevout].ToString().c_str());
                if (mapTxLocks.count(txHash))
                    mapTxLocks[txHash].m_expiration = GetTime();
                if (mapTxLocks.count(mapLockedInputs[in.prevout]))
                    mapTxLocks[mapLockedInputs[in.prevout]].m_expiration = GetTime();
                return true;
            }
        }
    }
    return false;
}

int64_t CInstantSend::GetAverageVoteTime() const
{
    std::map<uint256, int64_t>::const_iterator it = mapUnknownVotes.begin();
    int64_t total = 0;
    int64_t count = 0;

    while (it != mapUnknownVotes.end()) {
        total += it->second;
        count++;
        it++;
    }

    return total / count;
}

void CInstantSend::CheckAndRemove()
{
    LOCK(cs);
    if (!::ChainActive().Tip())
        return;

    std::map<uint256, CTransactionLock>::iterator it = mapTxLocks.begin();

    while (it != mapTxLocks.end()) {
        if (GetTime() > it->second.m_expiration) {
            LogPrintf("Removing old transaction lock %s\n", it->second.txHash.ToString().c_str());

            // Remove rejected transaction if expired
            mapTxLockReqRejected.erase(it->second.txHash);

            std::map<uint256, CMutableTransaction>::iterator itLock = mapTxLockReq.find(it->second.txHash);
            if (itLock != mapTxLockReq.end()) {
                CMutableTransaction& tx = itLock->second;

                for (const auto& in : tx.vin)
                    mapLockedInputs.erase(in.prevout);

                mapTxLockReq.erase(it->second.txHash);

                for (const auto& v : it->second.vecConsensusVotes)
                    mapTxLockVote.erase(v.GetHash());
            }
            mapTxLocks.erase(it++);
        } else {
            it++;
        }
    }

    std::map<uint256, CConsensusVote>::iterator itVote = mapTxLockVote.begin();
    while (itVote != mapTxLockVote.end()) {
        if (GetTime() > it->second.m_expiration || GetTransactionAge(it->second.txHash) > CInstantSend::nCompleteTXLocks) {
            // Remove transaction vote if it is expired or belongs to old transaction
            mapTxLockVote.erase(itVote++);
        } else {
            ++itVote;
        }
    }
}

int CInstantSend::GetSignaturesCount(uint256 txHash) const
{
    std::map<uint256, CTransactionLock>::const_iterator i = mapTxLocks.find(txHash);
    if (i != mapTxLocks.end()) {
        return (*i).second.CountSignatures();
    }
    return -1;
}

bool CInstantSend::IsLockTimedOut(uint256 txHash) const
{
    std::map<uint256, CTransactionLock>::const_iterator i = mapTxLocks.find(txHash);
    if (i != mapTxLocks.end()) {
        return GetTime() > (*i).second.m_timeout;
    }
    return false;
}

bool CInstantSend::TxLockRequested(uint256 txHash) const
{
    return mapTxLockReq.count(txHash) || mapTxLockReqRejected.count(txHash);
}

bool CInstantSend::AlreadyHave(uint256 txHash) const
{
    return mapTxLockVote.find(txHash) != mapTxLockVote.end();
}

std::string CInstantSend::ToString() const
{
    std::ostringstream info;

    info << "Transaction lock requests: " << mapTxLockReq.size() << ", Transaction locks: " << mapTxLocks.size() << ", Locked Inputs: " << mapLockedInputs.size() << ", Transaction lock votes: " << mapTxLockVote.size();

    return info.str();
}

void CInstantSend::Clear()
{
    LOCK(cs);
    mapLockedInputs.clear();
    mapTxLockVote.clear();
    mapTxLockReq.clear();
    mapTxLocks.clear();
    mapUnknownVotes.clear();
    mapTxLockReqRejected.clear();
}

int CInstantSend::GetCompleteLocksCount() const
{
    return nCompleteTXLocks;
}

uint256 CConsensusVote::GetHash() const
{
    return ArithToUint256(UintToArith256(vinMasternode.prevout.hash) + vinMasternode.prevout.n + UintToArith256(txHash));
}

bool CConsensusVote::SignatureValid() const
{
    std::string errorMessage;
    std::string strMessage = txHash.ToString().c_str() + boost::lexical_cast<std::string>(nBlockHeight);
    //LogPrintf("verify strMessage %s \n", strMessage.c_str());

    CMasternode* pmn = mnodeman.Find(vinMasternode);

    if (!pmn) {
        LogPrintf("InstantX::CConsensusVote::SignatureValid() - Unknown Masternode\n");
        return false;
    }

    if (!legacySigner.VerifyMessage(pmn->pubkey2, vchMasterNodeSignature, strMessage, errorMessage)) {
        LogPrintf("InstantX::CConsensusVote::SignatureValid() - Verify message failed\n");
        return false;
    }

    return true;
}

bool CConsensusVote::Sign()
{
    std::string errorMessage;

    CKey key2;
    CPubKey pubkey2;
    std::string strMessage = txHash.ToString().c_str() + boost::lexical_cast<std::string>(nBlockHeight);
    //LogPrintf("signing strMessage %s \n", strMessage.c_str());
    //LogPrintf("signing privkey %s \n", strMasterNodePrivKey.c_str());

    if (!legacySigner.SetKey(strMasterNodePrivKey, key2, pubkey2)) {
        LogPrintf("CConsensusVote::Sign() - ERROR: Invalid masternodeprivkey: '%s'\n", errorMessage.c_str());
        return false;
    }

    if (!legacySigner.SignMessage(strMessage, vchMasterNodeSignature, key2)) {
        LogPrintf("CConsensusVote::Sign() - Sign message failed");
        return false;
    }

    if (!legacySigner.VerifyMessage(pubkey2, vchMasterNodeSignature, strMessage, errorMessage)) {
        LogPrintf("CConsensusVote::Sign() - Verify message failed");
        return false;
    }

    return true;
}

bool CTransactionLock::SignaturesValid() const
{

    for (const auto& vote : vecConsensusVotes) {
        int n = mnodeman.GetMasternodeRank(vote.vinMasternode, vote.nBlockHeight, MIN_INSTANTX_PROTO_VERSION);

        if (n == -1) {
            LogPrintf("CTransactionLock::SignaturesValid() - Unknown Masternode\n");
            return false;
        }

        if (n > INSTANTX_SIGNATURES_TOTAL) {
            LogPrintf("CTransactionLock::SignaturesValid() - Masternode not in the top %s\n", INSTANTX_SIGNATURES_TOTAL);
            return false;
        }

        if (!vote.SignatureValid()) {
            LogPrintf("CTransactionLock::SignaturesValid() - Signature not valid\n");
            return false;
        }
    }

    return true;
}

void CTransactionLock::AddSignature(const CConsensusVote& cv)
{
    vecConsensusVotes.push_back(cv);
}

int CTransactionLock::CountSignatures() const
{
    /*
        Only count signatures where the BlockHeight matches the transaction's blockheight.
        The votes have no proof it's the correct blockheight
    */

    if (nBlockHeight == 0)
        return -1;

    int n = 0;
    for (const auto& v : vecConsensusVotes) {
        if (v.nBlockHeight == nBlockHeight) {
            n++;
        }
    }
    return n;
}

uint256 CTransactionLock::GetHash() const
{
    return txHash;
}

void RelayTransactionLockReq(CTransactionRef& tx)
{
    const CNetMsgMaker msgMaker(PROTOCOL_VERSION);
    CInv inv(MSG_TXLOCK_REQUEST, tx->GetHash());
    for (auto& pnode : g_rpc_node->connman->CopyNodeVector()) {
        if (!pnode->m_tx_relay->fRelayTxes)
            continue;
        g_rpc_node->connman->PushMessage(pnode, msgMaker.Make(NetMsgType::IX, tx));
    }
}

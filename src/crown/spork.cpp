#include <base58.h>
#include <consensus/validation.h>
#include <crown/spork.h>
#include <key.h>
#include <masternode/masternode-budget.h>
#include <net.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <node/context.h>
#include <protocol.h>
#include <rpc/blockchain.h>
#include <boost/lexical_cast.hpp>


class CSporkMessage;
class CSporkManager;

CSporkManager sporkManager;
std::map<uint256, CSporkMessage> mapSporks;
std::map<int, CSporkMessage> mapSporksActive;

//! forward declaration (only gets used once here)
void UpdateMempoolForReorg(CTxMemPool& mempool, DisconnectedBlockTransactions& disconnectpool, bool fAddToMempool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, mempool.cs);

void ProcessSpork(CNode* pfrom, CConnman* connman, const std::string& strCommand, CDataStream& vRecv)
{
    if (strCommand == NetMsgType::SPORK) {
        CDataStream vMsg(vRecv);
        CSporkMessage spork;
        vRecv >> spork;

        if (::ChainActive().Tip() == NULL)
            return;

        uint256 hash = spork.GetHash();
        if (mapSporksActive.count(spork.nSporkID)) {
            if (mapSporksActive[spork.nSporkID].nTimeSigned >= spork.nTimeSigned) {
                LogPrintf("spork - seen %s block %d \n", hash.ToString(), ::ChainActive().Tip()->nHeight);
                return;
            } else {
                LogPrintf("spork - got updated spork %s block %d \n", hash.ToString(), ::ChainActive().Tip()->nHeight);
            }
        }

        LogPrintf("spork - new %s ID %d Time %d bestHeight %d\n", hash.ToString(), spork.nSporkID, spork.nValue, ::ChainActive().Tip()->nHeight);

        if (!sporkManager.CheckSignature(spork)) {
            LogPrintf("spork - invalid signature\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        mapSporks[hash] = spork;
        mapSporksActive[spork.nSporkID] = spork;
        sporkManager.Relay(spork, *connman);

        //does a task if needed
        ExecuteSpork(spork.nSporkID, spork.nValue, *connman);
    }
    if (strCommand == NetMsgType::GETSPORKS) {
        std::map<int, CSporkMessage>::iterator it = mapSporksActive.begin();

        while (it != mapSporksActive.end()) {
            const CNetMsgMaker msgMaker(PROTOCOL_VERSION);
            connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::SPORK, it->second));
            it++;
        }
    }
}

// grab the spork, otherwise say it's off
bool IsSporkActive(int nSporkID)
{
    int64_t r = -1;

    if (mapSporksActive.count(nSporkID)) {
        r = mapSporksActive[nSporkID].nValue;
    } else {
        if (nSporkID == SPORK_2_INSTANTX)
            r = SPORK_2_INSTANTX_DEFAULT;
        if (nSporkID == SPORK_3_INSTANTX_BLOCK_FILTERING)
            r = SPORK_3_INSTANTX_BLOCK_FILTERING_DEFAULT;
        if (nSporkID == SPORK_4_ENABLE_MASTERNODE_PAYMENTS)
            r = SPORK_4_ENABLE_MASTERNODE_PAYMENTS_DEFAULT;
        if (nSporkID == SPORK_5_MAX_VALUE)
            r = SPORK_5_MAX_VALUE_DEFAULT;
        if (nSporkID == SPORK_7_MASTERNODE_SCANNING)
            r = SPORK_7_MASTERNODE_SCANNING_DEFAULT;
        if (nSporkID == SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT)
            r = SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT_DEFAULT;
        if (nSporkID == SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT)
            r = SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT_DEFAULT;
        if (nSporkID == SPORK_10_MASTERNODE_DONT_PAY_OLD_NODES)
            r = SPORK_10_MASTERNODE_DONT_PAY_OLD_NODES_DEFAULT;
        if (nSporkID == SPORK_11_RESET_BUDGET)
            r = SPORK_11_RESET_BUDGET_DEFAULT;
        if (nSporkID == SPORK_12_RECONSIDER_BLOCKS)
            r = SPORK_12_RECONSIDER_BLOCKS_DEFAULT;
        if (nSporkID == SPORK_13_ENABLE_SUPERBLOCKS)
            r = SPORK_13_ENABLE_SUPERBLOCKS_DEFAULT;
        if (nSporkID == SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT)
            r = SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT_DEFAULT;
        if (nSporkID == SPORK_15_SYSTEMNODE_DONT_PAY_OLD_NODES)
            r = SPORK_15_SYSTEMNODE_DONT_PAY_OLD_NODES_DEFAULT;
        if (nSporkID == SPORK_16_DISCONNECT_OLD_NODES)
            r = SPORK_16_DISCONNECT_OLD_NODES_DEFAULT;
        if (nSporkID == SPORK_17_NFT_TX)
            r = SPORK_17_NFT_TX_DEFAULT;

        if (r == -1)
            LogPrintf("GetSpork::Unknown Spork %d\n", nSporkID);
    }
    if (r == -1)
        r = 4070908800; //return 2099-1-1 by default

    return r < GetTime();
}

// grab the value of the spork on the network, or the default
int64_t GetSporkValue(int nSporkID)
{
    int64_t r = -1;

    if (mapSporksActive.count(nSporkID)) {
        r = mapSporksActive[nSporkID].nValue;
    } else {
        if (nSporkID == SPORK_2_INSTANTX)
            r = SPORK_2_INSTANTX_DEFAULT;
        if (nSporkID == SPORK_3_INSTANTX_BLOCK_FILTERING)
            r = SPORK_3_INSTANTX_BLOCK_FILTERING_DEFAULT;
        if (nSporkID == SPORK_4_ENABLE_MASTERNODE_PAYMENTS)
            r = SPORK_4_ENABLE_MASTERNODE_PAYMENTS_DEFAULT;
        if (nSporkID == SPORK_5_MAX_VALUE)
            r = SPORK_5_MAX_VALUE_DEFAULT;
        if (nSporkID == SPORK_7_MASTERNODE_SCANNING)
            r = SPORK_7_MASTERNODE_SCANNING_DEFAULT;
        if (nSporkID == SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT)
            r = SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT_DEFAULT;
        if (nSporkID == SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT)
            r = SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT_DEFAULT;
        if (nSporkID == SPORK_10_MASTERNODE_DONT_PAY_OLD_NODES)
            r = SPORK_10_MASTERNODE_DONT_PAY_OLD_NODES_DEFAULT;
        if (nSporkID == SPORK_11_RESET_BUDGET)
            r = SPORK_11_RESET_BUDGET_DEFAULT;
        if (nSporkID == SPORK_12_RECONSIDER_BLOCKS)
            r = SPORK_12_RECONSIDER_BLOCKS_DEFAULT;
        if (nSporkID == SPORK_13_ENABLE_SUPERBLOCKS)
            r = SPORK_13_ENABLE_SUPERBLOCKS_DEFAULT;
        if (nSporkID == SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT)
            r = SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT_DEFAULT;
        if (nSporkID == SPORK_15_SYSTEMNODE_DONT_PAY_OLD_NODES)
            r = SPORK_15_SYSTEMNODE_DONT_PAY_OLD_NODES_DEFAULT;
        if (nSporkID == SPORK_16_DISCONNECT_OLD_NODES)
            r = SPORK_16_DISCONNECT_OLD_NODES_DEFAULT;
        if (nSporkID == SPORK_17_NFT_TX)
            r = SPORK_17_NFT_TX_DEFAULT;

        if (r == -1)
            LogPrintf("GetSpork::Unknown Spork %d\n", nSporkID);
    }

    return r;
}

void ExecuteSpork(int nSporkID, int nValue, CConnman& connman)
{
    if (nSporkID == SPORK_11_RESET_BUDGET && nValue == 1) {
        budget.Clear();
    } else if (nSporkID == SPORK_12_RECONSIDER_BLOCKS && nValue > 0) {
        //correct fork via spork technology
        LogPrintf("Spork::ExecuteSpork -- Reconsider Last %d Blocks\n", nValue);
        ReprocessBlocks(nValue);
    }
}

bool DisconnectBlocks(int blocks)
{
    LOCK(cs_main);

    BlockValidationState state;
    const CChainParams& chainparams = Params();
    LogPrintf("DisconnectBlocks -- Got command to replay %d blocks\n", blocks);
    for (int i = 0; i < blocks; i++) {
        DisconnectedBlockTransactions disconnectpool;
        if (!::ChainstateActive().DisconnectTip(state, chainparams, &disconnectpool) || !state.IsValid()) {
            // This is likely a fatal error, but keep the mempool consistent,
            // just in case. Only remove from the mempool in this case.
            UpdateMempoolForReorg(*g_rpc_node->mempool, disconnectpool, false);
            return false;
        }
    }
    return true;
}

void ReprocessBlocks(int nBlocks)
{
    LOCK(cs_main);

    std::map<uint256, int64_t>::iterator it = mapRejectedBlocks.begin();
    while (it != mapRejectedBlocks.end()) {
        //use a window twice as large as is usual for the nBlocks we want to reset
        if ((*it).second > GetTime() - (nBlocks * 60 * 5)) {
            BlockMap::iterator mi = g_chainman.BlockIndex().find((*it).first);
            if (mi != g_chainman.BlockIndex().end() && (*mi).second) {
                CBlockIndex* pindex = (*mi).second;
                LogPrintf("ReprocessBlocks -- %s\n", (*it).first.ToString());
                ResetBlockFailureFlags(pindex);
            }
        }
        ++it;
    }

    DisconnectBlocks(nBlocks);
    BlockValidationState state;
    ActivateBestChain(state, Params(), std::shared_ptr<const CBlock>());
}

bool CSporkManager::CheckSignature(CSporkMessage& spork)
{
    CPubKey pubkey(ParseHex(Params().SporkKey()));
    std::string strMessage = std::to_string(spork.nSporkID) + std::to_string(spork.nValue) + std::to_string(spork.nTimeSigned);
    std::string strError = "";

    if (!legacySigner.VerifyMessage(pubkey, spork.vchSig, strMessage, strError)) {
        LogPrintf("CSporkMessage::CheckSignature -- VerifyHash() failed, error: %s\n", strError);
        return false;
    }

    return true;
}

bool CSporkManager::Sign(CSporkMessage& spork)
{
    std::string strMessage = boost::lexical_cast<std::string>(spork.nSporkID) + boost::lexical_cast<std::string>(spork.nValue) + boost::lexical_cast<std::string>(spork.nTimeSigned);

    CKey key2;
    CPubKey pubkey2;
    std::string errorMessage = "";
    std::string privk = strMasterPrivKey;
    
    if (privk == "" && gArgs.IsArgSet("-sporkkey")) // spork priv key
		privk = gArgs.GetArg("-sporkkey", "");
  
    if (!legacySigner.SetKey(privk, key2, pubkey2)) {
        LogPrintf("CMasternodePayments::Sign - ERROR: Invalid masternodeprivkey: '%s'\n", errorMessage);
        return false;
    }

    if (!legacySigner.SignMessage(strMessage, spork.vchSig, key2)) {
        LogPrintf("CMasternodePayments::Sign - Sign message failed");
        return false;
    }

    if(!legacySigner.VerifyMessage(pubkey2, spork.vchSig, strMessage, errorMessage)) {
        LogPrintf("CMasternodePayments::Sign - Verify message failed");
        return false;
    }

    return true;
}

bool CSporkManager::UpdateSpork(int nSporkID, int64_t nValue, CConnman& connman)
{
    CSporkMessage msg;
    msg.nSporkID = nSporkID;
    msg.nValue = nValue;
    msg.nTimeSigned = GetTime();

    if (Sign(msg)) {
        CInv spork(MSG_SPORK, msg.GetHash());
        connman.RelayInv(spork);
        mapSporks[msg.GetHash()] = msg;
        mapSporksActive[nSporkID] = msg;
        return true;
    }

    return false;
}

void CSporkManager::Relay(CSporkMessage& msg, CConnman& connman)
{
    CInv inv(MSG_SPORK, msg.GetHash());
    connman.RelayInv(inv);
}

bool CSporkManager::SetPrivKey(std::string strPrivKey)
{
    CSporkMessage msg;
    LogPrintf("CSporkManager::SetPrivKey - setting private key %s\n", strPrivKey);

    // Test signing successful, proceed
    strMasterPrivKey = strPrivKey;

    if(!Sign(msg)){
        LogPrintf("CSporkManager::SetPrivKey - Failed to Sign\n");
        return false;
    }

    if (CheckSignature(msg)) {
        LogPrintf("CSporkManager::SetPrivKey - Successfully initialized as spork signer\n");
        return true;
    } else {
        return false;
    }
}

int CSporkManager::GetSporkIDByName(std::string strName)
{
    if (strName == "SPORK_2_INSTANTX")
        return SPORK_2_INSTANTX;
    if (strName == "SPORK_3_INSTANTX_BLOCK_FILTERING")
        return SPORK_3_INSTANTX_BLOCK_FILTERING;
    if (strName == "SPORK_4_ENABLE_MASTERNODE_PAYMENTS")
        return SPORK_4_ENABLE_MASTERNODE_PAYMENTS;
    if (strName == "SPORK_5_MAX_VALUE")
        return SPORK_5_MAX_VALUE;
    if (strName == "SPORK_7_MASTERNODE_SCANNING")
        return SPORK_7_MASTERNODE_SCANNING;
    if (strName == "SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT")
        return SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT;
    if (strName == "SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT")
        return SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT;
    if (strName == "SPORK_10_MASTERNODE_PAY_UPDATED_NODES")
        return SPORK_10_MASTERNODE_DONT_PAY_OLD_NODES;
    if (strName == "SPORK_11_RESET_BUDGET")
        return SPORK_11_RESET_BUDGET;
    if (strName == "SPORK_12_RECONSIDER_BLOCKS")
        return SPORK_12_RECONSIDER_BLOCKS;
    if (strName == "SPORK_13_ENABLE_SUPERBLOCKS")
        return SPORK_13_ENABLE_SUPERBLOCKS;
    if (strName == "SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT")
        return SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT;
    if (strName == "SPORK_15_SYSTEMNODE_PAY_UPDATED_NODES")
        return SPORK_15_SYSTEMNODE_DONT_PAY_OLD_NODES;
    if (strName == "SPORK_16_DISCONNECT_OLD_NODES")
        return SPORK_16_DISCONNECT_OLD_NODES;
    if (strName == "SPORK_17_NFT_TX")
        return SPORK_17_NFT_TX;

    return -1;
}

std::string CSporkManager::GetSporkNameByID(int id)
{
    if (id == SPORK_2_INSTANTX)
        return "SPORK_2_INSTANTX";
    if (id == SPORK_3_INSTANTX_BLOCK_FILTERING)
        return "SPORK_3_INSTANTX_BLOCK_FILTERING";
    if (id == SPORK_4_ENABLE_MASTERNODE_PAYMENTS)
        return "SPORK_4_ENABLE_MASTERNODE_PAYMENTS";
    if (id == SPORK_5_MAX_VALUE)
        return "SPORK_5_MAX_VALUE";
    if (id == SPORK_7_MASTERNODE_SCANNING)
        return "SPORK_7_MASTERNODE_SCANNING";
    if (id == SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT)
        return "SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT";
    if (id == SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT)
        return "SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT";
    if (id == SPORK_10_MASTERNODE_DONT_PAY_OLD_NODES)
        return "SPORK_10_MASTERNODE_PAY_UPDATED_NODES";
    if (id == SPORK_11_RESET_BUDGET)
        return "SPORK_11_RESET_BUDGET";
    if (id == SPORK_12_RECONSIDER_BLOCKS)
        return "SPORK_12_RECONSIDER_BLOCKS";
    if (id == SPORK_13_ENABLE_SUPERBLOCKS)
        return "SPORK_13_ENABLE_SUPERBLOCKS";
    if (id == SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT)
        return "SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT";
    if (id == SPORK_15_SYSTEMNODE_DONT_PAY_OLD_NODES)
        return "SPORK_15_SYSTEMNODE_PAY_UPDATED_NODES";
    if (id == SPORK_16_DISCONNECT_OLD_NODES)
        return "SPORK_16_DISCONNECT_OLD_NODES";
    if (id == SPORK_17_NFT_TX)
        return "SPORK_17_NFT_TX";

    return "Unknown";
}

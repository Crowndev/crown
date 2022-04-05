// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrman.h>
#include <consensus/validation.h>
#include <crown/legacycalls.h>
#include <crown/legacysigner.h>
#include <crown/nodewallet.h>
#include <key_io.h>
#include <masternode/masternode.h>
#include <masternode/masternodeman.h>
#include <pos/blockwitness.h>
#include <pos/prooftracker.h>
#include <net_processing.h>
#include <netbase.h>
#include <shutdown.h>
#include <sync.h>
#include <util/system.h>
#include <validation.h>

#include <boost/lexical_cast.hpp>

// keep track of the scanning errors I've seen
std::map<uint256, int> mapSeenMasternodeScanningErrors;
// cache block hashes as we calculate them
std::map<int64_t, uint256> mapCacheBlockHashes;

//Get the last hash that matches the modulus given. Processed in reverse order
bool GetBlockHash(uint256& hash, int nBlockHeight)
{
    if (::ChainActive().Tip() == NULL) return false;

    if(nBlockHeight == 0)
        nBlockHeight = ::ChainActive().Tip()->nHeight;

    if(mapCacheBlockHashes.count(nBlockHeight)){
        hash = mapCacheBlockHashes[nBlockHeight];
        return true;
    }

    const CBlockIndex *BlockLastSolved = ::ChainActive().Tip();
    const CBlockIndex *BlockReading = ::ChainActive().Tip();

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || ::ChainActive().Tip()->nHeight+1 < nBlockHeight) return false;

    int nBlocksAgo = 0;
    if(nBlockHeight > 0) nBlocksAgo = (::ChainActive().Tip()->nHeight+1)-nBlockHeight;
    assert(nBlocksAgo >= 0);

    int n = 0;
    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if(n >= nBlocksAgo){
            hash = BlockReading->GetBlockHash();
            mapCacheBlockHashes[nBlockHeight] = hash;
            return true;
        }
        n++;

        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    return false;
}
void vecHash(uint256& hash, std::vector<uint256> vPrevBlockHash)
{
    bool firstHash = true;
    for (std::vector<uint256>::const_iterator it(vPrevBlockHash.begin()); it != vPrevBlockHash.end(); ++it) {
        if (firstHash) {
            hash = Hash(*it, hash);
            firstHash = false;
        } else {
            hash = Hash(hash, *it);
        }
    }
}

CMasternode::CMasternode()
{
    LOCK(cs);
    vin = CTxIn();
    addr = CService();
    pubkey = CPubKey();
    pubkey2 = CPubKey();
    sig = std::vector<unsigned char>();
    vchSignover = std::vector<unsigned char>();
    activeState = MASTERNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CMasternodePing();
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = PROTOCOL_VERSION;
    nLastDsq = 0;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
    lastTimeChecked = 0;
}

CMasternode::CMasternode(const CMasternode& other)
{
    LOCK(cs);
    vin = other.vin;
    addr = other.addr;
    pubkey = other.pubkey;
    pubkey2 = other.pubkey2;
    sig = other.sig;
    vchSignover = other.vchSignover;
    activeState = other.activeState;
    sigTime = other.sigTime;
    lastPing = other.lastPing;
    cacheInputAge = other.cacheInputAge;
    cacheInputAgeBlock = other.cacheInputAgeBlock;
    unitTest = other.unitTest;
    allowFreeTx = other.allowFreeTx;
    protocolVersion = other.protocolVersion;
    nLastDsq = other.nLastDsq;
    nScanningErrorCount = other.nScanningErrorCount;
    nLastScanningErrorBlockHeight = other.nLastScanningErrorBlockHeight;
    lastTimeChecked = 0;
}

CMasternode::CMasternode(const CMasternodeBroadcast& mnb)
{
    LOCK(cs);
    vin = mnb.vin;
    addr = mnb.addr;
    pubkey = mnb.pubkey;
    pubkey2 = mnb.pubkey2;
    sig = mnb.sig;
    vchSignover = mnb.vchSignover;
    activeState = MASTERNODE_ENABLED;
    sigTime = mnb.sigTime;
    lastPing = mnb.lastPing;
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = mnb.protocolVersion;
    nLastDsq = mnb.nLastDsq;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
    lastTimeChecked = 0;
}

//
// When a new masternode broadcast is sent, update our information
//
bool CMasternode::UpdateFromNewBroadcast(const CMasternodeBroadcast& mnb)
{
    if(mnb.sigTime > sigTime) {
        pubkey2 = mnb.pubkey2;
        sigTime = mnb.sigTime;
        sig = mnb.sig;
        protocolVersion = mnb.protocolVersion;
        addr = mnb.addr;
        lastTimeChecked = 0;
        int nDoS = 0;
        if(mnb.lastPing == CMasternodePing() || (mnb.lastPing != CMasternodePing() && mnb.lastPing.CheckAndUpdate(nDoS, false))) {
            lastPing = mnb.lastPing;
            mnodeman.mapSeenMasternodePing.insert(std::make_pair(lastPing.GetHash(), lastPing));
        }
        return true;
    }
    return false;
}

//
// Deterministically calculate a given "score" for a Masternode depending on how close it's hash is to
// the proof of work for that block. The further away they are the better, the furthest will win the election
// and get paid this block
//
arith_uint256 CMasternode::CalculateScore(int64_t nBlockHeight) const
{
    if(::ChainActive().Tip() == NULL)
        return arith_uint256();

    // Find the block hash where tx got MASTERNODE_MIN_CONFIRMATIONS
    int nPrevoutAge = GetUTXOConfirmations(vin.prevout);
    CBlockIndex *pblockIndex = ::ChainActive()[nPrevoutAge + MASTERNODE_MIN_CONFIRMATIONS - 1];
    if (!pblockIndex)
        return arith_uint256();
    uint256 collateralMinConfBlockHash = pblockIndex->GetBlockHash();

    uint256 hash = uint256();

    if(!GetBlockHash(hash, nBlockHeight)) {
        LogPrint(BCLog::MASTERNODE, "CalculateScore ERROR - nHeight %d - Returned 0\n", nBlockHeight);
        return arith_uint256();
    }

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << vin.prevout << collateralMinConfBlockHash << hash;
    return UintToArith256(ss.GetHash());
}

CMasternode::CollateralStatus CMasternode::CheckCollateral(const COutPoint& outpoint)
{
    int nHeight;
    return CheckCollateral(outpoint, nHeight);
}

CMasternode::CollateralStatus CMasternode::CheckCollateral(const COutPoint& outpoint, int& nHeightRet)
{
    AssertLockHeld(cs_main);

    Coin coin;
    if (!GetUTXOCoin(outpoint, coin)) {
        return COLLATERAL_UTXO_NOT_FOUND;
    }

    if (coin.IsSpent()) {
        return COLLATERAL_UTXO_NOT_FOUND;
    }

    if (coin.out.nValue != Params().MasternodeCollateral()) {
        return COLLATERAL_INVALID_AMOUNT;
    }

    nHeightRet = coin.nHeight;
    return COLLATERAL_OK;
}

void CMasternode::Check(bool forceCheck)
{
    if(ShutdownRequested()) return;

    if(!forceCheck && (GetTime() - lastTimeChecked < MASTERNODE_CHECK_SECONDS)) return;
    lastTimeChecked = GetTime();


    //once spent, stop doing the checks
    if(activeState == MASTERNODE_VIN_SPENT) return;


    if(!IsPingedWithin(MASTERNODE_REMOVAL_SECONDS)){
        activeState = MASTERNODE_REMOVE;
        return;
    }

    if(!IsPingedWithin(MASTERNODE_EXPIRATION_SECONDS)){
        activeState = MASTERNODE_EXPIRED;
        return;
    }

    //test if the collateral is still good
    if (!unitTest) {
        TRY_LOCK(cs_main, lockMain);
        if(!lockMain) return;
        CollateralStatus err = CheckCollateral(vin.prevout);
        if (err == COLLATERAL_UTXO_NOT_FOUND) {
            activeState = MASTERNODE_VIN_SPENT;
            LogPrint(BCLog::MASTERNODE, "CMasternode::Check -- Failed to find Masternode UTXO, masternode=%s\n", vin.prevout.ToString());
            return;
        }
    }

    activeState = MASTERNODE_ENABLED; // OK
}

bool CMasternode::IsValidNetAddr() const
{
    if (Params().NetworkIDString() == CBaseChainParams::TESTNET)
        return true;
    // TODO: regtest is fine with any addresses for now,
    // should probably be a bit smarter if one day we start to implement tests for this
    return (addr.IsIPv4() && addr.IsRoutable());
}

int64_t CMasternode::SecondsSincePayment() const
{
    CScript pubkeyScript;
    pubkeyScript = GetScriptForDestination(PKHash(pubkey.GetID()));

    int64_t sec = (GetAdjustedTime() - GetLastPaid());
    int64_t month = 60 * 60 * 24 * 30;
    if (sec < month)
        return sec; //if it's less than 30 days, give seconds

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << vin;
    ss << sigTime;
    uint256 hash = ss.GetHash();

    // return some deterministic value for unknown/unpaid but force it to be more than 30 days old
    return month + UintToArith256(hash).GetCompact(false);
}

int64_t CMasternode::GetLastPaid() const
{
    CBlockIndex* pindexPrev = ::ChainActive().Tip();
    if(pindexPrev == NULL) return false;

    CScript mnpayee;
    mnpayee = GetScriptForDestination(PKHash(pubkey.GetID()));

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << vin;
    ss << sigTime;
    uint256 hash =  ss.GetHash();

    // use a deterministic offset to break a tie -- 2.5 minutes
    int64_t nOffset = UintToArith256(hash).GetCompact(false) % 150;

    if (::ChainActive().Tip() == NULL) return false;

    const CBlockIndex *BlockReading = ::ChainActive().Tip();

    int nMnCount = mnodeman.CountEnabled()*1.25;
    int n = 0;
    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if(n >= nMnCount){
            return 0;
        }
        n++;

        if(masternodePayments.mapMasternodeBlocks.count(BlockReading->nHeight)){
            /*
                Search for this payee, with at least 2 votes. This will aid in consensus allowing the network
                to converge on the same payees quickly, then keep the same schedule.
            */
            if(masternodePayments.mapMasternodeBlocks[BlockReading->nHeight].HasPayeeWithVotes(mnpayee, 2)){
                return BlockReading->nTime + nOffset;
            }
        }

        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    return 0;
}


// Find all blocks where MN received reward within defined block depth
// Used for generating stakepointers
bool CMasternode::GetRecentPaymentBlocks(std::vector<const CBlockIndex*>& vPaymentBlocks, bool limitMostRecent) const
{
    vPaymentBlocks.clear();
    int nMinimumValidBlockHeight = ::ChainActive().Height() - Params().ValidStakePointerDuration() + 1;
    if (nMinimumValidBlockHeight < 1)
        nMinimumValidBlockHeight = 1;

    CBlockIndex* pindex = ::ChainActive()[nMinimumValidBlockHeight];

    CScript mnpayee;
    mnpayee = GetScriptForDestination(PKHash(pubkey));

    bool fBlockFound = false;
    while (::ChainActive().Next(pindex)) {
        CBlock block;
        if (!ReadBlockFromDisk(block, pindex, Params().GetConsensus()))
            continue;

        int m = (block.vtx[0]->nVersion >= TX_ELE_VERSION ? block.vtx[0]->vpout.size() : block.vtx[0]->vout.size());

        if (m > 1) {
            CTxOutAsset out = (block.vtx[0]->nVersion >= TX_ELE_VERSION ? block.vtx[0]->vpout[1] : block.vtx[0]->vout[1]);
            if (out.scriptPubKey == mnpayee){
                vPaymentBlocks.emplace_back(pindex);
                fBlockFound = true;
                if (limitMostRecent)
                    return fBlockFound;
            }
        }
        pindex = ::ChainActive().Next(pindex);
    }

    return fBlockFound;
}

CMasternodeBroadcast::CMasternodeBroadcast()
{
    vin = CTxIn();
    addr = CService();
    pubkey = CPubKey();
    pubkey2 = CPubKey();
    sig = std::vector<unsigned char>();
    vchSignover = std::vector<unsigned char>();
    activeState = MASTERNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CMasternodePing();
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = PROTOCOL_VERSION;
    nLastDsq = 0;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
}

CMasternodeBroadcast::CMasternodeBroadcast(CService newAddr, CTxIn newVin, CPubKey newPubkey, CPubKey newPubkey2, int protocolVersionIn)
{
    vin = newVin;
    addr = newAddr;
    pubkey = newPubkey;
    pubkey2 = newPubkey2;
    sig = std::vector<unsigned char>();
    vchSignover = std::vector<unsigned char>();
    activeState = MASTERNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CMasternodePing();
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = protocolVersionIn;
    nLastDsq = 0;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
}

CMasternodeBroadcast::CMasternodeBroadcast(const CMasternode& mn)
{
    vin = mn.vin;
    addr = mn.addr;
    pubkey = mn.pubkey;
    pubkey2 = mn.pubkey2;
    sig = mn.sig;
    vchSignover = mn.vchSignover;
    activeState = mn.activeState;
    sigTime = mn.sigTime;
    lastPing = mn.lastPing;
    cacheInputAge = mn.cacheInputAge;
    cacheInputAgeBlock = mn.cacheInputAgeBlock;
    unitTest = mn.unitTest;
    allowFreeTx = mn.allowFreeTx;
    protocolVersion = mn.protocolVersion;
    nLastDsq = mn.nLastDsq;
    nScanningErrorCount = mn.nScanningErrorCount;
    nLastScanningErrorBlockHeight = mn.nLastScanningErrorBlockHeight;
}

bool CMasternodeBroadcast::Create(std::string strService, std::string strKeyMasternode, std::string strTxHash, std::string strOutputIndex, std::string& strErrorMessage, CMasternodeBroadcast &mnb, bool fOffline) {
    CTxIn txin;
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;
    CPubKey pubKeyMasternodeNew;
    CKey keyMasternodeNew;

    if (!gArgs.GetBoolArg("-jumpstart", false))
    {
        //need correct blocks to send ping
        if(!fOffline && !masternodeSync.IsBlockchainSynced()) {
            strErrorMessage = "Sync in progress. Must wait until sync is complete to start Masternode";
            LogPrint(BCLog::NET, "CMasternodeBroadcast::Create -- %s\n", strErrorMessage);
            return false;
        }
    }

    if (!legacySigner.SetKey(strKeyMasternode, keyMasternodeNew, pubKeyMasternodeNew)) {
        strErrorMessage = strprintf("Can't find keys for masternode %s - %s", strService, strErrorMessage);
        LogPrint(BCLog::MASTERNODE, "CMasternodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    if(!GetWallets()[0]->GetMasternodeVinAndKeys(txin, pubKeyCollateralAddress, keyCollateralAddress, strTxHash, strOutputIndex)) {
        strErrorMessage = strprintf("Could not allocate txin %s:%s for masternode %s", strTxHash, strOutputIndex, strService);
        LogPrint(BCLog::MASTERNODE, "CMasternodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    int age = GetUTXOConfirmations(txin.prevout);
    if (age < MASTERNODE_MIN_CONFIRMATIONS)
    {
        strErrorMessage = strprintf("Input must have at least %d confirmations. Now it has %d",
                                     MASTERNODE_MIN_CONFIRMATIONS, age);
        LogPrint(BCLog::MASTERNODE, "CSystemnodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    CService service = CService(LookupNumeric(strService, Params().GetDefaultPort()));
    if(Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if(service.GetPort() != 9340) {
            strErrorMessage = strprintf("Invalid port %u for masternode %s - only 9340 is supported on mainnet.", service.GetPort(), strService);
            LogPrint(BCLog::MASTERNODE, "CMasternodeBroadcast::Create -- %s\n", strErrorMessage);
            return false;
        }
    } else if(service.GetPort() == 9340) {
        strErrorMessage = strprintf("Invalid port %u for masternode %s - 9340 is only supported on mainnet.", service.GetPort(), strService);
        LogPrint(BCLog::MASTERNODE, "CMasternodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    bool fSignOver = true;
    return Create(txin, service, keyCollateralAddress, pubKeyCollateralAddress, keyMasternodeNew, pubKeyMasternodeNew, fSignOver, strErrorMessage, mnb);
}

bool CMasternodeBroadcast::Create(CTxIn txin, CService service, CKey keyCollateralAddress, CPubKey pubKeyCollateralAddress, CKey keyMasternodeNew, CPubKey pubKeyMasternodeNew, bool fSignOver, std::string &strErrorMessage, CMasternodeBroadcast &mnb) {
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    CMasternodePing mnp(txin);
    if(!mnp.Sign(keyMasternodeNew, pubKeyMasternodeNew)){
        strErrorMessage = strprintf("Failed to sign ping, txin: %s", txin.ToString());
        LogPrint(BCLog::MASTERNODE, "CMasternodeBroadcast::Create --  %s\n", strErrorMessage);
        mnb = CMasternodeBroadcast();
        return false;
    }

    mnb = CMasternodeBroadcast(service, txin, pubKeyCollateralAddress, pubKeyMasternodeNew, PROTOCOL_VERSION);

    if(!mnb.IsValidNetAddr()) {
        strErrorMessage = strprintf("Invalid IP address, masternode=%s", txin.prevout.ToStringShort());
        LogPrint(BCLog::NET, "CMasternodeBroadcast::Create -- %s\n", strErrorMessage);
        mnb = CMasternodeBroadcast();
        return false;
    }

    mnb.lastPing = mnp;
    if(!mnb.Sign(keyCollateralAddress)){
        strErrorMessage = strprintf("Failed to sign broadcast, txin: %s", txin.ToString());
        LogPrint(BCLog::MASTERNODE, "CMasternodeBroadcast::Create -- %s\n", strErrorMessage);
        mnb = CMasternodeBroadcast();
        return false;
    }

    //Additional signature for use in proof of stake
    if (fSignOver) {
        if (!keyCollateralAddress.Sign(pubKeyMasternodeNew.GetHash(), mnb.vchSignover)) {
            LogPrint(BCLog::MASTERNODE, "CMasternodeBroadcast::Create failed signover\n");
            mnb = CMasternodeBroadcast();
            return false;
        }
        LogPrint(BCLog::MASTERNODE, "%s: Signed over to key %s\n", __func__, pubKeyMasternodeNew.GetID().GetHex());
    }

    return true;
}

bool CMasternodeBroadcast::CheckAndUpdate(int& nDos) const
{
    nDos = 0;

    // make sure signature isn't in the future (past is OK)
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrintf("mnb - Signature rejected, too far into the future %s\n", vin.ToString());
        nDos = 1;
        return false;
    }

    if(protocolVersion < masternodePayments.GetMinMasternodePaymentsProto()) {
        LogPrint(BCLog::MASTERNODE, "mnb - ignoring outdated Masternode %s protocol version %d\n", vin.ToString(), protocolVersion);
        return false;
    }

    CScript pubkeyScript;
    pubkeyScript = GetScriptForDestination(PKHash(pubkey));

    if(pubkeyScript.size() != 25) {
        LogPrint(BCLog::MASTERNODE, "mnb - pubkey the wrong size\n");
        nDos = 100;
        return false;
    }

    CScript pubkeyScript2;
    pubkeyScript2 = GetScriptForDestination(PKHash(pubkey2));

    if(pubkeyScript2.size() != 25) {
        LogPrint(BCLog::MASTERNODE, "mnb - pubkey2 the wrong size\n");
        nDos = 100;
        return false;
    }

    if(!vin.scriptSig.empty()) {
        LogPrint(BCLog::MASTERNODE, "mnb - Ignore Not Empty ScriptSig %s\n", vin.ToString());
        return false;
    }

    // incorrect ping or its sigTime
    if(lastPing == CMasternodePing() || !lastPing.CheckAndUpdate(nDos, false, true))
        return false;

    std::string strMessage;
    std::string errorMessage = "";
    std::string vchPubKey(pubkey.begin(), pubkey.end());
    std::string vchPubKey2(pubkey2.begin(), pubkey2.end());
    strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) +
                 vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

    if(!legacySigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)) {
        if (addr.ToString() != addr.ToString())
        {
            // maybe it's wrong format, try again with the old one
            strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) +
                            vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

            if(!legacySigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)){
                // didn't work either
                LogPrintf("mnb - Got bad Masternode address signature, sanitized error: %s\n", SanitizeString(errorMessage));
                // there is a bug in old MN signatures, ignore such MN but do not ban the peer we got this from
                return false;
            }
        } else {
            // nope, sig is actually wrong
            LogPrintf("mnb - Got bad Masternode address signature, sanitized error: %s\n", SanitizeString(errorMessage));
            // there is a bug in old MN signatures, ignore such MN but do not ban the peer we got this from
            return false;
        }
    }

    if(Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if(addr.GetPort() != 9340) return false;
    } else if(addr.GetPort() == 9340) return false;

    //search existing Masternode list, this is where we update existing Masternodes with new mnb broadcasts
    CMasternode* pmn = mnodeman.Find(vin);

    // no such masternode, nothing to update
    if(pmn == NULL) return true;

    // this broadcast is older or equal than the one that we already have - it's bad and should never happen
    // unless someone is doing something fishy
    // (mapSeenMasternodeBroadcast in CMasternodeMan::ProcessMessage should filter legit duplicates)
    if(pmn->sigTime >= sigTime) {
        LogPrint(BCLog::NET, "CMasternodeBroadcast::CheckAndUpdate - Bad sigTime %d for Masternode %20s %105s (existing broadcast is at %d)\n",
                      sigTime, addr.ToString(), vin.ToString(), pmn->sigTime);
        return false;
    }

    // masternode is not enabled yet/already, nothing to update
    if(!pmn->IsEnabled()) return true;

    // mn.pubkey = pubkey, IsVinAssociatedWithPubkey is validated once below,
    //   after that they just need to match
    if(pmn->pubkey == pubkey && !pmn->IsBroadcastedWithin(MASTERNODE_MIN_MNB_SECONDS)) {
        //take the newest entry
        LogPrintf("mnb - Got updated entry for %s\n", addr.ToString());
        if(pmn->UpdateFromNewBroadcast((*this))){
            pmn->Check();
            if(pmn->IsEnabled()) Relay();
        }
        masternodeSync.AddedMasternodeList(GetHash());
    }

    return true;
}

bool CMasternodeBroadcast::CheckInputsAndAdd(int& nDoS) const
{
    LogPrintf("%s - 1\n", __func__);

    // we are a masternode with the same vin (i.e. already activated) and this mnb is ours (matches our Masternode privkey)
    // so nothing to do here for us
    if(fMasterNode && vin.prevout == activeMasternode.vin.prevout && pubkey2 == activeMasternode.pubKeyMasternode)
        return true;

    // incorrect ping or its sigTime
    if(lastPing == CMasternodePing() || !lastPing.CheckAndUpdate(nDoS, false, true))
        return false;

    LogPrintf("%s - 2\n", __func__);

    // search existing Masternode list
    CMasternode* pmn = mnodeman.Find(vin);

    if(pmn != NULL) {
        // nothing to do here if we already know about this masternode and it's enabled
        if(pmn->IsEnabled()) return true;
        // if it's not enabled, remove old MN first and continue
        else mnodeman.Remove(pmn->vin);
    }

/*    TxValidationState state;
    CMutableTransaction tx;
    CTxOutAsset vout = CTxOutAsset(Params().GetConsensus().subsidy_asset, (Params().MasternodeCollateral() - 0.01)*COIN, legacySigner.collateralPubKey);
    tx.vin.push_back(vin);
    if(tx.nVersion >= TX_ELE_VERSION)
        tx.vpout.push_back(vout);
    else
        tx.vout.push_back(vout);

    {
        TRY_LOCK(cs_main, lockMain);
        if(!lockMain) {
            // not mnb fault, let it to be checked again later
            mnodeman.mapSeenMasternodeBroadcast.erase(GetHash());
            masternodeSync.mapSeenSyncMNB.erase(GetHash());
            return false;
        }

        if(!AcceptToMemoryPool(*g_mempool, state, MakeTransactionRef(tx), NULL, NULL, true, 0)) {
            //set nDos
            //state.IsInvalid(nDoS);
            return false;
        }
    }*/
    LogPrintf("%s - 3\n", __func__);

    LogPrint(BCLog::NET, "masternode", "mnb - Accepted Masternode entry\n");

    if(GetInputAge(vin) < MASTERNODE_MIN_CONFIRMATIONS){
        LogPrint(BCLog::MASTERNODE, "mnb - Input must have at least %d confirmations\n", MASTERNODE_MIN_CONFIRMATIONS);
        // maybe we miss few blocks, let this mnb to be checked again later
        mnodeman.mapSeenMasternodeBroadcast.erase(GetHash());
        masternodeSync.mapSeenSyncMNB.erase(GetHash());
        return false;
    }

    // verify that sig time is legit in past
    // should be at least not earlier than block when 10000 CRW tx got MASTERNODE_MIN_CONFIRMATIONS
    uint256 hashBlock = uint256();
    CTransactionRef tx2 = GetTransaction(nullptr, nullptr, vin.prevout.hash, Params().GetConsensus(), hashBlock);
    BlockMap::iterator mi = g_chainman.m_blockman.m_block_index.find(hashBlock);
    if (mi != g_chainman.m_blockman.m_block_index.end() && (*mi).second)
    {
        CBlockIndex* pMNIndex = (*mi).second; // block for 10000 CRW tx -> 1 confirmation
        CBlockIndex* pConfIndex = ::ChainActive()[pMNIndex->nHeight + MASTERNODE_MIN_CONFIRMATIONS - 1]; // block where tx got MASTERNODE_MIN_CONFIRMATIONS
        if(pConfIndex->GetBlockTime() > sigTime)
        {
            LogPrintf("mnb - Bad sigTime %d for Masternode %20s %105s (%i conf block is at %d)\n",
                      sigTime, addr.ToString(), vin.ToString(), MASTERNODE_MIN_CONFIRMATIONS, pConfIndex->GetBlockTime());
            return false;
        }
    }

    LogPrint(BCLog::MASTERNODE, "mnb - Got NEW Masternode entry - %s - %s - %s - %d \n", GetHash().ToString(), addr.ToString(), vin.ToString(), sigTime);
    CMasternode mn(*this);
    mnodeman.Add(mn);

    // if it matches our Masternode privkey, then we've been remotely activated
    if(pubkey2 == activeMasternode.pubKeyMasternode && protocolVersion == PROTOCOL_VERSION){
        activeMasternode.EnableHotColdMasterNode(vin, addr);
        if (!vchSignover.empty()) {
            if (pubkey.Verify(pubkey2.GetHash(), vchSignover)) {
                LogPrint(BCLog::MASTERNODE, "%s: Verified pubkey2 signover for staking, added to activemasternode\n", __func__);
                activeMasternode.vchSigSignover = vchSignover;
            } else {
                LogPrint(BCLog::MASTERNODE, "%s: Failed to verify pubkey on signover!\n", __func__);
            }
        } else {
            LogPrintf("%s: NOT SIGNOVER!\n", __func__);
        }
    }

    bool isLocal = addr.IsRFC1918() || addr.IsLocal();
    if(Params().NetworkIDString() == CBaseChainParams::REGTEST) isLocal = false;

    if(!isLocal) Relay();

    return true;
}

void CMasternodeBroadcast::Relay() const
{
    CInv inv(MSG_MASTERNODE_ANNOUNCE, GetHash());
    g_connman->RelayInv(inv);
}

bool CMasternodeBroadcast::Sign(const CKey& keyCollateralAddress)
{
    std::string errorMessage;

    std::string vchPubKey(pubkey.begin(), pubkey.end());
    std::string vchPubKey2(pubkey2.begin(), pubkey2.end());

    sigTime = GetAdjustedTime();

    std::string strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

    if (!legacySigner.SignMessage(strMessage, sig, keyCollateralAddress)) {
        LogPrint(BCLog::MASTERNODE, "CMasternodeBroadcast::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}

bool CMasternodeBroadcast::VerifySignature() const
{
    std::string errorMessage;

    std::string vchPubKey(pubkey.begin(), pubkey.end());
    std::string vchPubKey2(pubkey2.begin(), pubkey2.end());

    std::string strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

    if(!legacySigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)) {
        LogPrint(BCLog::MASTERNODE, "CMasternodeBroadcast::VerifySignature() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}

CMasternodePing::CMasternodePing()
{
    vin = CTxIn();
    blockHash = uint256();
    sigTime = 0;
    vchSig = std::vector<unsigned char>();
    vchSigPrevBlocks = std::vector<unsigned char>();
    vPrevBlockHash = std::vector<uint256>();
    nVersion = 2;
}

CMasternodePing::CMasternodePing(const CTxIn& newVin)
{
    vin = newVin;
    blockHash = ::ChainActive()[::ChainActive().Height() - 12]->GetBlockHash();
    sigTime = GetAdjustedTime();
    vchSig = std::vector<unsigned char>();
    vchSigPrevBlocks = std::vector<unsigned char>();
    vPrevBlockHash = std::vector<uint256>();

    //Add previous 10 blocks
    for (unsigned int i = 0; i < 10; i ++) {
        vPrevBlockHash.emplace_back(::ChainActive()[::ChainActive().Height() - i]->GetBlockHash());
    }

    nVersion = 2;
}


bool CMasternodePing::Sign(const CKey& keyMasternode, const CPubKey& pubKeyMasternode)
{
    std::string errorMessage;
    std::string strMasterNodeSignMessage;

    sigTime = GetAdjustedTime();
    std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);

    if (!legacySigner.SignMessage(strMessage, vchSig, keyMasternode)) {
        LogPrint(BCLog::MASTERNODE, "CMasternodePing::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    if (!legacySigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage)) {
        LogPrint(BCLog::MASTERNODE, "CMasternodePing::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    uint256 hash;
    vecHash(hash, vPrevBlockHash);
    if(!legacySigner.SignMessage(hash.GetHex(), vchSigPrevBlocks, keyMasternode)) {
        LogPrintf("CMasternodePing::Sign() - Error signing previous blocks: %s\n", errorMessage);
        return false;
    }

    if(!legacySigner.VerifyMessage(pubKeyMasternode, vchSigPrevBlocks, hash.GetHex(), errorMessage)) {
        LogPrint(BCLog::NET, "CMasternodePing::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}

bool CMasternodePing::VerifySignature(const CPubKey& pubKeyMasternode, int &nDos) const
{
    std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);
    std::string errorMessage = "";

    if(!legacySigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage))
    {
        LogPrint(BCLog::MASTERNODE, "CMasternodePing::VerifySignature - Got bad Masternode ping signature %s Error: %s\n", vin.ToString(), errorMessage);
        return false;
    }

    //Also check signature of previous blockhashes
    if (nVersion > 1) {
        uint256 hash;
        vecHash(hash, vPrevBlockHash);
        if (!legacySigner.VerifyMessage(pubKeyMasternode, vchSigPrevBlocks, hash.GetHex(), errorMessage)) {
            LogPrint(BCLog::NET, "CMasternodePing::VerifySignature - Got bad Masternode signature for previous blocks %s Error: %s\n", vin.ToString(), errorMessage);
            nDos = 33;
            return false;
        }
    }

    return true;
}

bool CMasternodePing::CheckAndUpdate(int& nDos, bool fRequireEnabled, bool fCheckSigTimeOnly) const
{
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrint(BCLog::MASTERNODE, "CMasternodePing::CheckAndUpdate - Signature rejected, too far into the future %s\n", vin.ToString());
        nDos = 1;
        return false;
    }

    if (sigTime <= GetAdjustedTime() - 60 * 60) {
        LogPrint(BCLog::MASTERNODE, "CMasternodePing::CheckAndUpdate - Signature rejected, too far into the past %s - %d %d \n", vin.ToString(), sigTime, GetAdjustedTime());
        nDos = 1;
        return false;
    }

    if(fCheckSigTimeOnly) {
        CMasternode* pmn = mnodeman.Find(vin);
        if(pmn) return VerifySignature(pmn->pubkey2, nDos);
        return true;
    }

    LogPrint(BCLog::MASTERNODE, "CMasternodePing::CheckAndUpdate - New Ping - %s - %s - %d\n", GetHash().ToString(), blockHash.ToString(), sigTime);

    // see if we have this Masternode
    CMasternode* pmn = mnodeman.Find(vin);
    if (pmn && pmn->protocolVersion >= masternodePayments.GetMinMasternodePaymentsProto()) {
        if (fRequireEnabled && !pmn->IsEnabled()) return false;

        // LogPrintf("mnping - Found corresponding mn for vin: %s\n", vin.ToString());
        // update only if there is no known ping for this masternode or
        // last ping was more then MASTERNODE_MIN_MNP_SECONDS-60 ago comparing to this one
        if(!pmn->IsPingedWithin(MASTERNODE_MIN_MNP_SECONDS - 60, sigTime))
        {
            if(!VerifySignature(pmn->pubkey2, nDos))
                return false;

            BlockMap::iterator mi = g_chainman.m_blockman.m_block_index.find(blockHash);
            if (mi != g_chainman.m_blockman.m_block_index.end() && (*mi).second)
            {
                if((*mi).second->nHeight < ::ChainActive().Height() - 24)
                {
                    LogPrint(BCLog::NET, "CMasternodePing::CheckAndUpdate - Masternode %s block hash %s is too old\n", vin.ToString(), blockHash.ToString());
                    // Do nothing here (no Masternode update, no mnping relay)
                    // Let this node to be visible but fail to accept mnping

                    return false;
                }
            } else {
                LogPrint(BCLog::MASTERNODE, "CMasternodePing::CheckAndUpdate - Masternode %s block hash %s is unknown\n", vin.ToString(), blockHash.ToString());
                // maybe we stuck so we shouldn't ban this node, just fail to accept it
                // TODO: or should we also request this block?

                return false;
            }

            pmn->lastPing = *this;

            //mnodeman.mapSeenMasternodeBroadcast.lastPing is probably outdated, so we'll update it
            CMasternodeBroadcast mnb(*pmn);
            uint256 hash = mnb.GetHash();
            if(mnodeman.mapSeenMasternodeBroadcast.count(hash)) {
                mnodeman.mapSeenMasternodeBroadcast[hash].lastPing = *this;
            }

            pmn->Check(true);
            if(!pmn->IsEnabled()) return false;

            if (this->nVersion > 1) {
                for (const uint256& hashBlock : vPrevBlockHash) {
                    LogPrint(BCLog::MASTERNODE, "%s: Adding witness for block %s from mn %s\n", __func__, hashBlock.GetHex(), vin.ToString());
                    g_proofTracker->AddWitness(BlockWitness(pmn->vin, hashBlock));
                }
            }

            LogPrint(BCLog::MASTERNODE, "CMasternodePing::CheckAndUpdate - Masternode ping accepted, vin: %s\n", vin.ToString());

            Relay();
            return true;
        }
        LogPrint(BCLog::MASTERNODE, "CMasternodePing::CheckAndUpdate - Masternode ping arrived too early, vin: %s\n", vin.ToString());
        //nDos = 1; //disable, this is happening frequently and causing banned peers
        return false;
    }
    LogPrint(BCLog::MASTERNODE, "CMasternodePing::CheckAndUpdate - Couldn't find compatible Masternode entry, vin: %s\n", vin.ToString());

    return false;
}

void CMasternodePing::Relay() const
{
    CInv inv(MSG_MASTERNODE_PING, GetHash());
    g_connman->RelayInv(inv);
}

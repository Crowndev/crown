// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrman.h>
#include <consensus/validation.h>
#include <crown/legacycalls.h>
#include <crown/legacysigner.h>
#include <crown/nodewallet.h>
#include <key_io.h>
#include <pos/blockwitness.h>
#include <pos/prooftracker.h>
#include <net_processing.h>
#include <shutdown.h>
#include <sync.h>
#include <systemnode/systemnode.h>
#include <systemnode/systemnodeman.h>
#include <util/system.h>
#include <validation.h>
#include <netbase.h>

#include <boost/lexical_cast.hpp>


//
// CSystemnode
//

CSystemnode::CSystemnode()
{
    LOCK(cs);
    vin = CTxIn();
    addr = CService();
    pubkey = CPubKey();
    pubkey2 = CPubKey();
    sig = std::vector<unsigned char>();
    vchSignover = std::vector<unsigned char>();
    activeState = SYSTEMNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CSystemnodePing();
    unitTest = false;
    protocolVersion = PROTOCOL_VERSION;
    lastTimeChecked = 0;
}

CSystemnode::CSystemnode(const CSystemnode& other)
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
    unitTest = other.unitTest;
    protocolVersion = other.protocolVersion;
    lastTimeChecked = 0;
}

CSystemnode::CSystemnode(const CSystemnodeBroadcast& snb)
{
    LOCK(cs);
    vin = snb.vin;
    addr = snb.addr;
    pubkey = snb.pubkey;
    pubkey2 = snb.pubkey2;
    sig = snb.sig;
    activeState = SYSTEMNODE_ENABLED;
    vchSignover = snb.vchSignover;
    sigTime = snb.sigTime;
    lastPing = snb.lastPing;
    unitTest = false;
    protocolVersion = snb.protocolVersion;
    lastTimeChecked = 0;
}


//
// Deterministically calculate a given "score" for a Systemnode depending on how close it's hash is to
// the proof of work for that block. The further away they are the better, the furthest will win the election
// and get paid this block
//
arith_uint256 CSystemnode::CalculateScore(int64_t nBlockHeight) const
{
    if (::ChainActive().Tip() == nullptr)
        return arith_uint256();

    // Find the block hash where tx got SYSTEMNODE_MIN_CONFIRMATIONS
    int nPrevoutAge = GetUTXOConfirmations(vin.prevout);
    CBlockIndex *pblockIndex = ::ChainActive()[nPrevoutAge + SYSTEMNODE_MIN_CONFIRMATIONS - 1];
    if (!pblockIndex)
        return arith_uint256();
    uint256 collateralMinConfBlockHash = pblockIndex->GetBlockHash();

    uint256 hash = uint256();

    if (!GetBlockHash(hash, nBlockHeight)) {
        LogPrint(BCLog::SYSTEMNODE, "CalculateScore ERROR - nHeight %d - Returned 0\n", nBlockHeight);
        return arith_uint256();
    }

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << vin.prevout << collateralMinConfBlockHash << hash;
    return UintToArith256(ss.GetHash());
}

CSystemnode::CollateralStatus CSystemnode::CheckCollateral(const COutPoint& outpoint)
{
    int nHeight;
    return CheckCollateral(outpoint, nHeight);
}

CSystemnode::CollateralStatus CSystemnode::CheckCollateral(const COutPoint& outpoint, int& nHeightRet)
{
    AssertLockHeld(cs_main);

    Coin coin;
    if (!GetUTXOCoin(outpoint, coin)) {
        return COLLATERAL_UTXO_NOT_FOUND;
    }

    if (coin.IsSpent()) {
        return COLLATERAL_UTXO_NOT_FOUND;
    }

    if (coin.out.nValue != Params().SystemnodeCollateral()) {
        return COLLATERAL_INVALID_AMOUNT;
    }

    nHeightRet = coin.nHeight;
    return COLLATERAL_OK;
}

//
// When a new systemnode broadcast is sent, update our information
//
bool CSystemnode::UpdateFromNewBroadcast(CSystemnodeBroadcast& snb, CConnman& connman)
{
    if (snb.sigTime > sigTime) {
        pubkey2 = snb.pubkey2;
        sigTime = snb.sigTime;
        sig = snb.sig;
        protocolVersion = snb.protocolVersion;
        addr = snb.addr;
        lastTimeChecked = 0;
        int nDoS = 0;
        if (snb.lastPing == CSystemnodePing() || (snb.lastPing != CSystemnodePing() && snb.lastPing.CheckAndUpdate(nDoS, connman, false))) {
            lastPing = snb.lastPing;
            snodeman.mapSeenSystemnodePing.insert(std::make_pair(lastPing.GetHash(), lastPing));
        }
        return true;
    }
    return false;
}

void CSystemnode::Check(bool forceCheck)
{
    if (ShutdownRequested())
        return;

    if (!forceCheck && (GetTime() - lastTimeChecked < SYSTEMNODE_CHECK_SECONDS))
        return;
    lastTimeChecked = GetTime();

    //once spent, stop doing the checks
    if (activeState == SYSTEMNODE_VIN_SPENT)
        return;

    if (!IsPingedWithin(SYSTEMNODE_REMOVAL_SECONDS)) {
        activeState = SYSTEMNODE_REMOVE;
        return;
    }

    if (!IsPingedWithin(SYSTEMNODE_EXPIRATION_SECONDS)) {
        activeState = SYSTEMNODE_EXPIRED;
        return;
    }

    //test if the collateral is still good
    if (!unitTest) {
        TRY_LOCK(cs_main, lockMain);
        if(!lockMain) return;
        CollateralStatus err = CheckCollateral(vin.prevout);
        if (err == COLLATERAL_UTXO_NOT_FOUND) {
            activeState = SYSTEMNODE_VIN_SPENT;
            LogPrint(BCLog::SYSTEMNODE, "CSystemnode::Check -- Failed to find Systemnode UTXO, systemnode=%s\n", vin.prevout.ToString());
            return;
        }
    }

    activeState = SYSTEMNODE_ENABLED; // OK
}

bool CSystemnode::IsValidNetAddr()
{
    if (Params().NetworkIDString() == CBaseChainParams::TESTNET)
        return true;
    // TODO: regtest is fine with any addresses for now,
    // should probably be a bit smarter if one day we start to implement tests for this
    return (addr.IsIPv4() && addr.IsRoutable());
}

int64_t CSystemnode::SecondsSincePayment() const
{
    CScript pubkeyScript;
    pubkeyScript = GetScriptForDestination(PKHash(pubkey));

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

int64_t CSystemnode::GetLastPaid() const
{
    CBlockIndex* pindexPrev = ::ChainActive().Tip();
    if (pindexPrev == nullptr)
        return false;

    CScript snpayee;
    snpayee = GetScriptForDestination(PKHash(pubkey));

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << vin;
    ss << sigTime;
    uint256 hash = ss.GetHash();

    // use a deterministic offset to break a tie -- 2.5 minutes
    int64_t nOffset = UintToArith256(hash).GetCompact(false) % 150; 

    if (::ChainActive().Tip() == NULL) return false;

    const CBlockIndex *BlockReading = ::ChainActive().Tip();

    int nMnCount = snodeman.CountEnabled()*1.25;
    int n = 0;
    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (n >= nMnCount) {
            return 0;
        }
        n++;

        if(systemnodePayments.mapSystemnodeBlocks.count(BlockReading->nHeight)){
            /*
                Search for this payee, with at least 2 votes. This will aid in consensus allowing the network 
                to converge on the same payees quickly, then keep the same schedule.
            */
            if(systemnodePayments.mapSystemnodeBlocks[BlockReading->nHeight].HasPayeeWithVotes(snpayee, 2)){
                return BlockReading->nTime + nOffset;
            }
        }

        if (BlockReading->pprev == nullptr) {
            assert(BlockReading);
            break;
        }
        BlockReading = BlockReading->pprev;
    }

    return 0;
}

// Find all blocks where SN received reward within defined block depth
// Used for generating stakepointers
bool CSystemnode::GetRecentPaymentBlocks(std::vector<const CBlockIndex*>& vPaymentBlocks, bool limitMostRecent) const
{
    vPaymentBlocks.clear();
    int nMinimumValidBlockHeight = ::ChainActive().Height() - Params().ValidStakePointerDuration();
    if (nMinimumValidBlockHeight < 1)
        nMinimumValidBlockHeight = 1;

    CBlockIndex* pindex = ::ChainActive()[nMinimumValidBlockHeight];

    CScript snpayee;
    snpayee = GetScriptForDestination(PKHash(pubkey));

    bool fBlockFound = false;
    while (::ChainActive().Next(pindex)) {
        CBlock block;
        if (!ReadBlockFromDisk(block, pindex, Params().GetConsensus()))
            continue;

        int m = (block.vtx[0]->nVersion >= TX_ELE_VERSION ? block.vtx[0]->vpout.size() : block.vtx[0]->vout.size());

        if (m > 2) {
            CTxOutAsset out = (block.vtx[0]->nVersion >= TX_ELE_VERSION ? block.vtx[0]->vpout[2] : block.vtx[0]->vout[2]);
            if (out.scriptPubKey == snpayee){
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

//
// CSystemnodeBroadcast
//

bool CSystemnodeBroadcast::CheckAndUpdate(int& nDos, CConnman& connman)
{
    nDos = 0;

    // make sure signature isn't in the future (past is OK)
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrint(BCLog::SYSTEMNODE, "snb - Signature rejected, too far into the future %s\n", vin.ToString());
        nDos = 1;
        return false;
    }

    if (protocolVersion < systemnodePayments.GetMinSystemnodePaymentsProto()) {
        LogPrint(BCLog::SYSTEMNODE, "snb - ignoring outdated systemnode %s protocol version %d\n", vin.ToString(), protocolVersion);
        return false;
    }

    CScript pubkeyScript;
    pubkeyScript = GetScriptForDestination(PKHash(pubkey));

    if (pubkeyScript.size() != 25) {
        LogPrint(BCLog::SYSTEMNODE, "snb - pubkey the wrong size\n");
        nDos = 100;
        return false;
    }

    CScript pubkeyScript2;
    pubkeyScript2 = GetScriptForDestination(PKHash(pubkey2));

    if (pubkeyScript2.size() != 25) {
        LogPrint(BCLog::SYSTEMNODE, "snb - pubkey2 the wrong size\n");
        nDos = 100;
        return false;
    }

    if (!vin.scriptSig.empty()) {
        LogPrint(BCLog::SYSTEMNODE, "snb - Ignore Not Empty ScriptSig %s\n", vin.ToString());
        return false;
    }

    // incorrect ping or its sigTime
    if (lastPing == CSystemnodePing() || !lastPing.CheckAndUpdate(nDos, connman, false, true))
        return false;

    std::string strMessage;
    std::string errorMessage = "";

    std::string vchPubKey(pubkey.begin(), pubkey.end());
    std::string vchPubKey2(pubkey2.begin(), pubkey2.end());
    strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) +
                 vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

    if(!legacySigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)){
        if (addr.ToString() != addr.ToString())
        {
            // maybe it's wrong format, try again with the old one
            strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) +
                            vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

            if(!legacySigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)){
                // didn't work either
                LogPrint(BCLog::NET, "snb - Got bad systemnode address signature, sanitized error: %s\n", SanitizeString(errorMessage));
                // there is a bug in old MN signatures, ignore such MN but do not ban the peer we got this from
                return false;
            }
        } else {
            // nope, sig is actually wrong
            LogPrint(BCLog::NET, "snb - Got bad systemnode address signature, sanitized error: %s\n", SanitizeString(errorMessage));
            // there is a bug in old MN signatures, ignore such MN but do not ban the peer we got this from
            return false;
        }
    }

    //! or do it like this for all nets...
    if (addr.GetPort() != Params().GetDefaultPort())
        return false;

    //search existing systemnode list, this is where we update existing Systemnodes with new snb broadcasts
    CSystemnode* psn = snodeman.Find(vin);

    // no such systemnode, nothing to update
    if (psn == nullptr)
        return true;

    // this broadcast is older or equal than the one that we already have - it's bad and should never happen
    // unless someone is doing something fishy
    // (mapSeensystemnodeBroadcast in CSystemnodeMan::ProcessMessage should filter legit duplicates)
    if (psn->sigTime >= sigTime) {
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodeBroadcast::CheckAndUpdate - Bad sigTime %d for Systemnode %20s %105s (existing broadcast is at %d)\n",
            sigTime, addr.ToString(), vin.ToString(), psn->sigTime);
        return false;
    }

    // systemnode is not enabled yet/already, nothing to update
    if (!psn->IsEnabled())
        return true;

    // sn.pubkey = pubkey, IsVinAssociatedWithPubkey is validated once below,
    //   after that they just need to match
    if (psn->pubkey == pubkey && !psn->IsBroadcastedWithin(SYSTEMNODE_MIN_SNB_SECONDS)) {
        //take the newest entry
        LogPrint(BCLog::SYSTEMNODE, "snb - Got updated entry for %s\n", addr.ToString());
        if (psn->UpdateFromNewBroadcast((*this), connman)) {
            psn->Check();
            if (psn->IsEnabled())
                Relay(connman);
        }
        systemnodeSync.AddedSystemnodeList(GetHash());
    }

    return true;
}

bool CSystemnodeBroadcast::CheckInputsAndAdd(int& nDoS, CConnman& connman)
{
    // we are a systemnode with the same vin (i.e. already activated) and this snb is ours (matches our systemnode privkey)
    // so nothing to do here for us
    if (fSystemNode && vin.prevout == activeSystemnode.vin.prevout && pubkey2 == activeSystemnode.pubKeySystemnode)
        return true;

    // incorrect ping or its sigTime
    if (lastPing == CSystemnodePing() || !lastPing.CheckAndUpdate(nDoS, connman, false, true))
        return false;

    // search existing systemnode list
    CSystemnode* psn = snodeman.Find(vin);

    if (psn) {
        // nothing to do here if we already know about this systemnode and it's enabled
        if (psn->IsEnabled())
            return true;
        // if it's not enabled, remove old MN first and continue
        else
            snodeman.Remove(psn->vin);
    }

    if(GetUTXOConfirmations(vin.prevout) < SYSTEMNODE_MIN_CONFIRMATIONS){
        LogPrint(BCLog::NET, "snb - Input must have at least %d confirmations\n", SYSTEMNODE_MIN_CONFIRMATIONS);
        // maybe we miss few blocks, let this snb to be checked again later
        snodeman.mapSeenSystemnodeBroadcast.erase(GetHash());
        systemnodeSync.mapSeenSyncSNB.erase(GetHash());
        return false;
    }

    // verify that sig time is legit in past
    // should be at least not earlier than block when 10000 CRW tx got SYSTEMNODE_MIN_CONFIRMATIONS
    uint256 hashBlock = uint256();
    CBlockIndex* blockindex = nullptr;
    CTransactionRef tx2 = GetTransaction(blockindex, nullptr, vin.prevout.hash, Params().GetConsensus(), hashBlock);;
    BlockMap::iterator mi = g_chainman.m_blockman.m_block_index.find(hashBlock);
    if (mi != g_chainman.m_blockman.m_block_index.end() && (*mi).second)
    {
        CBlockIndex* pMNIndex = (*mi).second; // block for 10000 CRW tx -> 1 confirmation
        CBlockIndex* pConfIndex = ::ChainActive()[pMNIndex->nHeight + SYSTEMNODE_MIN_CONFIRMATIONS - 1]; // block where tx got SYSTEMNODE_MIN_CONFIRMATIONS
        if(pConfIndex->GetBlockTime() > sigTime)
        {
            LogPrint(BCLog::NET, "snb - Bad sigTime %d for systemnode %20s %105s (%i conf block is at %d)\n",
                      sigTime, addr.ToString(), vin.ToString(), SYSTEMNODE_MIN_CONFIRMATIONS, pConfIndex->GetBlockTime());
            return false;
        }
    }

    LogPrint(BCLog::SYSTEMNODE, "snb - Got NEW systemnode entry - %s - %s - %s - %lli \n", GetHash().ToString(), addr.ToString(), vin.ToString(), sigTime);
    CSystemnode sn(*this);
    snodeman.Add(sn);

    // if it matches our systemnode privkey, then we've been remotely activated
    if (pubkey2 == activeSystemnode.pubKeySystemnode && protocolVersion == PROTOCOL_VERSION) {
        activeSystemnode.EnableHotColdSystemNode(vin, addr);
        if (!vchSignover.empty()) {
            if (pubkey.Verify(pubkey2.GetHash(), vchSignover)) {
                LogPrint(BCLog::SYSTEMNODE, "%s: Verified pubkey2 signover for staking, added to activesystemnode\n", __func__);
                activeSystemnode.vchSigSignover = vchSignover;
            } else {
                LogPrint(BCLog::SYSTEMNODE, "%s: Failed to verify pubkey on signover!\n", __func__);
            }
        } else {
            LogPrint(BCLog::SYSTEMNODE, "%s: NOT SIGNOVER!\n", __func__);
        }
    }

    bool isLocal = addr.IsRFC1918() || addr.IsLocal();
    if (Params().NetworkIDString() == CBaseChainParams::REGTEST)
        isLocal = false;

    if (!isLocal)
        Relay(connman);

    return true;
}

void CSystemnodeBroadcast::Relay(CConnman& connman) const
{
    CInv inv(MSG_SYSTEMNODE_ANNOUNCE, GetHash());
    connman.RelayInv(inv);
}

CSystemnodeBroadcast::CSystemnodeBroadcast()
{
    vin = CTxIn();
    addr = CService();
    pubkey = CPubKey();
    pubkey2 = CPubKey();
    sig = std::vector<unsigned char>();
    vchSignover = std::vector<unsigned char>();
    activeState = SYSTEMNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CSystemnodePing();
    unitTest = false;
    protocolVersion = PROTOCOL_VERSION;
}

CSystemnodeBroadcast::CSystemnodeBroadcast(CService newAddr, CTxIn newVin, CPubKey newPubkey, CPubKey newPubkey2, int protocolVersionIn)
{
    vin = newVin;
    addr = newAddr;
    pubkey = newPubkey;
    pubkey2 = newPubkey2;
    sig = std::vector<unsigned char>();
    vchSignover = std::vector<unsigned char>();
    activeState = SYSTEMNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CSystemnodePing();
    unitTest = false;
    protocolVersion = protocolVersionIn;
}

CSystemnodeBroadcast::CSystemnodeBroadcast(const CSystemnode& sn)
{
    vin = sn.vin;
    addr = sn.addr;
    pubkey = sn.pubkey;
    pubkey2 = sn.pubkey2;
    sig = sn.sig;
    vchSignover = sn.vchSignover;
    activeState = sn.activeState;
    sigTime = sn.sigTime;
    lastPing = sn.lastPing;
    unitTest = sn.unitTest;
    protocolVersion = sn.protocolVersion;
}

bool CSystemnodeBroadcast::Create(std::string strService, std::string strKeySystemnode, std::string strTxHash, std::string strOutputIndex, std::string& strErrorMessage, CSystemnodeBroadcast& snb, bool fOffline)
{
    CTxIn txin;
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;
    CPubKey pubKeySystemnodeNew;
    CKey keySystemnodeNew;

    if (!gArgs.GetBoolArg("-jumpstart", false)) {
        //need correct blocks to send ping
        if (!fOffline && !systemnodeSync.IsBlockchainSynced()) {
            strErrorMessage = "Sync in progress. Must wait until sync is complete to start Systemnode";
            LogPrint(BCLog::SYSTEMNODE, "CSystemnodeBroadcast::Create -- %s\n", strErrorMessage);
            return false;
        }
    }

    if (!legacySigner.SetKey(strKeySystemnode, keySystemnodeNew, pubKeySystemnodeNew)) {
        strErrorMessage = strprintf("Can't find keys for systemnode %s - %s", strService, strErrorMessage);
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    if(!GetWallets()[0]->GetSystemnodeVinAndKeys(txin, pubKeyCollateralAddress, keyCollateralAddress, strTxHash, strOutputIndex)) {
        strErrorMessage = strprintf("Could not allocate txin for systemnode");
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    int age = GetUTXOConfirmations(txin.prevout);
    if (age < SYSTEMNODE_MIN_CONFIRMATIONS)
    {
        strErrorMessage = strprintf("Input must have at least %d confirmations. Now it has %d",
                                     SYSTEMNODE_MIN_CONFIRMATIONS, age);
        LogPrint(BCLog::NET, "CSystemnodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    CService service = CService(LookupNumeric(strService, Params().GetDefaultPort()));
    if(Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if(service.GetPort() != 9340) {
            strErrorMessage = strprintf("Invalid port %u for systemnode %s - only 9340 is supported on mainnet.", service.GetPort(), strService);
            LogPrint(BCLog::SYSTEMNODE, "CSystemnodeBroadcast::Create -- %s\n", strErrorMessage);
            return false;
        }
    } else if(service.GetPort() == 9340) {
        strErrorMessage = strprintf("Invalid port %u for systemnode %s - 9340 is only supported on mainnet.", service.GetPort(), strService);
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    bool fSignOver = true;
    return Create(txin, service, keyCollateralAddress, pubKeyCollateralAddress, keySystemnodeNew, pubKeySystemnodeNew, fSignOver, strErrorMessage, snb);
}

bool CSystemnodeBroadcast::Create(CTxIn txin, CService service, CKey keyCollateralAddress, CPubKey pubKeyCollateralAddress, CKey keySystemnodeNew, CPubKey pubKeySystemnodeNew, bool fSignOver, std::string &strErrorMessage, CSystemnodeBroadcast &snb) {
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    CSystemnodePing snp(txin);
    if(!snp.Sign(keySystemnodeNew, pubKeySystemnodeNew)){
        strErrorMessage = strprintf("Failed to sign ping, txin: %s", txin.ToString());
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodeBroadcast::Create --  %s\n", strErrorMessage);
        snb = CSystemnodeBroadcast();
        return false;
    }

    snb = CSystemnodeBroadcast(service, txin, pubKeyCollateralAddress, pubKeySystemnodeNew, PROTOCOL_VERSION);

    if(!snb.IsValidNetAddr()) {
        strErrorMessage = strprintf("Invalid IP address, systemnode=%s", txin.prevout.ToStringShort());
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodeBroadcast::Create -- %s\n", strErrorMessage);
        snb = CSystemnodeBroadcast();
        return false;
    }

    snb.lastPing = snp;
    if(!snb.Sign(keyCollateralAddress)){
        strErrorMessage = strprintf("Failed to sign broadcast, txin: %s", txin.ToString());
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodeBroadcast::Create -- %s\n", strErrorMessage);
        snb = CSystemnodeBroadcast();
        return false;
    }

    //Additional signature for use in proof of stake
    if (fSignOver) {
        if (!keyCollateralAddress.Sign(pubKeySystemnodeNew.GetHash(), snb.vchSignover)) {
            LogPrint(BCLog::SYSTEMNODE, "CSystemnodeBroadcast::Create failed signover\n");
            snb = CSystemnodeBroadcast();
            return false;
        }
        LogPrint(BCLog::SYSTEMNODE, "%s: Signed over to key %s\n", __func__, EncodeDestination(PKHash(pubKeySystemnodeNew)));
    }

    return true;
}
bool CSystemnodeBroadcast::Sign(const CKey& keyCollateralAddress)
{
    std::string errorMessage;

    std::string vchPubKey(pubkey.begin(), pubkey.end());
    std::string vchPubKey2(pubkey2.begin(), pubkey2.end());

    sigTime = GetAdjustedTime();

    std::string strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

    if (!legacySigner.SignMessage(strMessage, sig, keyCollateralAddress)) {
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodeBroadcast::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}

bool CSystemnodeBroadcast::VerifySignature() const
{
    std::string errorMessage;

    std::string vchPubKey(pubkey.begin(), pubkey.end());
    std::string vchPubKey2(pubkey2.begin(), pubkey2.end());

    std::string strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

    if(!legacySigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)) {
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodeBroadcast::VerifySignature() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}

CSystemnodePing::CSystemnodePing()
{
    vin = CTxIn();
    blockHash = uint256();
    sigTime = 0;
    vchSig = std::vector<unsigned char>();
}

CSystemnodePing::CSystemnodePing(const CTxIn& newVin)
{
    vin = newVin;
    blockHash = ::ChainActive()[::ChainActive().Height() - 12]->GetBlockHash();
    sigTime = GetAdjustedTime();
    vchSig = std::vector<unsigned char>();
}

bool CSystemnodePing::Sign(const CKey& keySystemnode, const CPubKey& pubKeySystemnode)
{
    std::string errorMessage;
    std::string strThroNeSignMessage;
    sigTime = GetAdjustedTime();
    std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);

    if (!legacySigner.SignMessage(strMessage, vchSig, keySystemnode)) {
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodePing::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    if (!legacySigner.VerifyMessage(pubKeySystemnode, vchSig, strMessage, errorMessage)) {
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodePing::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}

bool CSystemnodePing::VerifySignature(const CPubKey& pubKeySystemnode, int &nDos) const
{
    std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);
    std::string errorMessage = "";

    if(!legacySigner.VerifyMessage(pubKeySystemnode, vchSig, strMessage, errorMessage))
    {
        LogPrint(BCLog::NET, "CSystemnodePing::VerifySignature - Got bad Systemnode ping signature %s Error: %s\n", vin.ToString(), errorMessage);
        nDos = 33;
        return false;
    }
    return true;
}

bool CSystemnodePing::CheckAndUpdate(int& nDos, CConnman& connman, bool fRequireEnabled, bool fCheckSigTimeOnly)
{
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodePing::CheckAndUpdate - Signature rejected, too far into the future %s\n", vin.ToString());
        nDos = 1;
        return false;
    }

    if (sigTime <= GetAdjustedTime() - 60 * 60) {
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodePing::CheckAndUpdate - Signature rejected, too far into the past %s - %d %d \n", vin.ToString(), sigTime, GetAdjustedTime());
        nDos = 1;
        return false;
    }

    if (fCheckSigTimeOnly) {
        CSystemnode* psn = snodeman.Find(vin);
        if (psn)
            return VerifySignature(psn->pubkey2, nDos);
        return true;
    }

    LogPrint(BCLog::SYSTEMNODE, "CSystemnodePing::CheckAndUpdate - New Ping - %s - %s - %lli\n", GetHash().ToString(), blockHash.ToString(), sigTime);

    // see if we have this Systemnode
    CSystemnode* psn = snodeman.Find(vin);
    if(psn && psn->protocolVersion >= systemnodePayments.GetMinSystemnodePaymentsProto())
    {
        if (fRequireEnabled && !psn->IsEnabled()) return false;

        // LogPrint(BCLog::NET, "snping - Found corresponding sn for vin: %s\n", vin.ToString());
        // update only if there is no known ping for this systemnode or
        // last ping was more then SYSTEMNODE_MIN_MNP_SECONDS-60 ago comparing to this one
        if(!psn->IsPingedWithin(SYSTEMNODE_MIN_SNP_SECONDS - 60, sigTime))
        {
            if(!VerifySignature(psn->pubkey2, nDos))
                return false;

            BlockMap::iterator mi = g_chainman.m_blockman.m_block_index.find(blockHash);
            if (mi != g_chainman.m_blockman.m_block_index.end() && (*mi).second)
            {
                if((*mi).second->nHeight < ::ChainActive().Height() - 24)
                {
                    LogPrint(BCLog::NET, "CSystemnodePing::CheckAndUpdate - Systemnode %s block hash %s is too old\n", vin.ToString(), blockHash.ToString());
                    // Do nothing here (no Systemnode update, no snping relay)
                    // Let this node to be visible but fail to accept snping

                    return false;
                }
            } else {
                LogPrint(BCLog::SYSTEMNODE, "CSystemnodePing::CheckAndUpdate - Systemnode %s block hash %s is unknown\n", vin.ToString(), blockHash.ToString());
                // maybe we stuck so we shouldn't ban this node, just fail to accept it
                // TODO: or should we also request this block?

                return false;
            }

            psn->lastPing = *this;

            //snodeman.mapSeenSystemnodeBroadcast.lastPing is probably outdated, so we'll update it
            CSystemnodeBroadcast snb(*psn);
            uint256 hash = snb.GetHash();
            if (snodeman.mapSeenSystemnodeBroadcast.count(hash)) {
                snodeman.mapSeenSystemnodeBroadcast[hash].lastPing = *this;
            }

            psn->Check(true);
            if (!psn->IsEnabled())
                return false;

            LogPrint(BCLog::SYSTEMNODE, "CSystemnodePing::CheckAndUpdate - Systemnode ping accepted, vin: %s\n", vin.ToString());

            Relay(connman);
            return true;
        }
        LogPrint(BCLog::SYSTEMNODE, "CSystemnodePing::CheckAndUpdate - Systemnode ping arrived too early, vin: %s\n", vin.ToString());
        //nDos = 1; //disable, this is happening frequently and causing banned peers
        return false;
    }
    LogPrint(BCLog::SYSTEMNODE, "CSystemnodePing::CheckAndUpdate - Couldn't find compatible Systemnode entry, vin: %s\n", vin.ToString());

    return false;
}

void CSystemnodePing::Relay(CConnman& connman)
{
    CInv inv(MSG_SYSTEMNODE_PING, GetHash());
    connman.RelayInv(inv);
}

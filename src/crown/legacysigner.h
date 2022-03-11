// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LEGACYSIGNER_H
#define LEGACYSIGNER_H

#include <masternode/activemasternode.h>
#include <masternode/masternode-payments.h>
#include <masternode/masternode-sync.h>
#include <masternode/masternodeman.h>
#include <sync.h>
#include <systemnode/activesystemnode.h>
#include <systemnode/systemnode-payments.h>
#include <systemnode/systemnode-sync.h>
#include <systemnode/systemnodeman.h>
#include <validation.h>

class CTxIn;
class CLegacySigner;
class CMasterNodeVote;
class CCrownAddress;
class CActiveMasternode;

// status update message constants
#define MASTERNODE_ACCEPTED 1
#define MASTERNODE_REJECTED 0
#define MASTERNODE_RESET -1

extern CLegacySigner legacySigner;
extern std::string strMasterNodePrivKey;

/** Helper object for signing and checking signatures
 */
class CLegacySigner
{
public:
    void InitCollateralAddress(){
        SetCollateralAddress(Params().LegacySignerDummyAddress());
    }
    bool IsVinAssociatedWithPubkey(CTxIn& vin, CPubKey& pubkey, int nodeType, const Consensus::Params& consensusParams);
    bool SetCollateralAddress(std::string strAddress);
    /// Set the private/public key values, returns true if successful
    bool SetKey(std::string strSecret, CKey& key, CPubKey& pubkey);
    /// Sign the message, returns true if successful
    static bool SignMessage(const std::string& strMessage, std::vector<unsigned char>& vchSigRet, const CKey& key);
    /// Verify the message signature, returns true if succcessful
    static bool VerifyMessage(const CPubKey& pubkey, const std::vector<unsigned char>& vchSig, const std::string& strMessage, std::string& strErrorRet);
    /// Verify the message signature, returns true if succcessful
    static bool VerifyMessage(const CKeyID& keyID, const std::vector<unsigned char>& vchSig, const std::string& strMessage, std::string& strErrorRet);
    // where collateral should be made out to
    CScript collateralPubKey;
    CMasternode* pSubmittedToMasternode;
    CSystemnode* pSubmittedToSystemnode;
};

/** Helper class for signing hashes and checking their signatures
 */
class CHashSigner
{
public:
    /// Sign the hash, returns true if successful
    static bool SignHash(const uint256& hash, const CKey& key, std::vector<unsigned char>& vchSigRet);
    /// Verify the hash signature, returns true if succcessful
    static bool VerifyHash(const uint256& hash, const CPubKey& pubkey, const std::vector<unsigned char>& vchSig, std::string& strErrorRet);
    /// Verify the hash signature, returns true if succcessful
    static bool VerifyHash(const uint256& hash, const CKeyID& keyID, const std::vector<unsigned char>& vchSig, std::string& strErrorRet);
};

#endif

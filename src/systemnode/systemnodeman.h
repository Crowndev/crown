// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSTEMNODEMAN_H
#define SYSTEMNODEMAN_H

#include <sync.h>
#include <net.h>
#include <key.h>
#include <util/system.h>
#include <base58.h>
#include <validation.h>
#include <systemnode/systemnode.h>

#define SYSTEMNODES_DUMP_SECONDS (15 * 60)
#define SYSTEMNODES_DSEG_SECONDS (3 * 60 * 60)


class CSystemnodeMan;

extern CSystemnodeMan snodeman;

class CSystemnodeMan {
private:
    // critical section to protect the inner data structures
    mutable RecursiveMutex cs;

    // critical section to protect the inner data structures specifically on messaging
    mutable RecursiveMutex cs_process_message;

    // map to hold all SNs
    std::vector<CSystemnode> vSystemnodes;
    // who's asked for the Systemnode list and the last time
    std::map<CNetAddr, int64_t> mAskedUsForSystemnodeList;
    // who we asked for the Systemnode list and the last time
    std::map<CNetAddr, int64_t> mWeAskedForSystemnodeList;
    // which Systemnodes we've asked for
    std::map<COutPoint, int64_t> mWeAskedForSystemnodeListEntry;

    // these maps are used for Systemnode recovery from SYSTEMNODE_NEW_START_REQUIRED state
    std::map<uint256, std::pair<int64_t, std::set<CNetAddr>>> mMnbRecoveryRequests;
    std::map<uint256, std::vector<CSystemnodeBroadcast>> mMnbRecoveryGoodReplies;
    std::list<std::pair<CService, uint256>> listScheduledMnbRequestConnections;

    /// Set when Systemnodes are added, cleared when CGovernanceManager is notified
    bool fSystemnodesAdded;

    /// Set when Systemnodes are removed, cleared when CGovernanceManager is notified
    bool fSystemnodesRemoved;

public:
    // Keep track of all broadcasts I've seen
    std::map<uint256, CSystemnodeBroadcast> mapSeenSystemnodeBroadcast;
    // Keep track of all pings I've seen
    std::map<uint256, CSystemnodePing> mapSeenSystemnodePing;

    SERIALIZE_METHODS(CSystemnodeMan, obj)
    {
        LOCK(obj.cs);

        READWRITE(obj.vSystemnodes);
        READWRITE(obj.mAskedUsForSystemnodeList);
        READWRITE(obj.mWeAskedForSystemnodeList);
        READWRITE(obj.mWeAskedForSystemnodeListEntry);
        READWRITE(obj.mapSeenSystemnodeBroadcast);
        READWRITE(obj.mapSeenSystemnodePing);
    }

    //CSystemnodeMan();
    //CSystemnodeMan(CSystemnodeMan& other);

    /// Return total sn
    int CountSystemnodes(bool fEnabled = false);

    /// Add an entry
    bool Add(CSystemnode& mn);

    /// Ask (source) node for mnb
    void AskForSN(CNode* pnode, CTxIn& vin, CConnman& connman);

    /// Check all Systemnodes
    void Check();

    /// Check all Systemnodes and remove inactive
    void CheckAndRemove(bool forceExpiredRemoval = false);

    /// Clear Systemnode vector
    void Clear();

    int CountEnabled(int protocolVersion = -1);

    void DsegUpdate(CNode* pnode, CConnman& connman);

    /// Find an entry
    CSystemnode* Find(const CTxIn& vin);
    CSystemnode* Find(const CPubKey& pubKeySystemnode);
    CSystemnode* Find(const CService& addr);

    /// Find an entry in the systemnode list that is next to be paid
    CSystemnode* GetNextSystemnodeInQueueForPayment(int nBlockHeight, bool fFilterSigTime, int& nCount);

    /// Get the current winner for this block
    CSystemnode* GetCurrentSystemNode(int mod = 1, int64_t nBlockHeight = 0, int minProtocol = 0);

    std::vector<CSystemnode> GetFullSystemnodeVector()
    {
        Check();
        return vSystemnodes;
    }

    std::vector<std::pair<int, CSystemnode>> GetSystemnodeRanks(int64_t nBlockHeight, int minProtocol = 0);
    int GetSystemnodeRank(const CTxIn& vin, int64_t nBlockHeight, int minProtocol = 0, bool fOnlyActive = true);

    void ProcessSystemnodeConnections(CConnman& connman);

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman);

    /// Return the number of (unique) Systemnodes
    int size() { return vSystemnodes.size(); }

    std::string ToString() const;

    void Remove(CTxIn vin);

    /// Update systemnode list and maps using provided CSystemnodeBroadcast
    void UpdateSystemnodeList(CSystemnodeBroadcast snb, CConnman& connman);

    /// Perform complete check and only then update list and maps
    bool CheckSnbAndUpdateSystemnodeList(CSystemnodeBroadcast snb, int& nDos, CConnman& connman);
};

#endif

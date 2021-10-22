// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_GOVERNANCE_VOTE_H
#define CROWN_GOVERNANCE_VOTE_H

#include <pubkey.h>
#include <primitives/transaction.h>
#include <platform/governance.h>
#include <univalue.h>

class CBlockIndex;
class TxValidationState;

namespace Platform
{
    class VoteTx
    {
    public:
        static const int CURRENT_VERSION = 1;

    public:
        VoteTx(Vote vote, int64_t electionCode)
            : voterId(vote.VoterId())
            , electionCode(electionCode)
            , vote(ConvertVote(vote.Value()))
            , time(vote.Time())
            , candidate(vote.Candidate())
        {}

        VoteTx() = default;

        CTxIn voterId;
        int64_t electionCode;
        int64_t vote;
        int64_t time;
        uint256 candidate;
        CKeyID keyId;
        std::vector<unsigned char> signature;

        static int64_t ConvertVote(VoteValue vote);
        static VoteValue ConvertVote(int64_t vote);
        Vote GetVote() const;

    public:
        SERIALIZE_METHODS(VoteTx, obj)
        {
            READWRITE(obj.voterId);
            READWRITE(obj.electionCode);
            READWRITE(obj.vote);
            READWRITE(obj.candidate);
            READWRITE(obj.keyId);
            if (!(s.GetType() & SER_GETHASH))
            {
                READWRITE(obj.signature);
            }
        }

        std::string ToString() const;
        void ToJson(UniValue& result) const;
    };



    bool CheckVoteTx(const CTransaction& tx, const CBlockIndex* pindex, TxValidationState& state);
    bool ProcessVoteTx(const CTransaction& tx, const CBlockIndex* pindex, TxValidationState& state);
}

#endif //PROJECT_GOVERNANCE_VOTE_H

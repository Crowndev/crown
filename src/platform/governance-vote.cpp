// Copyright (c) 2014-2018 Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "governance-vote.h"
#include "agent.h"
#include "specialtx.h"
#include "consensus/validation.h"
#include <univalue/include/univalue.h>
#include "key_io.h"

#include "sync.h"

namespace Platform
{
    bool CheckVoteTx(const CTransaction& tx, const CBlockIndex* pindex, TxValidationState& state)
    {
        AssertLockHeld(cs_main);

        VoteTx vtx;
        if (!GetNftTxPayload(tx, vtx))
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-tx-payload");

        if (!CheckInputsHashAndSig(tx, vtx, vtx.keyId, state))
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-vote-tx-invalid-signature");

        return true;
    }

    bool ProcessVoteTx(const CTransaction& tx, const CBlockIndex* pindex, TxValidationState& state)
    {
        try
        {
            VoteTx vtx;
            GetNftTxPayload(tx, vtx);

            CMasternode* pmn = mnodeman.Find(vtx.voterId);
            if(pmn == nullptr)
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "vote-tx-process-fail-no-masternode");

            if (pmn->pubkey2.GetID() != vtx.keyId)
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "vote-tx-process-fail-signed-by-wrong-key");

            AgentsVoting().AcceptVote(vtx.GetVote());

            return true;
        }
        catch (const std::exception& )
        {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "vote-tx-process-fail");
        }

    }

    int64_t VoteTx::ConvertVote(VoteValue vote)
    {
        // Replace with more explicit serialization
        return static_cast<int64_t>(vote);
    }

    VoteValue VoteTx::ConvertVote(int64_t vote)
    {
        // Replace with more explicit serialization
        return static_cast<VoteValue>(vote);
    }

    Vote VoteTx::GetVote() const
    {
        return { candidate, ConvertVote(vote), time, voterId };
    }

    void VoteTx::ToJson(UniValue& result) const
    {
        result.pushKV("voterId", voterId.ToString());
        result.pushKV("electionCode", electionCode);
        result.pushKV("vote", vote);
        result.pushKV("time", time);
        result.pushKV("candidate", candidate.ToString());
        result.pushKV("keyId", EncodeDestination(PKHash(keyId)));
    }

    std::string VoteTx::ToString() const
    {
        return strprintf(
            "VoteTx(candidate=%s,vote=%d,time=%d,voter=%s,election=%d)",
            candidate.ToString(),vote, time, voterId.ToString(),electionCode
        );

    }
}

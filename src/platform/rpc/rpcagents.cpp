#include <rpc/server.h>
#include <rpc/protocol.h>
#include <masternode/masternodeconfig.h>
#include <key.h>
#include <crown/legacysigner.h>
#include <primitives/transaction.h>
#include <platform/agent.h>
#include <platform/governance-vote.h>
#include <platform/governance.h>
#include <platform/specialtx.h>
#include <platform/rpc/specialtx-rpc-utils.h>

namespace Platform
{
    UniValue SendVotingTransaction(
        const CPubKey& pubKeyMasternode,
        const CKey& keyMasternode,
        const CTxIn& vin,
        Platform::VoteValue voteValue,
        const uint256& hash
    )
    {
        try
        {
            UniValue statusObj(UniValue::VOBJ);

            CMutableTransaction tx;
            tx.nVersion = 3;
            tx.nType = TRANSACTION_GOVERNANCE_VOTE;

            auto vote = Platform::Vote{hash, voteValue, GetTime(), vin};
            auto voteTx = Platform::VoteTx(vote, 1);

            FundSpecialTx(tx, voteTx);
            voteTx.keyId = keyMasternode.GetPubKey().GetID();
            SignSpecialTxPayload(tx, voteTx, keyMasternode);
            SetNftTxPayload(tx, voteTx);

            std::string result = SignAndSendSpecialTx(tx);

            statusObj.pushKV("result", result);
            return statusObj;
        }
        catch(const std::exception& ex)
        {
            UniValue statusObj(UniValue::VOBJ);
            statusObj.pushKV("result", "failed");
            statusObj.pushKV("errorMessage", ex.what());
            return statusObj;
        }
    }

    UniValue CastSingleVote(const uint256& hash, Platform::VoteValue vote, const CNodeEntry& mne)
    {
        std::string errorMessage;
        std::vector<unsigned char> vchMasterNodeSignature;
        std::string strMasterNodeSignMessage;

        CKey keyCollateralAddress;
        CPubKey pubKeyMasternode;
        CKey keyMasternode;

            UniValue statusObj(UniValue::VOBJ);

        if(!legacySigner.SetKey(mne.getPrivKey(), keyMasternode, pubKeyMasternode)){
            statusObj.pushKV("result", "failed");
            statusObj.pushKV("errorMessage", "Masternode signing error, could not set key correctly: " + errorMessage);
            return statusObj;
        }

        CMasternode* pmn = mnodeman.Find(pubKeyMasternode);
        if(pmn == nullptr)
        {
            statusObj.pushKV("result", "failed");
            statusObj.pushKV("errorMessage", "Can't find masternode by pubkey");
            return statusObj;
        }

        assert(pmn->pubkey2 == pubKeyMasternode);

        return SendVotingTransaction(pubKeyMasternode, keyMasternode, pmn->vin, vote, hash);
    }

    Platform::VoteValue ParseVote(const std::string& voteText)
    {
        if(voteText != "yes" && voteText != "no")
            throw std::runtime_error("You can only vote 'yes' or 'no'");

        else if(voteText == "yes")
            return Platform::VoteValue::yes;
        else
            return Platform::VoteValue::no;
    }
}

UniValue vote(const JSONRPCRequest& request)
{
    if (request.params.size() != 3)
        throw std::runtime_error("Correct usage is 'mnbudget vote proposal-hash yes|no'");

    auto hash = ParseHashV(request.params[1], "parameter 1");
    auto nVote = Platform::ParseVote(request.params[2].get_str());

    auto success = 0;
    auto failed = 0;
    UniValue resultsObj(UniValue::VOBJ);

    for(const auto& mne: masternodeConfig.getEntries())
    {
        auto statusObj = CastSingleVote(hash, nVote, mne);
        std::string result = find_value(statusObj, "result").get_str();
        if (result == "failed")
            ++failed;
        else
            ++success;

        resultsObj.pushKV(mne.getAlias(), statusObj);

    }
    UniValue returnObj(UniValue::VOBJ);

    returnObj.pushKV("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed));
    returnObj.pushKV("detail", resultsObj);

    return returnObj;
}

UniValue lists(const JSONRPCRequest& request)
{
    UniValue result(UniValue::VARR);

    for (const auto& agent: Platform::GetAgentRegistry())
    {
        result.push_back(agent.id.ToString());
    }

    return result;
}


Span<const CRPCCommand> GetAgentsRPCCommands()
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                                actor (function)                argNames
    //  --------------------- ------------------------          -----------------------         ----------
    { "nft",                "vote",                             &vote,                          {"txid"} },
    { "nft",                "lists",                             &lists,                          {} },
};
// clang-format on
    return MakeSpan(commands);
}

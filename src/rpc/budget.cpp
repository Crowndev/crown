// Copyright (c) 2014-2018 The Dash developers
// Copyright (c) 2014-2021 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <core_io.h>
#include <crown/legacysigner.h>
#include <crown/nodewallet.h>
#include <db.h>
#include <init.h>
#include <key_io.h>
#include <masternode/activemasternode.h>
#include <masternode/masternode-budget.h>
#include <masternode/masternode-payments.h>
#include <masternode/masternode-sync.h>
#include <masternode/masternodeconfig.h>
#include <masternode/masternodeman.h>
#include <node/context.h>
#include <nodeconfig.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <util/moneystr.h>
#include <util/system.h>
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>

#include <univalue.h>

#include <fstream>

UniValue mnbudget(const JSONRPCRequest& request)
{
    std::string strCommand;
    if (request.params.size() >= 1)
        strCommand = request.params[0].get_str();

    if (request.fHelp || (strCommand != "vote-many" && strCommand != "prepare" && strCommand != "submit" && strCommand != "vote" && strCommand != "getvotes" && strCommand != "getinfo" && strCommand != "show" && strCommand != "projection" && strCommand != "check" && strCommand != "nextblock"))
        throw runtime_error(
            "mnbudget \"command\"... ( \"passphrase\" )\n"
            "Vote or show current budgets\n"
            "\nAvailable commands:\n"
            "  prepare            - Prepare proposal for network by signing and creating tx\n"
            "  submit             - Submit proposal for network\n"
            "  vote-many          - Vote on a Crown initiative\n"
            "  vote-alias         - Vote on a Crown initiative\n"
            "  vote               - Vote on a Crown initiative/budget\n"
            "  getvotes           - Show current masternode budgets\n"
            "  getinfo            - Show current masternode budgets\n"
            "  show               - Show all budgets\n"
            "  projection         - Show the projection of which proposals will be paid the next cycle\n"
            "  check              - Scan proposals and remove invalid\n"
            "  nextblock          - Get next superblock for budget system\n");

    if (strCommand == "nextblock") {
        CBlockIndex* pindexPrev = ::ChainActive().Tip();
        if (!pindexPrev)
            return "unknown";

        return GetNextSuperblock(pindexPrev->nHeight);
    }

    if (strCommand == "prepare") {
        int nBlockMin = 0;
        CBlockIndex* pindexPrev = ::ChainActive().Tip();

        if (request.params.size() != 7)
            throw runtime_error("Correct usage is 'mnbudget prepare proposal-name url payment_count block_start crown_address monthly_payment_crown'");

        std::string strProposalName = request.params[1].get_str();
        if (strProposalName.size() > 20)
            return "Invalid proposal name, limit of 20 characters.";

        std::string strURL = request.params[2].get_str();
        if (strURL.size() > 64)
            return "Invalid url, limit of 64 characters.";

        int nPaymentCount = request.params[3].get_int();
        if (nPaymentCount < 1)
            return "Invalid payment count, must be more than zero.";

        //set block min
        if (pindexPrev != nullptr)
            nBlockMin = pindexPrev->nHeight - GetBudgetPaymentCycleBlocks() * (nPaymentCount + 1);

        int nBlockStart = request.params[4].get_int();
        if (nBlockStart % GetBudgetPaymentCycleBlocks() != 0) {
            const int nextBlock = GetNextSuperblock(pindexPrev->nHeight);
            return strprintf("Invalid block start - must be a budget cycle block. Next valid block: %d", nextBlock);
        }

        int nBlockEnd = nBlockStart + GetBudgetPaymentCycleBlocks() * nPaymentCount;

        if (nBlockStart < nBlockMin)
            return "Invalid block start, must be more than current height.";

        if (nBlockEnd < pindexPrev->nHeight)
            return "Invalid ending block, starting block + (payment_cycle*payments) must be more than current height.";

        if (pindexPrev) {
            const int64_t submissionBlock = pindexPrev->nHeight + BUDGET_FEE_CONFIRMATIONS + 1;
            const int64_t nextBlock = GetNextSuperblock(pindexPrev->nHeight);
            if (submissionBlock >= nextBlock - GetVotingThreshold())
                return "Sorry, your proposal can not be added as we're too close to the proposal payment. Please submit your proposal for the next proposal payment.";
        }

        CTxDestination address = DecodeDestination(request.params[5].get_str());
        if (!IsValidDestination(address))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Crown address");

        // Parse Crown address
        CScript scriptPubKey = GetScriptForDestination(address);
        CAmount nAmount = AmountFromValue(request.params[6]);

        //*************************************************************************

        // create transaction 15 minutes into the future, to allow for confirmation time
        CBudgetProposalBroadcast budgetProposalBroadcast(strProposalName, strURL, nPaymentCount, scriptPubKey, nAmount, nBlockStart, uint256());

        std::string strError = "";
        if (!budgetProposalBroadcast.IsValid(strError, false))
            return "Proposal is not valid - " + budgetProposalBroadcast.GetHash().ToString() + " - " + strError;

        CTransactionRef wtx;
        if (!currentNode.GetBudgetSystemCollateralTX(wtx, budgetProposalBroadcast.GetHash())) {
            return "Error making collateral transaction for proposal. Please check your wallet balance and make sure your wallet is unlocked.";
        }

        //send the tx to the network
        mapValue_t mapValue;
        auto m_wallet = GetMainWallet();
        ReserveDestination reservekey(m_wallet.get(), OutputType::LEGACY);
        m_wallet->CommitTransaction(wtx, mapValue, {});

        return wtx->GetHash().ToString();
    }

    if (strCommand == "submit") {
        int nBlockMin = 0;
        CBlockIndex* pindexPrev = ::ChainActive().Tip();

        if (request.params.size() != 8)
            throw runtime_error("Correct usage is 'mnbudget submit proposal-name url payment_count block_start crown_address monthly_payment_crown fee_tx'");

        // Check these inputs the same way we check the vote commands:
        // **********************************************************

        std::string strProposalName = request.params[1].get_str();
        if (strProposalName.size() > 20)
            return "Invalid proposal name, limit of 20 characters.";

        std::string strURL = request.params[2].get_str();
        if (strURL.size() > 64)
            return "Invalid url, limit of 64 characters.";

        int nPaymentCount = request.params[3].get_int();
        if (nPaymentCount < 1)
            return "Invalid payment count, must be more than zero.";

        //set block min
        if (pindexPrev != nullptr)
            nBlockMin = pindexPrev->nHeight - GetBudgetPaymentCycleBlocks() * (nPaymentCount + 1);

        int nBlockStart = request.params[4].get_int();
        if (nBlockStart % GetBudgetPaymentCycleBlocks() != 0) {
            const int nextBlock = GetNextSuperblock(pindexPrev->nHeight);
            return strprintf("Invalid block start - must be a budget cycle block. Next valid block: %d", nextBlock);
        }

        int nBlockEnd = nBlockStart + (GetBudgetPaymentCycleBlocks() * nPaymentCount);

        if (nBlockStart < nBlockMin)
            return "Invalid payment count, must be more than current height.";

        if (nBlockEnd < pindexPrev->nHeight)
            return "Invalid ending block, starting block + (payment_cycle*payments) must be more than current height.";

        CTxDestination address = DecodeDestination(request.params[5].get_str());
        if (!IsValidDestination(address))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Crown address");

        // Parse Crown address
        CScript scriptPubKey = GetScriptForDestination(address);
        CAmount nAmount = AmountFromValue(request.params[6]);
        uint256 hash = ParseHashV(request.params[7], "parameter 1");

        //create the proposal incase we're the first to make it
        CBudgetProposalBroadcast budgetProposalBroadcast(strProposalName, strURL, nPaymentCount, scriptPubKey, nAmount, nBlockStart, hash);

        std::string strError = "";
        int nConf = 0;
        if (!IsBudgetCollateralValid(hash, budgetProposalBroadcast.GetHash(), strError, budgetProposalBroadcast.nTime, nConf)) {
            return "Proposal FeeTX is not valid - " + hash.ToString() + " - " + strError;
        }

        if (!masternodeSync.IsBlockchainSynced()) {
            return "Must wait for client to sync with masternode network. Try again in a minute or so.";
        }

        budgetProposalBroadcast.Relay();
        budget.AddProposal(budgetProposalBroadcast);

        return budgetProposalBroadcast.GetHash().ToString();
    }

    if (strCommand == "vote-many") {

        if (request.params.size() != 3)
            throw runtime_error("Correct usage is 'mnbudget vote-many proposal-hash yes|no'");

        uint256 hash = ParseHashV(request.params[1], "parameter 1");
        std::string strVote = request.params[2].get_str();

        if (strVote != "yes" && strVote != "no")
            return "You can only vote 'yes' or 'no'";
        int nVote = VOTE_ABSTAIN;
        if (strVote == "yes")
            nVote = VOTE_YES;
        if (strVote == "no")
            nVote = VOTE_NO;

        int success = 0;
        int failed = 0;

        UniValue resultsObj(UniValue::VOBJ);

        for (auto mne : masternodeConfig.getEntries()) {
            std::string errorMessage;
            std::vector<unsigned char> vchMasterNodeSignature;
            std::string strMasterNodeSignMessage;

            CPubKey pubKeyCollateralAddress;
            CKey keyCollateralAddress;
            CPubKey pubKeyMasternode;
            CKey keyMasternode;

            UniValue statusObj(UniValue::VOBJ);

            if (!legacySigner.SetKey(mne.getPrivKey(), keyMasternode, pubKeyMasternode)) {
                failed++;
                statusObj.pushKV("result", "failed");
                statusObj.pushKV("errorMessage", "Masternode signing error, could not set key correctly: " + errorMessage);
                resultsObj.pushKV(mne.getAlias(), statusObj);
                continue;
            }

            CMasternode* pmn = mnodeman.Find(pubKeyMasternode);
            if (!pmn) {
                failed++;
                statusObj.pushKV("result", "failed");
                statusObj.pushKV("errorMessage", "Can't find masternode by pubkey");
                resultsObj.pushKV(mne.getAlias(), statusObj);
                continue;
            }

            CBudgetVote vote(pmn->vin, hash, nVote);
            if (!vote.Sign(keyMasternode, pubKeyMasternode)) {
                failed++;
                statusObj.pushKV("result", "failed");
                statusObj.pushKV("errorMessage", "Failure to sign.");
                resultsObj.pushKV(mne.getAlias(), statusObj);
                continue;
            }

            std::string strError = "";
            if (budget.SubmitProposalVote(vote, strError)) {
                vote.Relay();
                success++;
                statusObj.pushKV("result", "success");
            } else {
                failed++;
                statusObj.pushKV("result", strError.c_str());
            }

            resultsObj.pushKV(mne.getAlias(), statusObj);
        }

        UniValue returnObj(UniValue::VOBJ);
        returnObj.pushKV("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed));
        returnObj.pushKV("detail", resultsObj);

        return returnObj;
    }

    if (strCommand == "vote") {

        if (request.params.size() != 3)
            throw runtime_error("Correct usage is 'mnbudget vote proposal-hash yes|no'");

        uint256 hash = ParseHashV(request.params[1], "parameter 1");
        std::string strVote = request.params[2].get_str();

        if (strVote != "yes" && strVote != "no")
            return "You can only vote 'yes' or 'no'";
        int nVote = VOTE_ABSTAIN;
        if (strVote == "yes")
            nVote = VOTE_YES;
        if (strVote == "no")
            nVote = VOTE_NO;

        CPubKey pubKeyMasternode;
        CKey keyMasternode;
        std::string errorMessage;

        if (!legacySigner.SetKey(strMasterNodePrivKey, keyMasternode, pubKeyMasternode))
            return "Error upon calling SetKey";

        CMasternode* pmn = mnodeman.Find(activeMasternode.vin);
        if (!pmn) {
            return "Failure to find masternode in list : " + activeMasternode.vin.ToString();
        }

        CBudgetVote vote(activeMasternode.vin, hash, nVote);
        if (!vote.Sign(keyMasternode, pubKeyMasternode)) {
            return "Failure to sign.";
        }

        std::string strError = "";
        if (budget.SubmitProposalVote(vote, strError)) {
            vote.Relay();
            return "Voted successfully";
        } else {
            return "Error voting : " + strError;
        }
    }

    if (strCommand == "projection") {
        UniValue resultsObj(UniValue::VOBJ);
        CAmount nTotalAllotted = 0;

        std::vector<CBudgetProposal*> winningProps = budget.GetBudget();
        for (auto* pbudgetProposal : winningProps) {
            nTotalAllotted += pbudgetProposal->GetAllotted();

            CTxDestination address1;
            ExtractDestination(pbudgetProposal->GetPayee(), address1);
            CTxDestination address2(address1);

            UniValue bObj(UniValue::VOBJ);
            bObj.pushKV("URL", pbudgetProposal->GetURL());
            bObj.pushKV("Hash", pbudgetProposal->GetHash().ToString());
            bObj.pushKV("BlockStart", (int64_t)pbudgetProposal->GetBlockStart());
            bObj.pushKV("BlockEnd", (int64_t)pbudgetProposal->GetBlockEnd());
            bObj.pushKV("TotalPaymentCount", (int64_t)pbudgetProposal->GetTotalPaymentCount());
            bObj.pushKV("RemainingPaymentCount", (int64_t)pbudgetProposal->GetRemainingPaymentCount());
            bObj.pushKV("PaymentAddress", EncodeDestination(address2));
            bObj.pushKV("Ratio", pbudgetProposal->GetRatio());
            bObj.pushKV("Yeas", (int64_t)pbudgetProposal->GetYeas());
            bObj.pushKV("Nays", (int64_t)pbudgetProposal->GetNays());
            bObj.pushKV("Abstains", (int64_t)pbudgetProposal->GetAbstains());
            bObj.pushKV("TotalPayment", ValueFromAmount(pbudgetProposal->GetAmount() * pbudgetProposal->GetTotalPaymentCount()));
            bObj.pushKV("MonthlyPayment", ValueFromAmount(pbudgetProposal->GetAmount()));
            bObj.pushKV("Alloted", ValueFromAmount(pbudgetProposal->GetAllotted()));
            bObj.pushKV("TotalBudgetAlloted", ValueFromAmount(nTotalAllotted));

            std::string strError = "";
            bObj.pushKV("IsValid", pbudgetProposal->IsValid(strError));
            bObj.pushKV("IsValidReason", strError.c_str());
            bObj.pushKV("fValid", pbudgetProposal->fValid);

            resultsObj.pushKV(pbudgetProposal->GetName(), bObj);
        }

        return resultsObj;
    }

    if (strCommand == "show") {
        std::string strShow = "valid";
        if (request.params.size() == 2)
            std::string strProposalName = request.params[1].get_str();

        UniValue resultsObj(UniValue::VOBJ);
        int64_t nTotalAllotted = 0;

        std::vector<CBudgetProposal*> winningProps = budget.GetAllProposals();
        for (auto* pbudgetProposal : winningProps) {
            if (strShow == "valid" && !pbudgetProposal->fValid)
                continue;

            nTotalAllotted += pbudgetProposal->GetAllotted();

            CTxDestination address1;
            ExtractDestination(pbudgetProposal->GetPayee(), address1);
            CTxDestination address2(address1);

            UniValue bObj(UniValue::VOBJ);
            bObj.pushKV("Name", pbudgetProposal->GetName());
            bObj.pushKV("URL", pbudgetProposal->GetURL());
            bObj.pushKV("Hash", pbudgetProposal->GetHash().ToString());
            bObj.pushKV("FeeHash", pbudgetProposal->nFeeTXHash.ToString());
            bObj.pushKV("BlockStart", (int64_t)pbudgetProposal->GetBlockStart());
            bObj.pushKV("BlockEnd", (int64_t)pbudgetProposal->GetBlockEnd());
            bObj.pushKV("TotalPaymentCount", (int64_t)pbudgetProposal->GetTotalPaymentCount());
            bObj.pushKV("RemainingPaymentCount", (int64_t)pbudgetProposal->GetRemainingPaymentCount());
            bObj.pushKV("PaymentAddress", EncodeDestination(address2));
            bObj.pushKV("Ratio", pbudgetProposal->GetRatio());
            bObj.pushKV("Yeas", (int64_t)pbudgetProposal->GetYeas());
            bObj.pushKV("Nays", (int64_t)pbudgetProposal->GetNays());
            bObj.pushKV("Abstains", (int64_t)pbudgetProposal->GetAbstains());
            bObj.pushKV("TotalPayment", ValueFromAmount(pbudgetProposal->GetAmount() * pbudgetProposal->GetTotalPaymentCount()));
            bObj.pushKV("MonthlyPayment", ValueFromAmount(pbudgetProposal->GetAmount()));

            bObj.pushKV("IsEstablished", pbudgetProposal->IsEstablished());

            std::string strError = "";
            bObj.pushKV("IsValid", pbudgetProposal->IsValid(strError));
            bObj.pushKV("IsValidReason", strError.c_str());
            bObj.pushKV("fValid", pbudgetProposal->fValid);

            resultsObj.pushKV(pbudgetProposal->GetName(), bObj);
        }

        return resultsObj;
    }

    if (strCommand == "getinfo") {
        if (request.params.size() != 2)
            throw runtime_error("Correct usage is 'mnbudget getinfo profilename'");

        std::string strProposalName = request.params[1].get_str();

        CBudgetProposal* pbudgetProposal = budget.FindProposal(strProposalName);

        if (pbudgetProposal == nullptr)
            return "Unknown proposal name";

        CTxDestination address1;
        ExtractDestination(pbudgetProposal->GetPayee(), address1);
        CTxDestination address2(address1);

        UniValue obj(UniValue::VOBJ);
        obj.pushKV("Name", pbudgetProposal->GetName());
        obj.pushKV("Hash", pbudgetProposal->GetHash().ToString());
        obj.pushKV("FeeHash", pbudgetProposal->nFeeTXHash.ToString());
        obj.pushKV("URL", pbudgetProposal->GetURL());
        obj.pushKV("BlockStart", (int64_t)pbudgetProposal->GetBlockStart());
        obj.pushKV("BlockEnd", (int64_t)pbudgetProposal->GetBlockEnd());
        obj.pushKV("TotalPaymentCount", (int64_t)pbudgetProposal->GetTotalPaymentCount());
        obj.pushKV("RemainingPaymentCount", (int64_t)pbudgetProposal->GetRemainingPaymentCount());
        obj.pushKV("PaymentAddress", EncodeDestination(address2));
        obj.pushKV("Ratio", pbudgetProposal->GetRatio());
        obj.pushKV("Yeas", (int64_t)pbudgetProposal->GetYeas());
        obj.pushKV("Nays", (int64_t)pbudgetProposal->GetNays());
        obj.pushKV("Abstains", (int64_t)pbudgetProposal->GetAbstains());
        obj.pushKV("TotalPayment", ValueFromAmount(pbudgetProposal->GetAmount() * pbudgetProposal->GetTotalPaymentCount()));
        obj.pushKV("MonthlyPayment", ValueFromAmount(pbudgetProposal->GetAmount()));

        obj.pushKV("IsEstablished", pbudgetProposal->IsEstablished());

        std::string strError = "";
        obj.pushKV("IsValid", pbudgetProposal->IsValid(strError));
        obj.pushKV("fValid", pbudgetProposal->fValid);

        return obj;
    }

    if (strCommand == "getvotes") {
        if (request.params.size() != 2)
            throw runtime_error("Correct usage is 'mnbudget getvotes profilename'");

        std::string strProposalName = request.params[1].get_str();

        UniValue obj(UniValue::VOBJ);

        CBudgetProposal* pbudgetProposal = budget.FindProposal(strProposalName);

        if (pbudgetProposal == nullptr)
            return "Unknown proposal name";

        std::map<uint256, CBudgetVote>::iterator it = pbudgetProposal->mapVotes.begin();
        while (it != pbudgetProposal->mapVotes.end()) {

            UniValue bObj(UniValue::VOBJ);
            bObj.pushKV("nHash", (*it).first.ToString().c_str());
            bObj.pushKV("Vote", (*it).second.GetVoteString());
            bObj.pushKV("nTime", (int64_t)(*it).second.nTime);
            bObj.pushKV("fValid", (*it).second.fValid);

            obj.pushKV((*it).second.vin.prevout.ToStringShort(), bObj);

            it++;
        }

        return obj;
    }

    if (strCommand == "check") {
        budget.CheckAndRemove();

        return "Success";
    }

    return NullUniValue;
}

UniValue mnbudgetvoteraw(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 6)
        throw runtime_error(
            "mnbudgetvoteraw <masternode-tx-hash> <masternode-tx-index> <proposal-hash> <yes|no> <time> <vote-sig>\n"
            "Compile and relay a proposal vote with provided external signature instead of signing vote internally\n");

    uint256 hashMnTx = ParseHashV(request.params[0], "mn tx hash");
    int nMnTxIndex = request.params[1].get_int();
    CTxIn vin = CTxIn(hashMnTx, nMnTxIndex);

    uint256 hashProposal = ParseHashV(request.params[2], "Proposal hash");
    std::string strVote = request.params[3].get_str();

    if (strVote != "yes" && strVote != "no")
        return "You can only vote 'yes' or 'no'";
    int nVote = VOTE_ABSTAIN;
    if (strVote == "yes")
        nVote = VOTE_YES;
    if (strVote == "no")
        nVote = VOTE_NO;

    int64_t nTime = request.params[4].get_int64();
    std::string strSig = request.params[5].get_str();
    bool fInvalid = false;
    vector<unsigned char> vchSig = DecodeBase64(strSig.c_str(), &fInvalid);

    if (fInvalid)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");

    CMasternode* pmn = mnodeman.Find(vin);
    if (!pmn) {
        return "Failure to find masternode in list : " + vin.ToString();
    }

    CBudgetVote vote(vin, hashProposal, nVote);
    vote.nTime = nTime;
    vote.vchSig = vchSig;

    if (!vote.SignatureValid(true)) {
        return "Failure to verify signature.";
    }

    std::string strError = "";
    if (budget.SubmitProposalVote(vote, strError)) {
        vote.Relay();
        return "Voted successfully";
    } else {
        return "Error voting : " + strError;
    }
}

UniValue mnfinalbudget(const JSONRPCRequest& request)
{
    string strCommand;
    if (request.params.size() >= 1)
        strCommand = request.params[0].get_str();

    if (request.fHelp || (strCommand != "suggest" && strCommand != "vote-many" && strCommand != "vote" && strCommand != "show" && strCommand != "getvotes"))
        throw runtime_error(
            "mnfinalbudget \"command\"... ( \"passphrase\" )\n"
            "Vote or show current budgets\n"
            "\nAvailable commands:\n"
            "  vote-many   - Vote on a finalized budget\n"
            "  vote        - Vote on a finalized budget\n"
            "  show        - Show existing finalized budgets\n"
            "  getvotes     - Get vote information for each finalized budget\n");

    if (strCommand == "vote-many") {

        if (request.params.size() != 2)
            throw runtime_error("Correct usage is 'mnfinalbudget vote-many BUDGET_HASH'");

        std::string strHash = request.params[1].get_str();
        uint256 hash(uint256S(strHash));

        int success = 0;
        int failed = 0;

        UniValue resultsObj(UniValue::VOBJ);

        for (auto mne : masternodeConfig.getEntries()) {
            std::string errorMessage;
            std::vector<unsigned char> vchMasterNodeSignature;
            std::string strMasterNodeSignMessage;

            CPubKey pubKeyCollateralAddress;
            CKey keyCollateralAddress;
            CPubKey pubKeyMasternode;
            CKey keyMasternode;

            UniValue statusObj(UniValue::VOBJ);

            if (!legacySigner.SetKey(mne.getPrivKey(), keyMasternode, pubKeyMasternode)) {
                failed++;
                statusObj.pushKV("result", "failed");
                statusObj.pushKV("errorMessage", "Masternode signing error, could not set key correctly: " + errorMessage);
                resultsObj.pushKV(mne.getAlias(), statusObj);
                continue;
            }

            CMasternode* pmn = mnodeman.Find(pubKeyMasternode);
            if (!pmn) {
                failed++;
                statusObj.pushKV("result", "failed");
                statusObj.pushKV("errorMessage", "Can't find masternode by pubkey");
                resultsObj.pushKV(mne.getAlias(), statusObj);
                continue;
            }

            BudgetDraftVote vote(pmn->vin, hash);
            if (!vote.Sign(keyMasternode, pubKeyMasternode)) {
                failed++;
                statusObj.pushKV("result", "failed");
                statusObj.pushKV("errorMessage", "Failure to sign.");
                resultsObj.pushKV(mne.getAlias(), statusObj);
                continue;
            }

            std::string strError = "";
            if (budget.UpdateBudgetDraft(vote, nullptr, strError)) {
                vote.Relay();
                success++;
                statusObj.pushKV("result", "success");
            } else {
                failed++;
                statusObj.pushKV("result", strError.c_str());
            }

            resultsObj.pushKV(mne.getAlias(), statusObj);
        }

        UniValue returnObj(UniValue::VOBJ);
        returnObj.pushKV("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed));
        returnObj.pushKV("detail", resultsObj);

        return returnObj;
    }

    if (strCommand == "vote") {
        if (request.params.size() != 2)
            throw runtime_error("Correct usage is 'mnfinalbudget vote BUDGET_HASH'");

        std::string strHash = request.params[1].get_str();
        uint256 hash(uint256S(strHash));

        CPubKey pubKeyMasternode;
        CKey keyMasternode;
        std::string errorMessage;

        if (!legacySigner.SetKey(strMasterNodePrivKey, keyMasternode, pubKeyMasternode))
            return "Error upon calling SetKey";

        CMasternode* pmn = mnodeman.Find(activeMasternode.vin);
        if (!pmn) {
            return "Failure to find masternode in list : " + activeMasternode.vin.ToString();
        }

        BudgetDraftVote vote(activeMasternode.vin, hash);
        if (!vote.Sign(keyMasternode, pubKeyMasternode)) {
            return "Failure to sign.";
        }

        std::string strError = "";
        if (budget.UpdateBudgetDraft(vote, nullptr, strError)) {
            vote.Relay();
            return "success";
        } else {
            return "Error voting : " + strError;
        }
    }

    if (strCommand == "show") {
        UniValue resultsObj(UniValue::VOBJ);

        std::vector<BudgetDraft*> drafts = budget.GetBudgetDrafts();
        for (auto* budgetDraft : drafts) {
            UniValue bObj(UniValue::VOBJ);
            bObj.pushKV("IsSubmittedManually", budgetDraft->IsSubmittedManually());
            bObj.pushKV("Hash", budgetDraft->GetHash().ToString());
            bObj.pushKV("BlockStart", (int64_t)budgetDraft->GetBlockStart());
            bObj.pushKV("BlockEnd", (int64_t)budgetDraft->GetBlockStart()); // Budgets are paid in a single block !
            bObj.pushKV("Proposals", budgetDraft->GetProposals());
            bObj.pushKV("VoteCount", (int64_t)budgetDraft->GetVoteCount());
            bObj.pushKV("Status", budgetDraft->GetStatus());
            std::string strError = "";
            bObj.pushKV("IsValid", budgetDraft->IsValid(strError));
            bObj.pushKV("IsValidReason", strError.c_str());
            if (budgetDraft->IsSubmittedManually())
                bObj.pushKV("FeeTX", budgetDraft->GetFeeTxHash().ToString());

            resultsObj.pushKV(budgetDraft->GetHash().ToString(), bObj);
        }

        return resultsObj;
    }

    if (strCommand == "getvotes") {
        if (request.params.size() != 2)
            throw runtime_error("Correct usage is 'mnbudget getvotes budget-hash'");

        std::string strHash = request.params[1].get_str();
        uint256 hash(uint256S(strHash));

        UniValue obj(UniValue::VOBJ);

        BudgetDraft* pbudgetDraft = budget.FindBudgetDraft(hash);

        if (pbudgetDraft == nullptr)
            return "Unknown budget hash";

        const std::map<uint256, BudgetDraftVote>& fbVotes = pbudgetDraft->GetVotes();
        for (std::map<uint256, BudgetDraftVote>::const_iterator i = fbVotes.begin(); i != fbVotes.end(); ++i) {
            UniValue bObj(UniValue::VOBJ);
            bObj.pushKV("nHash", i->first.ToString().c_str());
            bObj.pushKV("nTime", static_cast<int64_t>(i->second.nTime));
            bObj.pushKV("fValid", i->second.fValid);

            obj.pushKV(i->second.vin.prevout.ToStringShort(), bObj);
        }

        const std::map<uint256, BudgetDraftVote>& oldVotes = pbudgetDraft->GetObsoleteVotes();
        for (std::map<uint256, BudgetDraftVote>::const_iterator i = oldVotes.begin(); i != oldVotes.end(); ++i) {
            UniValue bObj(UniValue::VOBJ);
            bObj.pushKV("nHash", i->first.ToString().c_str());
            bObj.pushKV("nTime", static_cast<int64_t>(i->second.nTime));
            bObj.pushKV("fValid", i->second.fValid);
            bObj.pushKV("obsolete", true);

            obj.pushKV(i->second.vin.prevout.ToStringShort(), bObj);
        }

        return obj;
    }

    return NullUniValue;
}

void RegisterBudgetCommands(CRPCTable &t)
{
    static const CRPCCommand commands[] =
    { //  category              name                         actor (function)            arguments
      //  --------------------- ------------------------     ------------------          ---------
          {"budget",        "mnbudget",                      &mnbudget,                  {}},
          {"budget",        "mnbudgetvoteraw",               &mnbudgetvoteraw,           {}},
          {"budget",        "mnfinalbudget",                 &mnfinalbudget,             {}},
    };

    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}

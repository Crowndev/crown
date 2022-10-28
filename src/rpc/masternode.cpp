// Copyright (c) 2014-2021 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <init.h>
#include <key_io.h>
#include <net.h>
#include <net_processing.h>
#include <node/context.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <util/moneystr.h>
#include <util/system.h>
#include <validation.h>

#include <masternode/activemasternode.h>
#include <masternode/masternode-payments.h>
#include <masternode/masternode-sync.h>
#include <masternode/masternodeconfig.h>
#include <masternode/masternodeman.h>
#include <crown/nodesync.h>

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

UniValue listmasternodes(const JSONRPCRequest& request)
{
    std::string strFilter = "";

    if (request.params.size() == 1)
        strFilter = request.params[0].get_str();

    if (request.fHelp || (request.params.size() > 1))
        throw std::runtime_error(
            "listmasternodes ( \"filter\" )\n"
            "\nGet a ranked list of masternodes\n"

            "\nArguments:\n"
            "1. \"filter\"    (string, optional) Filter search text. Partial match by txhash, status, or addr.\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"rank\": n,           (numeric) Masternode Rank (or 0 if not enabled)\n"
            "    \"txhash\": \"hash\",    (string) Collateral transaction hash\n"
            "    \"outidx\": n,         (numeric) Collateral transaction output index\n"
            "    \"pubkey\": \"key\",   (string) Masternode public key used for message broadcasting\n"
            "    \"status\": s,         (string) Status (ENABLED/EXPIRED/REMOVE/etc)\n"
            "    \"addr\": \"addr\",      (string) Masternode CRW address\n"
            "    \"version\": v,        (numeric) Masternode protocol version\n"
            "    \"ipaddr\": a,         (string) Masternode network address\n"
            "    \"lastseen\": ttt,     (numeric) The time in seconds since epoch (Jan 1 1970 GMT) of the last seen\n"
            "    \"activetime\": ttt,   (numeric) The time in seconds since epoch (Jan 1 1970 GMT) masternode has been active\n"
            "    \"lastpaid\": ttt,     (numeric) The time in seconds since epoch (Jan 1 1970 GMT) masternode was last paid\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n"
            + HelpExampleCli("listmasternodes", "") + HelpExampleRpc("listmasternodes", ""));

    if (!masternodeSync.IsSynced()) {
        throw std::runtime_error("Masternode sync has not yet completed.\n");
    }

    UniValue ret(UniValue::VARR);
    int nHeight;
    {
        LOCK(cs_main);
        CBlockIndex* pindex = ::ChainActive().Tip();
        if (!pindex)
            return 0;
        nHeight = pindex->nHeight;
    }

    std::vector<std::pair<int, CMasternode>> vMasternodeRanks = mnodeman.GetMasternodeRanks(nHeight);
    for (const auto& s : vMasternodeRanks) {
        UniValue obj(UniValue::VOBJ);
        std::string strVin = s.second.vin.prevout.ToStringShort();
        std::string strTxHash = s.second.vin.prevout.hash.ToString();
        uint32_t oIdx = s.second.vin.prevout.n;
        CMasternode* mn = mnodeman.Find(s.second.vin);

        if (mn) {
            if (strFilter != "" && strTxHash.find(strFilter) == std::string::npos && mn->Status().find(strFilter) == std::string::npos && EncodeDestination(PKHash(mn->pubkey.GetID())).find(strFilter) == std::string::npos)
                continue;

            std::string strStatus = mn->Status();
            std::string strHost;
            int port;
            SplitHostPort(mn->addr.ToString(), port, strHost);

            obj.pushKV("rank", (strStatus == "ENABLED" ? s.first : 0));
            obj.pushKV("txhash", strTxHash);
            obj.pushKV("outidx", (uint64_t)oIdx);
            obj.pushKV("pubkey", HexStr(mn->pubkey2));
            obj.pushKV("status", strStatus);
            obj.pushKV("addr", EncodeDestination(PKHash(mn->pubkey.GetID())));
            obj.pushKV("version", mn->protocolVersion);
            obj.pushKV("ipaddr", mn->addr.ToString());
            obj.pushKV("lastseen", (int64_t)mn->lastPing.sigTime);
            obj.pushKV("activetime", (int64_t)(mn->lastPing.sigTime - mn->sigTime));
            obj.pushKV("lastpaid", (int64_t)mn->GetLastPaid());

            ret.push_back(obj);
        }
    }

    return ret;
}

UniValue getmasternodecount(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() > 0))
        throw std::runtime_error(
            "getmasternodecount\n"
            "\nGet masternode count values\n"

            "\nResult:\n"
            "{\n"
            "  \"total\": n,        (numeric) Total masternodes\n"
            "  \"stable\": n,       (numeric) Stable count\n"
            "  \"enabled\": n,      (numeric) Enabled masternodes\n"
            "  \"inqueue\": n       (numeric) Masternodes in queue\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("getmasternodecount", "") + HelpExampleRpc("getmasternodecount", ""));

    if (!masternodeSync.IsSynced()) {
        throw std::runtime_error("Masternode sync has not yet completed.\n");
    }

    UniValue obj(UniValue::VOBJ);
    int nCount = 0;

    if (::ChainActive().Tip())
        mnodeman.GetNextMasternodeInQueueForPayment(::ChainActive().Tip()->nHeight, true, nCount);

    obj.pushKV("total", mnodeman.size());
    obj.pushKV("enabled", mnodeman.CountEnabled());
    obj.pushKV("inqueue", nCount);

    return obj;
}

UniValue masternodecurrent(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 0))
        throw std::runtime_error(
            "masternodecurrent\n"
            "\nGet current masternode winner (scheduled to be paid next).\n"

            "\nResult:\n"
            "{\n"
            "  \"protocol\": xxxx,        (numeric) Protocol version\n"
            "  \"txhash\": \"xxxx\",      (string) Collateral transaction hash\n"
            "  \"pubkey\": \"xxxx\",      (string) MN Public key\n"
            "  \"lastseen\": xxx,         (numeric) Time since epoch of last seen\n"
            "  \"activeseconds\": xxx,    (numeric) Seconds MN has been active\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("masternodecurrent", "") + HelpExampleRpc("masternodecurrent", ""));

    if (!masternodeSync.IsSynced()) {
        throw std::runtime_error("Masternode sync has not yet completed.\n");
    }

    const int nHeight = WITH_LOCK(cs_main, return ::ChainActive().Height() + 1);
    int nCount = 0;
    CMasternode* winner = mnodeman.GetNextMasternodeInQueueForPayment(nHeight, true, nCount);
    if (winner) {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("protocol", (int64_t)winner->protocolVersion);
        obj.pushKV("txhash", winner->vin.prevout.hash.ToString());
        obj.pushKV("pubkey", EncodeDestination(PKHash(winner->pubkey.GetID())));
        obj.pushKV("lastseen", (winner->lastPing == CMasternodePing() ? winner->sigTime : (int64_t)winner->lastPing.sigTime));
        obj.pushKV("activeseconds", (winner->lastPing == CMasternodePing() ? 0 : (int64_t)(winner->lastPing.sigTime - winner->sigTime)));
        return obj;
    }

    throw std::runtime_error("unknown");
}

bool StartMasternodeEntry(UniValue& statusObjRet, CMasternodeBroadcast& mnbRet, bool& fSuccessRet, const CNodeEntry& mne, std::string& errorMessage, std::string strCommand = "")
{
    CTxIn vin = CTxIn(uint256S(mne.getTxHash()), (uint32_t)atoi(mne.getOutputIndex()));
    CMasternode* pmn = mnodeman.Find(vin);
    if (pmn) {
        if (strCommand == "missing")
            return false;
        if (strCommand == "disabled" && pmn->IsEnabled())
            return false;
    }

    fSuccessRet = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage, mnbRet);

    statusObjRet.pushKV("alias", mne.getAlias());
    statusObjRet.pushKV("result", fSuccessRet ? "success" : "failed");
    statusObjRet.pushKV("error", fSuccessRet ? "" : errorMessage);

    return true;
}

void RelayMNB(CMasternodeBroadcast& mnb, const bool fSuccess, int& successful, int& failed)
{
    if (fSuccess) {
        successful++;
        mnodeman.UpdateMasternodeList(mnb);
        mnb.Relay();
    } else {
        failed++;
    }
}

void RelayMNB(CMasternodeBroadcast& mnb, const bool fSucces)
{
    int successful = 0, failed = 0;
    return RelayMNB(mnb, fSucces, successful, failed);
}

void SerializeMNB(UniValue& statusObjRet, const CMasternodeBroadcast& mnb, const bool fSuccess, int& successful, int& failed)
{
    if (fSuccess) {
        successful++;
        CDataStream ssMnb(SER_NETWORK, PROTOCOL_VERSION);
        ssMnb << mnb;
        statusObjRet.pushKV("hex", HexStr(ssMnb));
    } else {
        failed++;
    }
}

void SerializeMNB(UniValue& statusObjRet, const CMasternodeBroadcast& mnb, const bool fSuccess)
{
    int successful = 0, failed = 0;
    return SerializeMNB(statusObjRet, mnb, fSuccess, successful, failed);
}

UniValue startmasternode(const JSONRPCRequest& request)
{
    std::string strCommand;
    if (request.params.size() >= 1) {
        strCommand = request.params[0].get_str();

        // Backwards compatibility with legacy 'masternode' super-command forwarder
        if (strCommand == "start")
            strCommand = "local";
        if (strCommand == "start-alias")
            strCommand = "alias";
        if (strCommand == "start-all")
            strCommand = "all";
        if (strCommand == "start-many")
            strCommand = "many";
        if (strCommand == "start-missing")
            strCommand = "missing";
        if (strCommand == "start-disabled")
            strCommand = "disabled";
    }

    if (!masternodeSync.IsSynced()) {
        throw std::runtime_error("Masternode sync has not yet completed.\n" + currentSyncStatus() +"\n");
    }

    if (request.fHelp || request.params.size() < 2 || request.params.size() > 3 || (request.params.size() == 2 && (strCommand != "local" && strCommand != "all" && strCommand != "many" && strCommand != "missing" && strCommand != "disabled")) || (request.params.size() == 3 && strCommand != "alias"))
        throw std::runtime_error(
            "startmasternode \"local|all|many|missing|disabled|alias\" lockwallet ( \"alias\" )\n"
            "\nAttempts to start one or more masternode(s)\n"

            "\nArguments:\n"
            "1. set         (string, required) Specify which set of masternode(s) to start.\n"
            "2. lockwallet  (boolean, required) Lock wallet after completion.\n"
            "3. alias       (string) Masternode alias. Required if using 'alias' as the set.\n"

            "\nResult: (for 'local' set):\n"
            "\"status\"     (string) Masternode status message\n"

            "\nResult: (for other sets):\n"
            "{\n"
            "  \"overall\": \"xxxx\",     (string) Overall status message\n"
            "  \"detail\": [\n"
            "    {\n"
            "      \"node\": \"xxxx\",    (string) Node name or alias\n"
            "      \"result\": \"xxxx\",  (string) 'success' or 'failed'\n"
            "      \"error\": \"xxxx\"    (string) Error message, if failed\n"
            "    }\n"
            "    ,...\n"
            "  ]\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("startmasternode", "\"alias\" \"0\" \"my_mn\"") + HelpExampleRpc("startmasternode", "\"alias\" \"0\" \"my_mn\""));

    bool fLock = (request.params[1].get_str() == "true" ? true : false);

    if (strCommand == "local") {
        if (!fMasterNode)
            throw std::runtime_error("you must set masternode=1 in the configuration\n");

        if (activeMasternode.status != ACTIVE_MASTERNODE_STARTED) {
            activeMasternode.status = ACTIVE_MASTERNODE_INITIAL;
            activeMasternode.ManageStatus();
            if (fLock)
                GetMainWallet()->Lock();
        }

        return activeMasternode.GetStatus();
    }

    if (strCommand == "all" || strCommand == "many" || strCommand == "missing" || strCommand == "disabled") {
        if ((strCommand == "missing" || strCommand == "disabled") && (masternodeSync.RequestedMasternodeAssets <= MASTERNODE_SYNC_LIST || masternodeSync.RequestedMasternodeAssets == MASTERNODE_SYNC_FAILED)) {
            throw std::runtime_error("You can't use this command until masternode list is synced\n");
        }

        std::vector<CNodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        int successful = 0;
        int failed = 0;

        UniValue resultsObj(UniValue::VARR);

        for (CNodeEntry mne : masternodeConfig.getEntries()) {
            UniValue statusObj(UniValue::VOBJ);
            CMasternodeBroadcast mnb;
            std::string errorMessage;
            bool fSuccess = false;
            if (!StartMasternodeEntry(statusObj, mnb, fSuccess, mne, errorMessage, strCommand))
                continue;
            resultsObj.push_back(statusObj);
            RelayMNB(mnb, fSuccess, successful, failed);
        }
        if (fLock)
            GetMainWallet()->Lock();

        UniValue returnObj(UniValue::VOBJ);
        returnObj.pushKV("overall", strprintf("Successfully started %d masternodes, failed to start %d, total %d", successful, failed, successful + failed));
        returnObj.pushKV("detail", resultsObj);

        return returnObj;
    }

    if (strCommand == "alias") {
        std::string alias = request.params[2].get_str();

        bool found = false;

        UniValue resultsObj(UniValue::VARR);
        UniValue statusObj(UniValue::VOBJ);

        for (CNodeEntry mne : masternodeConfig.getEntries()) {
            if (mne.getAlias() == alias) {
                CMasternodeBroadcast mnb;
                found = true;
                std::string errorMessage;
                bool fSuccess = false;
                if (!StartMasternodeEntry(statusObj, mnb, fSuccess, mne, errorMessage, strCommand))
                    continue;
                RelayMNB(mnb, fSuccess);
                break;
            }
        }

        if (fLock)
            GetMainWallet()->Lock();

        if (!found) {
            statusObj.pushKV("success", false);
            statusObj.pushKV("error_message", "Could not find alias in config. Verify with listmasternodeconf.");
        }

        return statusObj;
    }
    return NullUniValue;
}

UniValue createmasternodekey(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 0))
        throw std::runtime_error(
            "createmasternodekey\n"
            "\nCreate a new masternode private key\n"

            "\nResult:\n"
            "\"key\"    (string) Masternode private key\n"

            "\nExamples:\n"
            + HelpExampleCli("createmasternodekey", "") + HelpExampleRpc("createmasternodekey", ""));

    CKey secret;
    secret.MakeNewKey(false);

    return EncodeSecret(secret);
}

UniValue getmasternodeoutputs(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 0))
        throw std::runtime_error(
            "getmasternodeoutputs\n"
            "\nPrint all masternode transaction outputs\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"txhash\": \"xxxx\",    (string) output transaction hash\n"
            "    \"outputidx\": n       (numeric) output index number\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n"
            + HelpExampleCli("getmasternodeoutputs", "") + HelpExampleRpc("getmasternodeoutputs", ""));

    // Find possible candidates
    std::vector<COutput> possibleCoins = activeMasternode.SelectCoinsMasternode();

    UniValue ret(UniValue::VARR);
    for (COutput& out : possibleCoins) {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("txhash", out.tx->GetHash().ToString());
        obj.pushKV("outputidx", out.i);
        ret.push_back(obj);
    }

    return ret;
}

UniValue listmasternodeconf(const JSONRPCRequest& request)
{
    std::string strFilter = "";

    if (request.params.size() == 1)
        strFilter = request.params[0].get_str();

    if (request.fHelp || (request.params.size() > 1))
        throw std::runtime_error(
            "listmasternodeconf ( \"filter\" )\n"
            "\nPrint masternode.conf in JSON format\n"

            "\nArguments:\n"
            "1. \"filter\"    (string, optional) Filter search text. Partial match on alias, address, txHash, or status.\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"alias\": \"xxxx\",        (string) masternode alias\n"
            "    \"address\": \"xxxx\",      (string) masternode IP address\n"
            "    \"privateKey\": \"xxxx\",   (string) masternode private key\n"
            "    \"txHash\": \"xxxx\",       (string) transaction hash\n"
            "    \"outputIndex\": n,       (numeric) transaction output index\n"
            "    \"status\": \"xxxx\"        (string) masternode status\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n"
            + HelpExampleCli("listmasternodeconf", "") + HelpExampleRpc("listmasternodeconf", ""));

    std::vector<CNodeEntry> mnEntries;
    mnEntries = masternodeConfig.getEntries();

    UniValue ret(UniValue::VARR);

    for (CNodeEntry mne : masternodeConfig.getEntries()) {
        CTxIn vin = CTxIn(uint256S(mne.getTxHash()), (uint32_t)atoi(mne.getOutputIndex()));
        CMasternode* pmn = mnodeman.Find(vin);

        std::string strStatus = pmn ? pmn->Status() : "MISSING";

        if (strFilter != "" && mne.getAlias().find(strFilter) == std::string::npos && mne.getIp().find(strFilter) == std::string::npos && mne.getTxHash().find(strFilter) == std::string::npos && strStatus.find(strFilter) == std::string::npos)
            continue;

        UniValue mnObj(UniValue::VOBJ);
        mnObj.pushKV("alias", mne.getAlias());
        mnObj.pushKV("address", mne.getIp());
        mnObj.pushKV("privateKey", mne.getPrivKey());
        mnObj.pushKV("txHash", mne.getTxHash());
        mnObj.pushKV("outputIndex", mne.getOutputIndex());
        mnObj.pushKV("status", strStatus);
        ret.push_back(mnObj);
    }

    return ret;
}

UniValue getmasternodestatus(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 0))
        throw std::runtime_error(
            "getmasternodestatus\n"
            "\nPrint masternode status\n"

            "\nResult:\n"
            "{\n"
            "  \"txhash\": \"xxxx\",      (string) Collateral transaction hash\n"
            "  \"outputidx\": n,          (numeric) Collateral transaction output index number\n"
            "  \"netaddr\": \"xxxx\",     (string) Masternode network address\n"
            "  \"addr\": \"xxxx\",        (string) CRW address for masternode payments\n"
            "  \"status\": \"xxxx\",      (string) Masternode status\n"
            "  \"message\": \"xxxx\"      (string) Masternode status message\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("getmasternodestatus", "") + HelpExampleRpc("getmasternodestatus", ""));

    if (!fMasterNode)
        throw JSONRPCError(RPC_MISC_ERROR, ("This is not a masternode."));

    if (activeMasternode.vin == CTxIn())
        throw JSONRPCError(RPC_MISC_ERROR, ("Active Masternode not initialized."));

    CMasternode* pmn = mnodeman.Find(activeMasternode.vin);

    if (pmn) {
        UniValue mnObj(UniValue::VOBJ);
        mnObj.pushKV("txid", activeMasternode.vin.prevout.hash.ToString());
        mnObj.pushKV("outputidx", (uint64_t)activeMasternode.vin.prevout.n);
        mnObj.pushKV("netaddr", activeMasternode.service.ToString());
        mnObj.pushKV("addr", EncodeDestination(PKHash(pmn->pubkey.GetID())));
        mnObj.pushKV("status", activeMasternode.GetStatus());
        return mnObj;
    }
    throw std::runtime_error("Masternode not found in the list of available masternodes. Current status: "
        + activeMasternode.GetStatus());
}

UniValue getmasternodewinners(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 3)
        throw std::runtime_error(
            "getmasternodewinners ( blocks \"filter\" )\n"
            "\nPrint the masternode winners for the last n blocks\n"

            "\nArguments:\n"
            "1. blocks      (numeric, optional) Number of previous blocks to show (default: 10)\n"
            "2. filter      (string, optional) Search filter matching MN address\n"

            "\nResult (single winner):\n"
            "[\n"
            "  {\n"
            "    \"nHeight\": n,           (numeric) block height\n"
            "    \"winner\": {\n"
            "      \"address\": \"xxxx\",    (string) CRW MN Address\n"
            "      \"nVotes\": n,          (numeric) Number of votes for winner\n"
            "    }\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nResult (multiple winners):\n"
            "[\n"
            "  {\n"
            "    \"nHeight\": n,           (numeric) block height\n"
            "    \"winner\": [\n"
            "      {\n"
            "        \"address\": \"xxxx\",  (string) CRW MN Address\n"
            "        \"nVotes\": n,        (numeric) Number of votes for winner\n"
            "      }\n"
            "      ,...\n"
            "    ]\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n"
            + HelpExampleCli("getmasternodewinners", "") + HelpExampleRpc("getmasternodewinners", ""));

    if (!masternodeSync.IsSynced()) {
        throw std::runtime_error("Masternode sync has not yet completed.\n");
    }

    int nHeight;
    {
        LOCK(cs_main);
        CBlockIndex* pindex = ::ChainActive().Tip();
        if (!pindex)
            return 0;
        nHeight = pindex->nHeight;
    }

    int nLast = 10;
    std::string strFilter = "";

    if (request.params.size() >= 1)
        nLast = atoi(request.params[0].get_str());

    if (request.params.size() == 2)
        strFilter = request.params[1].get_str();

    UniValue ret(UniValue::VARR);

    for (int i = nHeight - nLast; i < nHeight + 20; i++) {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("nHeight", i);

        std::string strPayment = GetRequiredPaymentsString(i);
        if (strFilter != "" && strPayment.find(strFilter) == std::string::npos)
            continue;

        if (strPayment.find(',') != std::string::npos) {
            UniValue winner(UniValue::VARR);
            boost::char_separator<char> sep(",");
            boost::tokenizer<boost::char_separator<char>> tokens(strPayment, sep);
            for (const std::string& t : tokens) {
                UniValue addr(UniValue::VOBJ);
                unsigned int pos = t.find(":");
                std::string strAddress = t.substr(0, pos);
                uint64_t nVotes = atoi(t.substr(pos + 1));
                addr.pushKV("address", strAddress);
                addr.pushKV("nVotes", nVotes);
                winner.push_back(addr);
            }
            obj.pushKV("winner", winner);
        } else if (strPayment.find("Unknown") == std::string::npos) {
            UniValue winner(UniValue::VOBJ);
            unsigned int pos = strPayment.find(":");
            std::string strAddress = strPayment.substr(0, pos);
            uint64_t nVotes = atoi(strPayment.substr(pos + 1));
            winner.pushKV("address", strAddress);
            winner.pushKV("nVotes", nVotes);
            obj.pushKV("winner", winner);
        } else {
            UniValue winner(UniValue::VOBJ);
            winner.pushKV("address", strPayment);
            winner.pushKV("nVotes", 0);
            obj.pushKV("winner", winner);
        }

        ret.push_back(obj);
    }

    return ret;
}

UniValue getmasternodescores(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "getmasternodescores ( blocks )\n"
            "\nPrint list of winning masternode by score\n"

            "\nArguments:\n"
            "1. blocks      (numeric, optional) Show the last n blocks (default 10)\n"

            "\nResult:\n"
            "{\n"
            "  xxxx: \"xxxx\"   (numeric : string) Block height : Masternode hash\n"
            "  ,...\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("getmasternodescores", "") + HelpExampleRpc("getmasternodescores", ""));

    if (!masternodeSync.IsSynced()) {
        throw std::runtime_error("Masternode sync has not yet completed.\n");
    }

    int nLast = 10;

    if (request.params.size() == 1) {
        try {
            nLast = std::stoi(request.params[0].get_str());
        } catch (const std::invalid_argument&) {
            throw std::runtime_error("Exception on param 2");
        }
    }
    UniValue obj(UniValue::VOBJ);

    std::vector<CMasternode> vMasternodes = mnodeman.GetFullMasternodeVector();
    for (int nHeight = ::ChainActive().Tip()->nHeight - nLast; nHeight < ::ChainActive().Tip()->nHeight + 20; nHeight++) {
        uint256 nHigh;
        CMasternode* pBestMasternode = NULL;
        for (CMasternode& mn : vMasternodes) {
            uint256 n = ArithToUint256(mn.CalculateScore(nHeight));
            if (UintToArith256(n) > UintToArith256(nHigh)) {
                nHigh = n;
                pBestMasternode = &mn;
            }
        }
        if (pBestMasternode)
            obj.pushKV(strprintf("%d", nHeight), pBestMasternode->vin.prevout.hash.ToString().c_str());
    }

    return obj;
}

bool DecodeHexMnb(CMasternodeBroadcast& mnb, std::string strHexMnb)
{

    if (!IsHex(strHexMnb))
        return false;

    std::vector<unsigned char> mnbData(ParseHex(strHexMnb));
    CDataStream ssData(mnbData, SER_NETWORK, PROTOCOL_VERSION);
    try {
        ssData >> mnb;
    } catch (const std::exception&) {
        return false;
    }

    return true;
}
UniValue createmasternodebroadcast(const JSONRPCRequest& request)
{
    std::string strCommand;
    if (request.params.size() >= 1)
        strCommand = request.params[0].get_str();
    if (request.fHelp || (strCommand != "alias" && strCommand != "all") || (strCommand == "alias" && request.params.size() < 2))
        throw std::runtime_error(
            "createmasternodebroadcast \"command\" ( \"alias\")\n"
            "\nCreates a masternode broadcast message for one or all masternodes configured in masternode.conf\n"

            "\nArguments:\n"
            "1. \"command\"      (string, required) \"alias\" for single masternode, \"all\" for all masternodes\n"
            "2. \"alias\"        (string, required if command is \"alias\") Alias of the masternode\n"

            "\nResult (all):\n"
            "{\n"
            "  \"overall\": \"xxx\",        (string) Overall status message indicating number of successes.\n"
            "  \"detail\": [                (array) JSON array of broadcast objects.\n"
            "    {\n"
            "      \"alias\": \"xxx\",      (string) Alias of the masternode.\n"
            "      \"success\": true|false, (boolean) Success status.\n"
            "      \"hex\": \"xxx\"         (string, if success=true) Hex encoded broadcast message.\n"
            "      \"error_message\": \"xxx\"   (string, if success=false) Error message, if any.\n"
            "    }\n"
            "    ,...\n"
            "  ]\n"
            "}\n"

            "\nResult (alias):\n"
            "{\n"
            "  \"alias\": \"xxx\",      (string) Alias of the masternode.\n"
            "  \"success\": true|false, (boolean) Success status.\n"
            "  \"hex\": \"xxx\"         (string, if success=true) Hex encoded broadcast message.\n"
            "  \"error_message\": \"xxx\"   (string, if success=false) Error message, if any.\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("createmasternodebroadcast", "alias mymn1") + HelpExampleRpc("createmasternodebroadcast", "alias mymn1"));

    if (strCommand == "alias") {
        // wait for reindex and/or import to finish
        if (fImporting || fReindex)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Wait for reindex and/or import to finish");

        std::string alias = request.params[1].get_str();
        bool found = false;

        UniValue statusObj(UniValue::VOBJ);
        statusObj.pushKV("alias", alias);

        for (CNodeEntry mne : masternodeConfig.getEntries()) {
            if (mne.getAlias() == alias) {
                CMasternodeBroadcast mnb;
                found = true;
                std::string errorMessage;
                bool fSuccess = false;
                if (!StartMasternodeEntry(statusObj, mnb, fSuccess, mne, errorMessage, strCommand))
                    continue;
                SerializeMNB(statusObj, mnb, fSuccess);
                break;
            }
        }

        if (!found) {
            statusObj.pushKV("success", false);
            statusObj.pushKV("error_message", "Could not find alias in config. Verify with listmasternodeconf.");
        }

        return statusObj;
    }

    if (strCommand == "all") {
        // wait for reindex and/or import to finish
        if (fImporting || fReindex)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Wait for reindex and/or import to finish");

        std::vector<CNodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        int successful = 0;
        int failed = 0;

        UniValue resultsObj(UniValue::VARR);

        for (CNodeEntry mne : masternodeConfig.getEntries()) {
            UniValue statusObj(UniValue::VOBJ);
            CMasternodeBroadcast mnb;
            std::string errorMessage;
            bool fSuccess = false;
            if (!StartMasternodeEntry(statusObj, mnb, fSuccess, mne, errorMessage, strCommand))
                continue;
            SerializeMNB(statusObj, mnb, fSuccess, successful, failed);
            resultsObj.push_back(statusObj);
        }

        UniValue returnObj(UniValue::VOBJ);
        returnObj.pushKV("overall", strprintf("Successfully created broadcast messages for %d masternodes, failed to create %d, total %d", successful, failed, successful + failed));
        returnObj.pushKV("detail", resultsObj);

        return returnObj;
    }
    return NullUniValue;
}

UniValue decodemasternodebroadcast(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "decodemasternodebroadcast \"hexstring\"\n"
            "\nCommand to decode masternode broadcast messages\n"

            "\nArgument:\n"
            "1. \"hexstring\"        (string) The hex encoded masternode broadcast message\n"

            "\nResult:\n"
            "{\n"
            "  \"vin\": \"xxxx\"                (string) The unspent output which is holding the masternode collateral\n"
            "  \"addr\": \"xxxx\"               (string) IP address of the masternode\n"
            "  \"pubkeycollateral\": \"xxxx\"   (string) Collateral address's public key\n"
            "  \"pubkeymasternode\": \"xxxx\"   (string) Masternode's public key\n"
            "  \"vchsig\": \"xxxx\"             (string) Base64-encoded signature of this message (verifiable via pubkeycollateral)\n"
            "  \"sigtime\": \"nnn\"             (numeric) Signature timestamp\n"
            "  \"sigvalid\": \"xxx\"            (string) \"true\"/\"false\" whether or not the mnb signature checks out.\n"
            "  \"protocolversion\": \"nnn\"     (numeric) Masternode's protocol version\n"
            "  \"nlastdsq\": \"nnn\"            (numeric) The last time the masternode sent a DSQ message (for mixing) (DEPRECATED)\n"
            "  \"lastping\" : {                 (object) JSON object with information about the masternode's last ping\n"
            "      \"vin\": \"xxxx\"            (string) The unspent output of the masternode which is signing the message\n"
            "      \"blockhash\": \"xxxx\"      (string) Current chaintip blockhash minus 12\n"
            "      \"sigtime\": \"nnn\"         (numeric) Signature time for this ping\n"
            "      \"sigvalid\": \"xxx\"        (string) \"true\"/\"false\" whether or not the mnp signature checks out.\n"
            "  }\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("decodemasternodebroadcast", "hexstring") + HelpExampleRpc("decodemasternodebroadcast", "hexstring"));

    int nDos = 0;
    CMasternodeBroadcast mnb;

    if (!DecodeHexMnb(mnb, request.params[0].get_str()))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Masternode broadcast message decode failed");

    UniValue resultObj(UniValue::VOBJ);

    resultObj.pushKV("vin", mnb.vin.prevout.ToString());
    resultObj.pushKV("addr", mnb.addr.ToString());
    resultObj.pushKV("pubkeycollateral", EncodeDestination(PKHash(mnb.pubkey.GetID())));
    resultObj.pushKV("pubkeymasternode", EncodeDestination(PKHash(mnb.pubkey2.GetID())));
    resultObj.pushKV("sigtime", mnb.sigTime);
    resultObj.pushKV("sigvalid", mnb.VerifySignature() ? "true" : "false");
    resultObj.pushKV("protocolversion", mnb.protocolVersion);
    resultObj.pushKV("nlastdsq", mnb.nLastDsq);

    UniValue lastPingObj(UniValue::VOBJ);
    lastPingObj.pushKV("vin", mnb.lastPing.vin.prevout.ToString());
    lastPingObj.pushKV("blockhash", mnb.lastPing.blockHash.ToString());
    lastPingObj.pushKV("sigtime", mnb.lastPing.sigTime);
    lastPingObj.pushKV("sigvalid", mnb.lastPing.VerifySignature(mnb.pubkey2, nDos) ? "true" : "false");

    resultObj.pushKV("lastping", lastPingObj);

    return resultObj;
}

void RegisterMasternodeCommands(CRPCTable& t)
{
    static const CRPCCommand commands[] = {
        //  category              name                         actor (function)            arguments
        //  --------------------- ------------------------     ------------------          ---------
        { "masternode", "listmasternodes", &listmasternodes, {} },
        { "masternode", "getmasternodecount", &getmasternodecount, {} },
        { "masternode", "createmasternodebroadcast", &createmasternodebroadcast, {} },
        { "masternode", "decodemasternodebroadcast", &decodemasternodebroadcast, {} },
        { "masternode", "masternodecurrent", &masternodecurrent, {} },
        { "masternode", "startmasternode", &startmasternode, {} },
        { "masternode", "createmasternodekey", &createmasternodekey, {} },
        { "masternode", "getmasternodeoutputs", &getmasternodeoutputs, {} },
        { "masternode", "listmasternodeconf", &listmasternodeconf, {} },
        { "masternode", "getmasternodestatus", &getmasternodestatus, {} },
        { "masternode", "getmasternodewinners", &getmasternodewinners, {} },
        { "masternode", "getmasternodescores", &getmasternodescores, {} },
    };

    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}

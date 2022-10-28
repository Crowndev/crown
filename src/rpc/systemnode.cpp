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
#include <systemnode/activesystemnode.h>
#include <systemnode/systemnode-payments.h>
#include <systemnode/systemnode-sync.h>
#include <systemnode/systemnodeconfig.h>
#include <systemnode/systemnodeman.h>
#include <util/moneystr.h>
#include <util/system.h>
#include <validation.h>

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

UniValue listsystemnodes(const JSONRPCRequest& request)
{
    std::string strFilter = "";

    if (request.params.size() == 1)
        strFilter = request.params[0].get_str();

    if (request.fHelp || (request.params.size() > 1))
        throw std::runtime_error(
            "listsystemnodes ( \"filter\" )\n"
            "\nGet a ranked list of systemnodes\n"

            "\nArguments:\n"
            "1. \"filter\"    (string, optional) Filter search text. Partial match by txhash, status, or addr.\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"rank\": n,           (numeric) Systemnode Rank (or 0 if not enabled)\n"
            "    \"txhash\": \"hash\",    (string) Collateral transaction hash\n"
            "    \"outidx\": n,         (numeric) Collateral transaction output index\n"
            "    \"pubkey\": \"key\",   (string) Systemnode public key used for message broadcasting\n"
            "    \"status\": s,         (string) Status (ENABLED/EXPIRED/REMOVE/etc)\n"
            "    \"addr\": \"addr\",      (string) Systemnode CRW address\n"
            "    \"version\": v,        (numeric) Systemnode protocol version\n"
            "    \"ipaddr\": a,         (string) Systemnode network address\n"
            "    \"lastseen\": ttt,     (numeric) The time in seconds since epoch (Jan 1 1970 GMT) of the last seen\n"
            "    \"activetime\": ttt,   (numeric) The time in seconds since epoch (Jan 1 1970 GMT) systemnode has been active\n"
            "    \"lastpaid\": ttt,     (numeric) The time in seconds since epoch (Jan 1 1970 GMT) systemnode was last paid\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n"
            + HelpExampleCli("listsystemnodes", "") + HelpExampleRpc("listsystemnodes", ""));

    if (!systemnodeSync.IsSynced()) {
        throw std::runtime_error("Systemnode sync has not yet completed.\n");
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

    std::vector<std::pair<int, CSystemnode>> vSystemnodeRanks = snodeman.GetSystemnodeRanks(nHeight);
    for (const auto& s : vSystemnodeRanks) {
        UniValue obj(UniValue::VOBJ);
        std::string strVin = s.second.vin.prevout.ToStringShort();
        std::string strTxHash = s.second.vin.prevout.hash.ToString();
        uint32_t oIdx = s.second.vin.prevout.n;
        CSystemnode* sn = snodeman.Find(s.second.vin);

        if (sn) {
            if (strFilter != "" && strTxHash.find(strFilter) == std::string::npos && sn->Status().find(strFilter) == std::string::npos && EncodeDestination(PKHash(sn->pubkey.GetID())).find(strFilter) == std::string::npos)
                continue;

            std::string strStatus = sn->Status();
            std::string strHost;
            int port;
            SplitHostPort(sn->addr.ToString(), port, strHost);

            obj.pushKV("rank", (strStatus == "ENABLED" ? s.first : 0));
            obj.pushKV("txhash", strTxHash);
            obj.pushKV("outidx", (uint64_t)oIdx);
            obj.pushKV("pubkey", HexStr(sn->pubkey2));
            obj.pushKV("status", strStatus);
            obj.pushKV("addr", EncodeDestination(PKHash(sn->pubkey.GetID())));
            obj.pushKV("version", sn->protocolVersion);
            obj.pushKV("ipaddr", sn->addr.ToString());
            obj.pushKV("lastseen", (int64_t)sn->lastPing.sigTime);
            obj.pushKV("activetime", (int64_t)(sn->lastPing.sigTime - sn->sigTime));
            obj.pushKV("lastpaid", (int64_t)sn->GetLastPaid());

            ret.push_back(obj);
        }
    }

    return ret;
}

UniValue getsystemnodecount(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() > 0))
        throw std::runtime_error(
            "getsystemnodecount\n"
            "\nGet systemnode count values\n"

            "\nResult:\n"
            "{\n"
            "  \"total\": n,        (numeric) Total systemnodes\n"
            "  \"stable\": n,       (numeric) Stable count\n"
            "  \"enabled\": n,      (numeric) Enabled systemnodes\n"
            "  \"inqueue\": n       (numeric) Systemnodes in queue\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("getsystemnodecount", "") + HelpExampleRpc("getsystemnodecount", ""));

    if (!systemnodeSync.IsSynced()) {
        throw std::runtime_error("Systemnode sync has not yet completed.\n");
    }

    UniValue obj(UniValue::VOBJ);
    int nCount = 0;

    if (::ChainActive().Tip())
        snodeman.GetNextSystemnodeInQueueForPayment(::ChainActive().Tip()->nHeight, true, nCount);

    obj.pushKV("total", snodeman.size());
    obj.pushKV("enabled", snodeman.CountEnabled());
    obj.pushKV("inqueue", nCount);

    return obj;
}

UniValue systemnodecurrent(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 0))
        throw std::runtime_error(
            "systemnodecurrent\n"
            "\nGet current systemnode winner (scheduled to be paid next).\n"

            "\nResult:\n"
            "{\n"
            "  \"protocol\": xxxx,        (numeric) Protocol version\n"
            "  \"txhash\": \"xxxx\",      (string) Collateral transaction hash\n"
            "  \"pubkey\": \"xxxx\",      (string) SN Public key\n"
            "  \"lastseen\": xxx,         (numeric) Time since epoch of last seen\n"
            "  \"activeseconds\": xxx,    (numeric) Seconds SN has been active\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("systemnodecurrent", "") + HelpExampleRpc("systemnodecurrent", ""));

    if (!systemnodeSync.IsSynced()) {
        throw std::runtime_error("Systemnode sync has not yet completed.\n");
    }

    const int nHeight = WITH_LOCK(cs_main, return ::ChainActive().Height() + 1);
    int nCount = 0;
    CSystemnode* winner = snodeman.GetNextSystemnodeInQueueForPayment(nHeight, true, nCount);
    if (winner) {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("protocol", (int64_t)winner->protocolVersion);
        obj.pushKV("txhash", winner->vin.prevout.hash.ToString());
        obj.pushKV("pubkey", EncodeDestination(PKHash(winner->pubkey.GetID())));
        obj.pushKV("lastseen", (winner->lastPing == CSystemnodePing() ? winner->sigTime : (int64_t)winner->lastPing.sigTime));
        obj.pushKV("activeseconds", (winner->lastPing == CSystemnodePing() ? 0 : (int64_t)(winner->lastPing.sigTime - winner->sigTime)));
        return obj;
    }

    throw std::runtime_error("unknown");
}

bool StartSystemnodeEntry(UniValue& statusObjRet, CSystemnodeBroadcast& snbRet, bool& fSuccessRet, const CNodeEntry& sne, std::string& errorMessage, std::string strCommand = "")
{
    CTxIn vin = CTxIn(uint256S(sne.getTxHash()), (uint32_t)atoi(sne.getOutputIndex()));
    CSystemnode* psn = snodeman.Find(vin);
    if (psn) {
        if (strCommand == "missing")
            return false;
        if (strCommand == "disabled" && psn->IsEnabled())
            return false;
    }

    fSuccessRet = CSystemnodeBroadcast::Create(sne.getIp(), sne.getPrivKey(), sne.getTxHash(), sne.getOutputIndex(), errorMessage, snbRet);

    statusObjRet.pushKV("alias", sne.getAlias());
    statusObjRet.pushKV("result", fSuccessRet ? "success" : "failed");
    statusObjRet.pushKV("error", fSuccessRet ? "" : errorMessage);

    return true;
}

void RelaySNB(CSystemnodeBroadcast& snb, const bool fSuccess, int& successful, int& failed)
{
    if (fSuccess) {
        successful++;
        snodeman.UpdateSystemnodeList(snb);
        snb.Relay();
    } else {
        failed++;
    }
}

void RelaySNB(CSystemnodeBroadcast& snb, const bool fSucces)
{
    int successful = 0, failed = 0;
    return RelaySNB(snb, fSucces, successful, failed);
}

void SerializeSNB(UniValue& statusObjRet, const CSystemnodeBroadcast& snb, const bool fSuccess, int& successful, int& failed)
{
    if (fSuccess) {
        successful++;
        CDataStream ssSnb(SER_NETWORK, PROTOCOL_VERSION);
        ssSnb << snb;
        statusObjRet.pushKV("hex", HexStr(ssSnb));
    } else {
        failed++;
    }
}

void SerializeSNB(UniValue& statusObjRet, const CSystemnodeBroadcast& snb, const bool fSuccess)
{
    int successful = 0, failed = 0;
    return SerializeSNB(statusObjRet, snb, fSuccess, successful, failed);
}

UniValue startsystemnode(const JSONRPCRequest& request)
{
    std::string strCommand;
    if (request.params.size() >= 1) {
        strCommand = request.params[0].get_str();

        // Backwards compatibility with legacy 'systemnode' super-command forwarder
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

    if (!systemnodeSync.IsSynced()) {
        throw std::runtime_error("Systemnode sync has not yet completed.\n");
    }

    if (request.fHelp || request.params.size() < 2 || request.params.size() > 3 || (request.params.size() == 2 && (strCommand != "local" && strCommand != "all" && strCommand != "many" && strCommand != "missing" && strCommand != "disabled")) || (request.params.size() == 3 && strCommand != "alias"))
        throw std::runtime_error(
            "startsystemnode \"local|all|many|missing|disabled|alias\" lockwallet ( \"alias\" )\n"
            "\nAttempts to start one or more systemnode(s)\n"

            "\nArguments:\n"
            "1. set         (string, required) Specify which set of systemnode(s) to start.\n"
            "2. lockwallet  (boolean, required) Lock wallet after completion.\n"
            "3. alias       (string) Systemnode alias. Required if using 'alias' as the set.\n"

            "\nResult: (for 'local' set):\n"
            "\"status\"     (string) Systemnode status message\n"

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
            + HelpExampleCli("startsystemnode", "\"alias\" \"0\" \"my_sn\"") + HelpExampleRpc("startsystemnode", "\"alias\" \"0\" \"my_sn\""));

    bool fLock = (request.params[1].get_str() == "true" ? true : false);

    if (strCommand == "local") {
        if (!fSystemNode)
            throw std::runtime_error("you must set systemnode=1 in the configuration\n");

        if (activeSystemnode.status != ACTIVE_SYSTEMNODE_STARTED) {
            activeSystemnode.status = ACTIVE_SYSTEMNODE_INITIAL;
            activeSystemnode.ManageStatus();
            if (fLock)
                GetMainWallet()->Lock();
        }

        return activeSystemnode.GetStatus();
    }

    if (strCommand == "all" || strCommand == "many" || strCommand == "missing" || strCommand == "disabled") {
        if ((strCommand == "missing" || strCommand == "disabled") && (systemnodeSync.RequestedSystemnodeAssets <= SYSTEMNODE_SYNC_LIST || systemnodeSync.RequestedSystemnodeAssets == SYSTEMNODE_SYNC_FAILED)) {
            throw std::runtime_error("You can't use this command until systemnode list is synced\n");
        }

        std::vector<CNodeEntry> snEntries;
        snEntries = systemnodeConfig.getEntries();

        int successful = 0;
        int failed = 0;

        UniValue resultsObj(UniValue::VARR);

        for (CNodeEntry sne : systemnodeConfig.getEntries()) {
            UniValue statusObj(UniValue::VOBJ);
            CSystemnodeBroadcast snb;
            std::string errorMessage;
            bool fSuccess = false;
            if (!StartSystemnodeEntry(statusObj, snb, fSuccess, sne, errorMessage, strCommand))
                continue;
            resultsObj.push_back(statusObj);
            RelaySNB(snb, fSuccess, successful, failed);
        }
        if (fLock)
            GetMainWallet()->Lock();

        UniValue returnObj(UniValue::VOBJ);
        returnObj.pushKV("overall", strprintf("Successfully started %d systemnodes, failed to start %d, total %d", successful, failed, successful + failed));
        returnObj.pushKV("detail", resultsObj);

        return returnObj;
    }

    if (strCommand == "alias") {
        std::string alias = request.params[2].get_str();

        bool found = false;

        UniValue resultsObj(UniValue::VARR);
        UniValue statusObj(UniValue::VOBJ);

        for (CNodeEntry sne : systemnodeConfig.getEntries()) {
            if (sne.getAlias() == alias) {
                CSystemnodeBroadcast snb;
                found = true;
                std::string errorMessage;
                bool fSuccess = false;
                if (!StartSystemnodeEntry(statusObj, snb, fSuccess, sne, errorMessage, strCommand))
                    continue;
                RelaySNB(snb, fSuccess);
                break;
            }
        }

        if (fLock)
            GetMainWallet()->Lock();

        if (!found) {
            statusObj.pushKV("success", false);
            statusObj.pushKV("error_message", "Could not find alias in config. Verify with listsystemnodeconf.");
        }

        return statusObj;
    }
    return NullUniValue;
}

UniValue createsystemnodekey(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 0))
        throw std::runtime_error(
            "createsystemnodekey\n"
            "\nCreate a new systemnode private key\n"

            "\nResult:\n"
            "\"key\"    (string) Systemnode private key\n"

            "\nExamples:\n"
            + HelpExampleCli("createsystemnodekey", "") + HelpExampleRpc("createsystemnodekey", ""));

    CKey secret;
    secret.MakeNewKey(false);

    return EncodeSecret(secret);
}

UniValue getsystemnodeoutputs(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 0))
        throw std::runtime_error(
            "getsystemnodeoutputs\n"
            "\nPrint all systemnode transaction outputs\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"txhash\": \"xxxx\",    (string) output transaction hash\n"
            "    \"outputidx\": n       (numeric) output index number\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n"
            + HelpExampleCli("getsystemnodeoutputs", "") + HelpExampleRpc("getsystemnodeoutputs", ""));

    // Find possible candidates
    std::vector<COutput> possibleCoins = activeSystemnode.SelectCoinsSystemnode();

    UniValue ret(UniValue::VARR);
    for (COutput& out : possibleCoins) {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("txhash", out.tx->GetHash().ToString());
        obj.pushKV("outputidx", out.i);
        ret.push_back(obj);
    }

    return ret;
}

UniValue listsystemnodeconf(const JSONRPCRequest& request)
{
    std::string strFilter = "";

    if (request.params.size() == 1)
        strFilter = request.params[0].get_str();

    if (request.fHelp || (request.params.size() > 1))
        throw std::runtime_error(
            "listsystemnodeconf ( \"filter\" )\n"
            "\nPrint systemnode.conf in JSON format\n"

            "\nArguments:\n"
            "1. \"filter\"    (string, optional) Filter search text. Partial match on alias, address, txHash, or status.\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"alias\": \"xxxx\",        (string) systemnode alias\n"
            "    \"address\": \"xxxx\",      (string) systemnode IP address\n"
            "    \"privateKey\": \"xxxx\",   (string) systemnode private key\n"
            "    \"txHash\": \"xxxx\",       (string) transaction hash\n"
            "    \"outputIndex\": n,       (numeric) transaction output index\n"
            "    \"status\": \"xxxx\"        (string) systemnode status\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n"
            + HelpExampleCli("listsystemnodeconf", "") + HelpExampleRpc("listsystemnodeconf", ""));

    std::vector<CNodeEntry> snEntries;
    snEntries = systemnodeConfig.getEntries();

    UniValue ret(UniValue::VARR);

    for (CNodeEntry sne : systemnodeConfig.getEntries()) {
        CTxIn vin = CTxIn(uint256S(sne.getTxHash()), (uint32_t)atoi(sne.getOutputIndex()));
        CSystemnode* psn = snodeman.Find(vin);

        std::string strStatus = psn ? psn->Status() : "MISSING";

        if (strFilter != "" && sne.getAlias().find(strFilter) == std::string::npos && sne.getIp().find(strFilter) == std::string::npos && sne.getTxHash().find(strFilter) == std::string::npos && strStatus.find(strFilter) == std::string::npos)
            continue;

        UniValue snObj(UniValue::VOBJ);
        snObj.pushKV("alias", sne.getAlias());
        snObj.pushKV("address", sne.getIp());
        snObj.pushKV("privateKey", sne.getPrivKey());
        snObj.pushKV("txHash", sne.getTxHash());
        snObj.pushKV("outputIndex", sne.getOutputIndex());
        snObj.pushKV("status", strStatus);
        ret.push_back(snObj);
    }

    return ret;
}

UniValue getsystemnodestatus(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 0))
        throw std::runtime_error(
            "getsystemnodestatus\n"
            "\nPrint systemnode status\n"

            "\nResult:\n"
            "{\n"
            "  \"txhash\": \"xxxx\",      (string) Collateral transaction hash\n"
            "  \"outputidx\": n,          (numeric) Collateral transaction output index number\n"
            "  \"netaddr\": \"xxxx\",     (string) Systemnode network address\n"
            "  \"addr\": \"xxxx\",        (string) CRW address for systemnode payments\n"
            "  \"status\": \"xxxx\",      (string) Systemnode status\n"
            "  \"message\": \"xxxx\"      (string) Systemnode status message\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("getsystemnodestatus", "") + HelpExampleRpc("getsystemnodestatus", ""));

    if (!fSystemNode)
        throw JSONRPCError(RPC_MISC_ERROR, ("This is not a systemnode."));

    if (activeSystemnode.vin == CTxIn())
        throw JSONRPCError(RPC_MISC_ERROR, ("Active Systemnode not initialized."));

    CSystemnode* psn = snodeman.Find(activeSystemnode.vin);

    if (psn) {
        UniValue snObj(UniValue::VOBJ);
        snObj.pushKV("txid", activeSystemnode.vin.prevout.hash.ToString());
        snObj.pushKV("outputidx", (uint64_t)activeSystemnode.vin.prevout.n);
        snObj.pushKV("netaddr", activeSystemnode.service.ToString());
        snObj.pushKV("addr", EncodeDestination(PKHash(psn->pubkey.GetID())));
        snObj.pushKV("status", activeSystemnode.GetStatus());
        return snObj;
    }
    throw std::runtime_error("Systemnode not found in the list of available systemnodes. Current status: "
        + activeSystemnode.GetStatus());
}

UniValue getsystemnodewinners(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 3)
        throw std::runtime_error(
            "getsystemnodewinners ( blocks \"filter\" )\n"
            "\nPrint the systemnode winners for the last n blocks\n"

            "\nArguments:\n"
            "1. blocks      (numeric, optional) Number of previous blocks to show (default: 10)\n"
            "2. filter      (string, optional) Search filter matching SN address\n"

            "\nResult (single winner):\n"
            "[\n"
            "  {\n"
            "    \"nHeight\": n,           (numeric) block height\n"
            "    \"winner\": {\n"
            "      \"address\": \"xxxx\",    (string) CRW SN Address\n"
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
            "        \"address\": \"xxxx\",  (string) CRW SN Address\n"
            "        \"nVotes\": n,        (numeric) Number of votes for winner\n"
            "      }\n"
            "      ,...\n"
            "    ]\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n"
            + HelpExampleCli("getsystemnodewinners", "") + HelpExampleRpc("getsystemnodewinners", ""));

    if (!systemnodeSync.IsSynced()) {
        throw std::runtime_error("Systemnode sync has not yet completed.\n");
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

        std::string strPayment = SNGetRequiredPaymentsString(i);
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

UniValue getsystemnodescores(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "getsystemnodescores ( blocks )\n"
            "\nPrint list of winning systemnode by score\n"

            "\nArguments:\n"
            "1. blocks      (numeric, optional) Show the last n blocks (default 10)\n"

            "\nResult:\n"
            "{\n"
            "  xxxx: \"xxxx\"   (numeric : string) Block height : Systemnode hash\n"
            "  ,...\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("getsystemnodescores", "") + HelpExampleRpc("getsystemnodescores", ""));

    if (!systemnodeSync.IsSynced()) {
        throw std::runtime_error("Systemnode sync has not yet completed.\n");
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

    std::vector<CSystemnode> vSystemnodes = snodeman.GetFullSystemnodeVector();
    for (int nHeight = ::ChainActive().Tip()->nHeight - nLast; nHeight < ::ChainActive().Tip()->nHeight + 20; nHeight++) {
        uint256 nHigh;
        CSystemnode* pBestSystemnode = NULL;
        for (CSystemnode& sn : vSystemnodes) {
            uint256 n = ArithToUint256(sn.CalculateScore(nHeight));
            if (UintToArith256(n) > UintToArith256(nHigh)) {
                nHigh = n;
                pBestSystemnode = &sn;
            }
        }
        if (pBestSystemnode)
            obj.pushKV(strprintf("%d", nHeight), pBestSystemnode->vin.prevout.hash.ToString().c_str());
    }

    return obj;
}

bool DecodeHexSnb(CSystemnodeBroadcast& snb, std::string strHexSnb)
{

    if (!IsHex(strHexSnb))
        return false;

    std::vector<unsigned char> snbData(ParseHex(strHexSnb));
    CDataStream ssData(snbData, SER_NETWORK, PROTOCOL_VERSION);
    try {
        ssData >> snb;
    } catch (const std::exception&) {
        return false;
    }

    return true;
}
UniValue createsystemnodebroadcast(const JSONRPCRequest& request)
{
    std::string strCommand;
    if (request.params.size() >= 1)
        strCommand = request.params[0].get_str();
    if (request.fHelp || (strCommand != "alias" && strCommand != "all") || (strCommand == "alias" && request.params.size() < 2))
        throw std::runtime_error(
            "createsystemnodebroadcast \"command\" ( \"alias\")\n"
            "\nCreates a systemnode broadcast message for one or all systemnodes configured in systemnode.conf\n"

            "\nArguments:\n"
            "1. \"command\"      (string, required) \"alias\" for single systemnode, \"all\" for all systemnodes\n"
            "2. \"alias\"        (string, required if command is \"alias\") Alias of the systemnode\n"

            "\nResult (all):\n"
            "{\n"
            "  \"overall\": \"xxx\",        (string) Overall status message indicating number of successes.\n"
            "  \"detail\": [                (array) JSON array of broadcast objects.\n"
            "    {\n"
            "      \"alias\": \"xxx\",      (string) Alias of the systemnode.\n"
            "      \"success\": true|false, (boolean) Success status.\n"
            "      \"hex\": \"xxx\"         (string, if success=true) Hex encoded broadcast message.\n"
            "      \"error_message\": \"xxx\"   (string, if success=false) Error message, if any.\n"
            "    }\n"
            "    ,...\n"
            "  ]\n"
            "}\n"

            "\nResult (alias):\n"
            "{\n"
            "  \"alias\": \"xxx\",      (string) Alias of the systemnode.\n"
            "  \"success\": true|false, (boolean) Success status.\n"
            "  \"hex\": \"xxx\"         (string, if success=true) Hex encoded broadcast message.\n"
            "  \"error_message\": \"xxx\"   (string, if success=false) Error message, if any.\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("createsystemnodebroadcast", "alias mysn1") + HelpExampleRpc("createsystemnodebroadcast", "alias mysn1"));

    if (strCommand == "alias") {
        // wait for reindex and/or import to finish
        if (fImporting || fReindex)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Wait for reindex and/or import to finish");

        std::string alias = request.params[1].get_str();
        bool found = false;

        UniValue statusObj(UniValue::VOBJ);
        statusObj.pushKV("alias", alias);

        for (CNodeEntry sne : systemnodeConfig.getEntries()) {
            if (sne.getAlias() == alias) {
                CSystemnodeBroadcast snb;
                found = true;
                std::string errorMessage;
                bool fSuccess = false;
                if (!StartSystemnodeEntry(statusObj, snb, fSuccess, sne, errorMessage, strCommand))
                    continue;
                SerializeSNB(statusObj, snb, fSuccess);
                break;
            }
        }

        if (!found) {
            statusObj.pushKV("success", false);
            statusObj.pushKV("error_message", "Could not find alias in config. Verify with listsystemnodeconf.");
        }

        return statusObj;
    }

    if (strCommand == "all") {
        // wait for reindex and/or import to finish
        if (fImporting || fReindex)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Wait for reindex and/or import to finish");

        std::vector<CNodeEntry> snEntries;
        snEntries = systemnodeConfig.getEntries();

        int successful = 0;
        int failed = 0;

        UniValue resultsObj(UniValue::VARR);

        for (CNodeEntry sne : systemnodeConfig.getEntries()) {
            UniValue statusObj(UniValue::VOBJ);
            CSystemnodeBroadcast snb;
            std::string errorMessage;
            bool fSuccess = false;
            if (!StartSystemnodeEntry(statusObj, snb, fSuccess, sne, errorMessage, strCommand))
                continue;
            SerializeSNB(statusObj, snb, fSuccess, successful, failed);
            resultsObj.push_back(statusObj);
        }

        UniValue returnObj(UniValue::VOBJ);
        returnObj.pushKV("overall", strprintf("Successfully created broadcast messages for %d systemnodes, failed to create %d, total %d", successful, failed, successful + failed));
        returnObj.pushKV("detail", resultsObj);

        return returnObj;
    }
    return NullUniValue;
}

UniValue decodesystemnodebroadcast(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "decodesystemnodebroadcast \"hexstring\"\n"
            "\nCommand to decode systemnode broadcast messages\n"

            "\nArgument:\n"
            "1. \"hexstring\"        (string) The hex encoded systemnode broadcast message\n"

            "\nResult:\n"
            "{\n"
            "  \"vin\": \"xxxx\"                (string) The unspent output which is holding the systemnode collateral\n"
            "  \"addr\": \"xxxx\"               (string) IP address of the systemnode\n"
            "  \"pubkeycollateral\": \"xxxx\"   (string) Collateral address's public key\n"
            "  \"pubkeysystemnode\": \"xxxx\"   (string) Systemnode's public key\n"
            "  \"vchsig\": \"xxxx\"             (string) Base64-encoded signature of this message (verifiable via pubkeycollateral)\n"
            "  \"sigtime\": \"nnn\"             (numeric) Signature timestamp\n"
            "  \"sigvalid\": \"xxx\"            (string) \"true\"/\"false\" whether or not the snb signature checks out.\n"
            "  \"protocolversion\": \"nnn\"     (numeric) Systemnode's protocol version\n"
            "  \"nlastdsq\": \"nnn\"            (numeric) The last time the systemnode sent a DSQ message (for mixing) (DEPRECATED)\n"
            "  \"lastping\" : {                 (object) JSON object with information about the systemnode's last ping\n"
            "      \"vin\": \"xxxx\"            (string) The unspent output of the systemnode which is signing the message\n"
            "      \"blockhash\": \"xxxx\"      (string) Current chaintip blockhash minus 12\n"
            "      \"sigtime\": \"nnn\"         (numeric) Signature time for this ping\n"
            "      \"sigvalid\": \"xxx\"        (string) \"true\"/\"false\" whether or not the snp signature checks out.\n"
            "  }\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("decodesystemnodebroadcast", "hexstring") + HelpExampleRpc("decodesystemnodebroadcast", "hexstring"));

    int nDos = 0;
    CSystemnodeBroadcast snb;

    if (!DecodeHexSnb(snb, request.params[0].get_str()))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Systemnode broadcast message decode failed");

    UniValue resultObj(UniValue::VOBJ);

    resultObj.pushKV("vin", snb.vin.prevout.ToString());
    resultObj.pushKV("addr", snb.addr.ToString());
    resultObj.pushKV("pubkeycollateral", EncodeDestination(PKHash(snb.pubkey.GetID())));
    resultObj.pushKV("pubkeysystemnode", EncodeDestination(PKHash(snb.pubkey2.GetID())));
    resultObj.pushKV("sigtime", snb.sigTime);
    resultObj.pushKV("sigvalid", snb.VerifySignature() ? "true" : "false");
    resultObj.pushKV("protocolversion", snb.protocolVersion);

    UniValue lastPingObj(UniValue::VOBJ);
    lastPingObj.pushKV("vin", snb.lastPing.vin.prevout.ToString());
    lastPingObj.pushKV("blockhash", snb.lastPing.blockHash.ToString());
    lastPingObj.pushKV("sigtime", snb.lastPing.sigTime);
    lastPingObj.pushKV("sigvalid", snb.lastPing.VerifySignature(snb.pubkey2, nDos) ? "true" : "false");

    resultObj.pushKV("lastping", lastPingObj);

    return resultObj;
}

void RegisterSystemnodeCommands(CRPCTable& t)
{
    static const CRPCCommand commands[] = {
        //  category              name                         actor (function)            arguments
        //  --------------------- ------------------------     ------------------          ---------
        { "systemnode", "listsystemnodes", &listsystemnodes, {} },
        { "systemnode", "getsystemnodecount", &getsystemnodecount, {} },
        { "systemnode", "createsystemnodebroadcast", &createsystemnodebroadcast, {} },
        { "systemnode", "decodesystemnodebroadcast", &decodesystemnodebroadcast, {} },
        { "systemnode", "systemnodecurrent", &systemnodecurrent, {} },
        { "systemnode", "startsystemnode", &startsystemnode, {} },
        { "systemnode", "createsystemnodekey", &createsystemnodekey, {} },
        { "systemnode", "getsystemnodeoutputs", &getsystemnodeoutputs, {} },
        { "systemnode", "listsystemnodeconf", &listsystemnodeconf, {} },
        { "systemnode", "getsystemnodestatus", &getsystemnodestatus, {} },
        { "systemnode", "getsystemnodewinners", &getsystemnodewinners, {} },
        { "systemnode", "getsystemnodescores", &getsystemnodescores, {} },
    };

    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}

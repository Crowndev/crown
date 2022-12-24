// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <protocol.h>

#include <util/strencodings.h>
#include <util/system.h>

static std::atomic<bool> g_initial_block_download_completed(false);

namespace NetMsgType {
const char *VERSION="version";
const char *VERACK="verack";
const char *ADDR="addr";
const char *ADDRV2="addrv2";
const char *SENDADDRV2="sendaddrv2";
const char *INV="inv";
const char *GETDATA="getdata";
const char *MERKLEBLOCK="merkleblock";
const char *GETBLOCKS="getblocks";
const char *GETHEADERS="getheaders";
const char *TX="tx";
const char *HEADERS="headers";
const char *BLOCK="block";
const char *GETADDR="getaddr";
const char *MEMPOOL="mempool";
const char *PING="ping";
const char *PONG="pong";
const char *NOTFOUND="notfound";
const char *FILTERLOAD="filterload";
const char *FILTERADD="filteradd";
const char *FILTERCLEAR="filterclear";
const char *REJECT="reject";
const char *SENDHEADERS="sendheaders";
const char *FEEFILTER="feefilter";
const char *SENDCMPCT="sendcmpct";
const char *CMPCTBLOCK="cmpctblock";
const char *GETBLOCKTXN="getblocktxn";
const char *BLOCKTXN="blocktxn";
const char *GETCFILTERS="getcfilters";
const char *CFILTER="cfilter";
const char *GETCFHEADERS="getcfheaders";
const char *CFHEADERS="cfheaders";
const char *GETCFCHECKPT="getcfcheckpt";
const char *CFCHECKPT="cfcheckpt";
const char *WTXIDRELAY="wtxidrelay";
const char *SMSGIGNORE = "smsgIgnore";
const char *SMSGPING = "smsgPing";
const char *SMSGPONG = "smsgPong";
const char *SMSGDISABLED = "smsgDisabled";
const char *SMSGSHOW = "smsgShow";
const char *SMSGMATCH = "smsgMatch";
const char *SMSGHAVE = "smsgHave";
const char *SMSGWANT = "smsgWant";
const char *SMSGMSG = "smsgMsg";
const char *SMSGINV = "smsgInv";
//! crown types
const char *BUDGETPROPOSAL = "mprop";
const char *BUDGETVOTE = "mvote";
const char *BUDGETVOTESYNC = "mnvs";
const char *DSEEP = "dseep";
const char *DSEG = "dseg";
const char *DSTX = "dstx";
const char *FINALBUDGET = "fbs";
const char *FINALBUDGETVOTE = "fbvote";
const char *GETMNWINNERS = "mnget";
const char *GETSNWINNERS = "snget";
const char *GETSPORKS = "getsporks";
const char *IX = "ix";
const char *IXLOCKLIST = "txllist";
const char *IXLOCKVOTE = "txlvote";
const char *MNBROADCAST = "mnb";
const char *MNBROADCAST2 = "mnb_new";
const char *MNPING = "mnp";
const char *MNPING2 = "mnp_new";
const char *MNSYNCSTATUS = "ssc";
const char *MNWINNER = "mnw";
const char *SNDSEG = "sndseg";
const char *SNSYNCSTATUS = "snssc";
const char *SPORK = "spork";
const char *SNBROADCAST = "snb";
const char *SNPING = "snp";
const char *SNWINNER = "snw";
const char *BLOCKPROOF = "blockproof";

} // namespace NetMsgType

static const char* ppszTypeName[] = {
        "ERROR",
        "tx",
        "block",
        "filtered block",
        "tx lock request",
        "tx lock vote",
        "spork",
        "mn winner",
        "mn scan error",
        "mn budget vote",
        "mn budget proposal",
        "mn budget finalized",
        "mn budget finalized vote",
        "mn quorum",
        "mn announce",
        "mn ping",
        "sn announce",
        "sn ping",
        "sn winner",
        "dstx"
};

/** All known message types. Keep this in the same order as the list of
 * messages above and in protocol.h.
 */
const static std::string allNetMessageTypes[] = {
    NetMsgType::VERSION,
    NetMsgType::VERACK,
    NetMsgType::ADDR,
    NetMsgType::ADDRV2,
    NetMsgType::SENDADDRV2,
    NetMsgType::INV,
    NetMsgType::GETDATA,
    NetMsgType::MERKLEBLOCK,
    NetMsgType::GETBLOCKS,
    NetMsgType::GETHEADERS,
    NetMsgType::TX,
    NetMsgType::HEADERS,
    NetMsgType::BLOCK,
    NetMsgType::GETADDR,
    NetMsgType::MEMPOOL,
    NetMsgType::PING,
    NetMsgType::PONG,
    NetMsgType::NOTFOUND,
    NetMsgType::FILTERLOAD,
    NetMsgType::FILTERADD,
    NetMsgType::FILTERCLEAR,
    NetMsgType::REJECT,
    NetMsgType::SENDHEADERS,
    NetMsgType::FEEFILTER,
    NetMsgType::SENDCMPCT,
    NetMsgType::CMPCTBLOCK,
    NetMsgType::GETBLOCKTXN,
    NetMsgType::BLOCKTXN,
    NetMsgType::GETCFILTERS,
    NetMsgType::CFILTER,
    NetMsgType::GETCFHEADERS,
    NetMsgType::CFHEADERS,
    NetMsgType::GETCFCHECKPT,
    NetMsgType::CFCHECKPT,
    NetMsgType::WTXIDRELAY,
    NetMsgType::SMSGIGNORE,
    NetMsgType::SMSGPING,
    NetMsgType::SMSGPONG,
    NetMsgType::SMSGDISABLED,
    NetMsgType::SMSGSHOW,
    NetMsgType::SMSGMATCH,
    NetMsgType::SMSGHAVE,
    NetMsgType::SMSGWANT,
    NetMsgType::SMSGMSG,
    NetMsgType::SMSGINV,
    //! crown types
    NetMsgType::BUDGETPROPOSAL,
    NetMsgType::BUDGETVOTE,
    NetMsgType::BUDGETVOTESYNC,
    NetMsgType::DSEEP,
    NetMsgType::DSEG,
    NetMsgType::DSTX,
    NetMsgType::FINALBUDGET,
    NetMsgType::FINALBUDGETVOTE,
    NetMsgType::GETMNWINNERS,
    NetMsgType::GETSNWINNERS,
    NetMsgType::GETSPORKS,
    NetMsgType::IX,
    NetMsgType::IXLOCKLIST,
    NetMsgType::IXLOCKVOTE,
    NetMsgType::MNBROADCAST,
    NetMsgType::MNBROADCAST2,
    NetMsgType::MNPING,
    NetMsgType::MNPING2,
    NetMsgType::MNSYNCSTATUS,
    NetMsgType::MNWINNER,
    NetMsgType::SNDSEG,
    NetMsgType::SNSYNCSTATUS,
    NetMsgType::SPORK,
    NetMsgType::SNBROADCAST,
    NetMsgType::SNPING,
    NetMsgType::SNWINNER,
    NetMsgType::BLOCKPROOF
};
const static std::vector<std::string> allNetMessageTypesVec(allNetMessageTypes, allNetMessageTypes+ARRAYLEN(allNetMessageTypes));

CMessageHeader::CMessageHeader()
{
    memset(pchMessageStart, 0, MESSAGE_START_SIZE);
    memset(pchCommand, 0, sizeof(pchCommand));
    nMessageSize = -1;
    memset(pchChecksum, 0, CHECKSUM_SIZE);
}

CMessageHeader::CMessageHeader(const MessageStartChars& pchMessageStartIn, const char* pszCommand, unsigned int nMessageSizeIn)
{
    memcpy(pchMessageStart, pchMessageStartIn, MESSAGE_START_SIZE);

    // Copy the command name, zero-padding to COMMAND_SIZE bytes
    size_t i = 0;
    for (; i < COMMAND_SIZE && pszCommand[i] != 0; ++i) pchCommand[i] = pszCommand[i];
    assert(pszCommand[i] == 0); // Assert that the command name passed in is not longer than COMMAND_SIZE
    for (; i < COMMAND_SIZE; ++i) pchCommand[i] = 0;

    nMessageSize = nMessageSizeIn;
    memset(pchChecksum, 0, CHECKSUM_SIZE);
}

std::string CMessageHeader::GetCommand() const
{
    return std::string(pchCommand, pchCommand + strnlen(pchCommand, COMMAND_SIZE));
}

bool CMessageHeader::IsCommandValid() const
{
    // Check the command string for errors
    for (const char* p1 = pchCommand; p1 < pchCommand + COMMAND_SIZE; ++p1) {
        if (*p1 == 0) {
            // Must be all zeros after the first zero
            for (; p1 < pchCommand + COMMAND_SIZE; ++p1) {
                if (*p1 != 0) {
                    return false;
                }
            }
        } else if (*p1 < ' ' || *p1 > 0x7E) {
            return false;
        }
    }

    return true;
}


ServiceFlags GetDesirableServiceFlags(ServiceFlags services) {
    if ((services & NODE_NETWORK_LIMITED) && g_initial_block_download_completed) {
        return ServiceFlags(NODE_NETWORK_LIMITED | NODE_WITNESS);
    }
    return ServiceFlags(NODE_NETWORK | NODE_WITNESS);
}

void SetServiceFlagsIBDCache(bool state) {
    g_initial_block_download_completed = state;
}

CInv::CInv()
{
    type = 0;
    hash.SetNull();
}

CInv::CInv(int typeIn, const uint256& hashIn) : type(typeIn), hash(hashIn) {}

CInv::CInv(const std::string& strType, const uint256& hashIn)
{
    unsigned int i;
    for (i = 1; i < ARRAYLEN(ppszTypeName); i++) {
        if (strType == ppszTypeName[i]) {
            type = i;
            break;
        }
    }
    if (i == ARRAYLEN(ppszTypeName))
        LogPrint(BCLog::NET, "CInv::CInv(string, uint256) : unknown type '%s'", strType);
    hash = hashIn;
}

bool operator<(const CInv& a, const CInv& b)
{
    return (a.type < b.type || (a.type == b.type && a.hash < b.hash));
}

bool CInv::IsKnownType() const
{
    return (type >= 1 && type < (int)ARRAYLEN(ppszTypeName));
}

std::string CInv::GetCommand() const
{
    std::string cmd;
    if (type & MSG_WITNESS_FLAG)
        cmd.append("witness-");
    int masked = type & MSG_TYPE_MASK;
    switch (masked) {
        case MSG_TX:             return cmd.append(NetMsgType::TX);
        case MSG_WTX:            return cmd.append("wtx"); 
        case MSG_BLOCK:          return cmd.append(NetMsgType::BLOCK);
        case MSG_FILTERED_BLOCK: return cmd.append(NetMsgType::MERKLEBLOCK);
        case MSG_CMPCT_BLOCK:    return cmd.append(NetMsgType::CMPCTBLOCK);
        default:
            //! unconventional but works..
            if (!IsKnownType()) {
                LogPrint(BCLog::NET, "CInv::GetCommand() : type=%d unknown type", type);
                return "UNKNOWN";
            }
            return ppszTypeName[type];
    }
    return "";
}

std::string CInv::ToString() const
{
    try {
        return strprintf("%s %s", GetCommand(), hash.ToString());
    } catch(const std::out_of_range &) {
        return strprintf("0x%08x %s", type, hash.ToString());
    }
}

const std::vector<std::string> &getAllNetMessageTypes()
{
    return allNetMessageTypesVec;
}

/**
 * Convert a service flag (NODE_*) to a human readable string.
 * It supports unknown service flags which will be returned as "UNKNOWN[...]".
 * @param[in] bit the service flag is calculated as (1 << bit)
 */
static std::string serviceFlagToStr(size_t bit)
{
    const uint64_t service_flag = 1ULL << bit;
    switch ((ServiceFlags)service_flag) {
    case NODE_NONE: abort();  // impossible
    case NODE_NETWORK:         return "NETWORK";
    case NODE_GETUTXO:         return "GETUTXO";
    case NODE_BLOOM:           return "BLOOM";
    case NODE_WITNESS:         return "WITNESS";
    case NODE_COMPACT_FILTERS: return "COMPACT_FILTERS";
    case NODE_NETWORK_LIMITED: return "NETWORK_LIMITED";
    // Not using default, so we get warned when a case is missing
    }

    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << "UNKNOWN[";
    stream << "2^" << bit;
    stream << "]";
    return stream.str();
}

std::vector<std::string> serviceFlagsToStr(uint64_t flags)
{
    std::vector<std::string> str_flags;

    for (size_t i = 0; i < sizeof(flags) * 8; ++i) {
        if (flags & (1ULL << i)) {
            str_flags.emplace_back(serviceFlagToStr(i));
        }
    }

    return str_flags;
}

GenTxid ToGenTxid(const CInv& inv)
{
    assert(inv.IsGenTxMsg());
    return {inv.IsMsgWtx(), inv.hash, inv.type};
}

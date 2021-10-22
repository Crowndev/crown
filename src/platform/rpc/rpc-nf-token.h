// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_RPC_NF_TOKEN_H
#define CROWN_PLATFORM_RPC_NF_TOKEN_H

#include "json/json_spirit_value.h"

UniValue nftoken(const JSONRPCRequest& request);

namespace Platform
{
    UniValue RegisterNfToken(const JSONRPCRequest& request);
    UniValue ListNfTokenTxs(const JSONRPCRequest& request);
    UniValue GetNfToken(const JSONRPCRequest& request);
    UniValue GetNfTokenByTxId(const JSONRPCRequest& request);
    UniValue NfTokenTotalSupply(const JSONRPCRequest& request);
    UniValue NfTokenBalanceOf(const JSONRPCRequest& request);
    UniValue NfTokenOwnerOf(const JSONRPCRequest& request);
}

#endif // CROWN_PLATFORM_RPC_NF_TOKEN_H

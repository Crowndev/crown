// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_RPC_NF_TOKEN_H
#define CROWN_PLATFORM_RPC_NF_TOKEN_H

UniValue nftoken(const JSONRPCRequest& request);

namespace Platform
{
    UniValue RegisterNfToken(const UniValue& params);
    UniValue ListNfTokenTxs(const UniValue& params);
    UniValue GetNfToken(const UniValue& params);
    UniValue GetNfTokenByTxId(const UniValue& params);
    UniValue NfTokenTotalSupply(const UniValue& params);
    UniValue NfTokenBalanceOf(const UniValue& params);
    UniValue NfTokenOwnerOf(const UniValue& params);
}

#endif // CROWN_PLATFORM_RPC_NF_TOKEN_H

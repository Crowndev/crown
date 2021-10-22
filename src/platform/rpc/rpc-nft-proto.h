// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PROJECT_RPC_NFT_PROTO_H
#define PROJECT_RPC_NFT_PROTO_H

#include "json/json_spirit_value.h"

UniValue nftproto(const JSONRPCRequest& request);

namespace Platform
{
    UniValue RegisterNftProtocol(const JSONRPCRequest& request);
    void RegisterNftProtocolHelp();
    UniValue ListNftProtocols(const JSONRPCRequest& request);
    void ListNftProtocolsHelp();
    UniValue GetNftProtocol(const JSONRPCRequest& request);
    void GetNftProtocolHelp();
    UniValue GetNftProtocolByTxId(const JSONRPCRequest& request);
    void GetNftProtocolByTxIdHelp();
    UniValue NftProtoOwnerOf(const JSONRPCRequest& request);
    void NftProtoOwnerOfHelp();
    UniValue NftProtoTotalSupply(const JSONRPCRequest& request);
    void NftProtoTotalSupplyHelp();
}

#endif // PROJECT_RPC_NFT_PROTO_H

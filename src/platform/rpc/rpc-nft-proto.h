// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PROJECT_RPC_NFT_PROTO_H
#define PROJECT_RPC_NFT_PROTO_H

class UniValue;

UniValue nftproto(const JSONRPCRequest& request);

namespace Platform
{
    UniValue RegisterNftProtocol(const UniValue& params);
    void RegisterNftProtocolHelp();
    UniValue ListNftProtocols(const UniValue& params);
    void ListNftProtocolsHelp();
    UniValue GetNftProtocol(const UniValue& params);
    void GetNftProtocolHelp();
    UniValue GetNftProtocolByTxId(const UniValue& params);
    void GetNftProtocolByTxIdHelp();
    UniValue NftProtoOwnerOf(const UniValue& params);
    void NftProtoOwnerOfHelp();
    UniValue NftProtoTotalSupply(const UniValue& params);
    void NftProtoTotalSupplyHelp();
}

#endif // PROJECT_RPC_NFT_PROTO_H

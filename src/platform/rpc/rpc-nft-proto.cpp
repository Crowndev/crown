// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crown/spork.h>
#include <key_io.h>

#include <platform/nf-token/nf-token-protocol.h>
#include <platform/nf-token/nft-protocol-reg-tx-builder.h>
#include <platform/nf-token/nft-protocols-manager.h>
#include <primitives/transaction.h>
#include <platform/specialtx.h>
#include <platform/platform-utils.h>
#include <platform/rpc/specialtx-rpc-utils.h>
#include <platform/rpc/rpc-nft-proto.h>
#include <rpc/server.h>

UniValue nftproto(const JSONRPCRequest& request)
{
    if (!IsSporkActive(SPORK_17_NFT_TX))
    {
        throw std::runtime_error("NFT spork is off");
    }

    std::string command = Platform::GetCommand(request.params, "usage: nftproto register|list|get|getbytxid|ownerof");

    if (command == "register")
        return Platform::RegisterNftProtocol(request.params);
    else if (command == "list")
        return Platform::ListNftProtocols(request.params);
    else if (command == "get")
        return Platform::GetNftProtocol(request.params);
    else if (command == "getbytxid")
        return Platform::GetNftProtocolByTxId(request.params);
    else if (command == "ownerof")
        return Platform::NftProtoOwnerOf(request.params);
    else if (command == "totalsupply")
        return Platform::NftProtoTotalSupply(request.params);

    throw std::runtime_error("Invalid command: " + command);
}

namespace Platform
{
    UniValue BuildNftProtoRecord(const NftProtoIndex & nftProtoIndex)
    {
        UniValue protoJsonObj(UniValue::VOBJ);

        protoJsonObj.pushKV("blockHash", nftProtoIndex.BlockIndex()->phashBlock->ToString());
        protoJsonObj.pushKV("registrationTxHash", nftProtoIndex.RegTxHash().ToString());
        protoJsonObj.pushKV("height", nftProtoIndex.BlockIndex()->nHeight);
        //auto blockTime = static_cast<time_t>(nftProtoIndex.BlockIndex()->nTime);
        //std::string timeStr(asctime(gmtime(&blockTime)));
        //protoJsonObj.pushKV("timestamp", timeStr));
        protoJsonObj.pushKV("timestamp", nftProtoIndex.BlockIndex()->GetBlockTime());

        protoJsonObj.pushKV("nftProtocolId", ProtocolName{nftProtoIndex.NftProtoPtr()->tokenProtocolId}.ToString());
        protoJsonObj.pushKV("tokenProtocolName", nftProtoIndex.NftProtoPtr()->tokenProtocolName);
        protoJsonObj.pushKV("tokenMetadataSchemaUri", nftProtoIndex.NftProtoPtr()->tokenMetadataSchemaUri);
        protoJsonObj.pushKV("tokenMetadataMimeType", nftProtoIndex.NftProtoPtr()->tokenMetadataMimeType);
        protoJsonObj.pushKV("isTokenTransferable", nftProtoIndex.NftProtoPtr()->isTokenTransferable);
        protoJsonObj.pushKV("isMetadataEmbedded", nftProtoIndex.NftProtoPtr()->isMetadataEmbedded);
        auto nftRegSignStr = NftRegSignToString(static_cast<NftRegSign>(nftProtoIndex.NftProtoPtr()->nftRegSign));
        protoJsonObj.pushKV("nftRegSign", nftRegSignStr);
        protoJsonObj.pushKV("maxMetadataSize", nftProtoIndex.NftProtoPtr()->maxMetadataSize);
        protoJsonObj.pushKV("tokenProtocolOwnerId", EncodeDestination(PKHash(nftProtoIndex.NftProtoPtr()->tokenProtocolOwnerId)));

        return protoJsonObj;
    }

    UniValue RegisterNftProtocol(const UniValue& params)
    {
        CKey ownerKey;
        NftProtocolRegTxBuilder txBuilder;
        txBuilder.SetTokenProtocol(params[1]).SetTokenProtocolName(params[2]).SetTokenProtocolOwnerKey(params[3], ownerKey);

        if (params.size() > 4)
            txBuilder.SetNftRegSign(params[4]);
        if (params.size() > 5)
            txBuilder.SetMetadataMimeType(params[5]);
        if (params.size() > 6)
            txBuilder.SetMetadataSchemaUri(params[6]);
        if (params.size() > 7)
            txBuilder.SetIsTokenTransferable(params[7]);
        if (params.size() > 8)
            txBuilder.SetIsMetadataEmbedded(params[8]);
        if (params.size() > 9)
            txBuilder.SetMaxMetadataSize(params[9]);

        auto nftProtoRegTx = txBuilder.BuildTx();

        CMutableTransaction tx;
        tx.nVersion = 3;
        tx.nType = TRANSACTION_NF_TOKEN_PROTOCOL_REGISTER;

        FundSpecialTx(tx, nftProtoRegTx);
        SignSpecialTxPayload(tx, nftProtoRegTx, ownerKey);
        SetNftTxPayload(tx, nftProtoRegTx);

        std::string result = SignAndSendSpecialTx(tx);
        return result;
    }

    void RegisterNftProtocolHelp()
    {
        static std::string helpMessage =
                R"(nftproto register
Register a new NFT protocol

Arguments:
1. "tokenProtocolId"         (string, required) A non-fungible token protocol ID/symbol
                             NF token protocol unique symbol/identifier, can be an a abbreviated name describing this NF token type.
                             Represented as a base32 string, can only contain characters: .abcdefghijklmnopqrstuvwxyz12345
                             Minimum length 3 symbols, maximum length 12 symbols
2. "tokenProtocolName"       (string, required) Full readable name for this NF token type/protocol.
                             Minimum length 3 symbols, maximum length 24 symbols
3. "tokenProtocolOwnerAddr"  (string, required) The NFT protocol owner address, it can be used to generate new NFTs or update the protocol settings
                             The private key belonging to this address MUST be known to your wallet.
4. "nftRegSign"              (numeric, optional) Defines who must sign an NFT registration transaction.
                             1 (SelfSign): every transaction should be signed by the NFT owner
                             2 (SignByCreator): every transaction should be signed by the owner/creator of the NFT protocol
                             3 (SignPayer): every transaction should be signed by a payer key
                             Default: SignByCreator
5. "tokenMetadataMimeType"   (string, optional) MIME type describing metadata content type
                             Examples: application/json, text/plain, application/octet-stream etc. Default: text/plain
6. "tokenMetadataSchemaUri"  (string, optional) URI to schema (json/xml/binary) describing metadata format.
                             Default: "". Arbitrary data can be written by default.

7. "isTokenTransferable"     (string, optional) Defines if this NF token type can be transferred. Default: true
8. "isMetadataEmbedded"      (string, optional) Defines if metadata is embedded or contains a URI
                             It's recommended to use embedded metadata only if it's shorter than the URI. Default: false
9 "maxMetadataSize"          (number, optional) Defines maximum metadata length for the NFT. Absolute max length is 255.
                             So the value must be <= 255. Default: 255.
Examples:
)"
+ HelpExampleCli("nftproto", R"(register "doc" "Doc Proof" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 3 "text/plain" "" true false)")
+ HelpExampleCli("nftproto", R"(register "crd" "Crown ID" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 1 "application/octet-stream" "https://binary-schema" false false)")
+ HelpExampleCli("nftproto", R"(register "crd" "Crown ID" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 1 "application/octet-stream" "https://binary-schema" false false 64)")
+ HelpExampleRpc("nftproto", R"(register "doc" "Doc Proof" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 3 "text/plain" "" true true false)")
+ HelpExampleRpc("nftproto", R"(register "crd" "Crown ID" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 1 "application/octet-stream" "https://binary-schema" false false 127)")
+ HelpExampleRpc("nftproto", R"(register "cks" "ERC721 CryptoKitties" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 2 "text/plain" "" true false)");

        throw std::runtime_error(helpMessage);
    }

    UniValue ListNftProtocols(const UniValue& params)
    {
        static const unsigned int defaultTxsCount = 20;
        static const unsigned int defaultSkipFromTip = 0;
        unsigned int count =  defaultTxsCount;
        unsigned int skipFromTip = defaultSkipFromTip;

        unsigned int height = ::ChainActive().Height();
        if (height < 0 || height > ::ChainActive().Height())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height is out of range");

        bool regTxOnly =  false;
        UniValue protoList(UniValue::VARR);

        auto protoHandler = [&](const NftProtoIndex & protoIndex) -> bool
        {
            if (regTxOnly)
            {
                UniValue hashObj(UniValue::VOBJ);
                hashObj.pushKV("registrationTxHash", protoIndex.RegTxHash().GetHex());
                protoList.push_back(hashObj);
            }
            else
            {
                protoList.push_back(BuildNftProtoRecord(protoIndex));
            }
            return true;
        };

        NftProtocolsManager::Instance().ProcessNftProtoIndexRangeByHeight(protoHandler, height, count, skipFromTip);
        return protoList;
    }

    void ListNftProtocolsHelp()
    {
        static std::string helpMessage =
                R"(nftproto list
Lists all NFT protocol records on chain

Arguments:
1. count       (numeric, optional, default=20) The number of transactions to return
2. skipFromTip (numeric, optional, default=0) The number of transactions to skip from tip
3. height      (numeric, optional) If height is not specified, it defaults to the current chain-tip.
               To explicitly use the current tip height, set it to "*".
4. regTxOnly   (boolean, optional, default=false) false for a detailed list, true for an array of transaction IDs

Examples:
List the most recent 20 NFT protocol records
)"
+ HelpExampleCli("nftproto", R"(list)")
+ R"(List the most recent 20 NFT protocol records up to 5050st block
)"
+ HelpExampleCli("nftproto", R"(list 20 0 5050)")
+ R"(List recent 100 records skipping 50 from the end up to 5050st block
)"
+ HelpExampleCli("nftproto", R"(list 100 50 5050)")
+ R"(List recent 100 records skipping 50 from the end up to the most recent block. List only registration tx IDs.
)"
+ HelpExampleCli("nftproto", R"(list 100 50 * true)")
+ R"(As JSON-RPC calls
)"
+ HelpExampleRpc("nftproto", R"(list)")
+ HelpExampleRpc("nftproto", R"(list 20 0 5050)")
+ HelpExampleCli("nftproto", R"(list 100 50 5050)");

        throw std::runtime_error(helpMessage);
    }

    UniValue GetNftProtocol(const UniValue& params)
    {
        uint64_t tokenProtocolId = StringToProtocolName(params[1].get_str().c_str());

        auto nftProtoIndex = NftProtocolsManager::Instance().GetNftProtoIndex(tokenProtocolId);
        if (nftProtoIndex.IsNull())
            throw std::runtime_error("Can't find an NFT protocol record: " + params[1].get_str());

        return BuildNftProtoRecord(nftProtoIndex);
    }

    void GetNftProtocolHelp()
    {
        static std::string helpMessage =
                R"(nftproto get
Get an NFT protocol record by an NFT protocol ID
Arguments:

1. "nfTokenProtocol"  (string, required) The non-fungible token protocol symbol of the registered token record
                      The protocol name must be valid and registered previously.
Examples:
)"
+ HelpExampleCli("nftproto", R"(get "doc")")
+ HelpExampleRpc("nftproto", R"(get "cks")");

        throw std::runtime_error(helpMessage);
    }

    void GetNftProtocolByTxIdHelp()
    {
        static std::string helpMessage =
                R"(nftproto getbytxid
Get an NFT protocol record by a transaction ID
Arguments:

1. "registrationTxId"   (string, required) The NFT protocol registration transaction ID

Examples:
)"
+ HelpExampleCli("nftproto", R"(getbytxid "3840804e62350b6337ca0b4653547477aa46dab2677c0514a8dccf80b51a899a")")
+ HelpExampleRpc("nftproto", R"(getbytxid "3840804e62350b6337ca0b4653547477aa46dab2677c0514a8dccf80b51a899a")");

        throw std::runtime_error(helpMessage);
    }

    UniValue GetNftProtocolByTxId(const UniValue& params)
    {
        uint256 regTxHash = ParseHashV(params[1].get_str(), "registrationTxHash");
        auto nftProtoIndex = NftProtocolsManager::Instance().GetNftProtoIndex(regTxHash);
        if (nftProtoIndex.IsNull())
            throw std::runtime_error("Can't find an NFT protocol record: " + params[1].get_str());

        return BuildNftProtoRecord(nftProtoIndex);
    }

    void NftProtoOwnerOfHelp()
    {
        static std::string helpMessage = R"(nftproto ownerof
Get address of the NFT protocol owner by using its protocol symbol

1. "nfTokenProtocol"  (string, required) The non-fungible token protocol symbol of the registered protocol
                      The protocol name must be valid and registered previously.

Examples:
)"
+ HelpExampleCli("nftproto", R"(ownerof "doc")")
+ HelpExampleRpc("nftproto", R"(ownerof "doc")");

        throw std::runtime_error(helpMessage);
    }

    UniValue NftProtoOwnerOf(const UniValue& params)
    {
        uint64_t tokenProtocolId = StringToProtocolName(params[1].get_str().c_str());
        if (tokenProtocolId == NfToken::UNKNOWN_TOKEN_PROTOCOL)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "NFT protocol ID contains invalid characters");

        CKeyID ownerId = NftProtocolsManager::Instance().OwnerOf(tokenProtocolId);
        if (ownerId.IsNull())
            throw std::runtime_error("Can't find an NFT protocol: " + std::to_string(tokenProtocolId));

        return EncodeDestination(PKHash(ownerId));
    }

    UniValue NftProtoTotalSupply(const UniValue& params)
    {
        unsigned int totalSupply = NftProtocolsManager::Instance().TotalSupply();
        return static_cast<uint64_t>(totalSupply);
    }

    void NftProtoTotalSupplyHelp()
    {
        static std::string helpMessage = R"(nftproto totalsupply
Get NFT protocols current total supply
Examples:
)"
+ HelpExampleCli("nftproto",   "totalsupply")
+ HelpExampleRpc("nftproto", R"(totalsupply)");

        throw std::runtime_error(helpMessage);
    }
}

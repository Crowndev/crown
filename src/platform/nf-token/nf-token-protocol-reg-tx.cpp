// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "nf-token-protocol-reg-tx.h"
#include "primitives/transaction.h"
#include "platform/platform-utils.h"
#include "platform/specialtx.h"
#include "nft-protocols-manager.h"
#include "nf-tokens-manager.h"
#include "consensus/validation.h"
#include <univalue/include/univalue.h>
#include "key_io.h"

#include "sync.h"

namespace Platform
{
    bool NfTokenProtocolRegTx::CheckTx(const CTransaction& tx, const CBlockIndex* pindexLast, TxValidationState& state)
    {
        AssertLockHeld(cs_main);

        NfTokenProtocolRegTx nftProtoRegTx;
        if (!GetNftTxPayload(tx, nftProtoRegTx))
            return state.Invalid(TxValidationResult::TX_CONSENSUS,  "bad-tx-payload");

        const NfTokenProtocol & nftProto = nftProtoRegTx.GetNftProto();

        if (nftProtoRegTx.m_version != NfTokenProtocolRegTx::CURRENT_VERSION)
            return state.Invalid(TxValidationResult::TX_CONSENSUS,  "bad-nft-proto-reg-tx-version");

        if (nftProto.tokenProtocolId == NfToken::UNKNOWN_TOKEN_PROTOCOL)
            return state.Invalid(TxValidationResult::TX_CONSENSUS,  "bad-nft-proto-reg-tx-token-protocol");

        if (nftProto.tokenProtocolOwnerId.IsNull())
            return state.Invalid(TxValidationResult::TX_CONSENSUS,  "bad-nft-proto-reg-tx-owner-key-null");

        if (nftProto.nftRegSign < static_cast<uint8_t>(NftRegSignMin) || nftProto.nftRegSign > static_cast<uint8_t>(NftRegSignMax))
            return state.Invalid(TxValidationResult::TX_CONSENSUS,  "bad-nft-proto-reg-tx-reg-sign");

        if (nftProto.tokenProtocolName.size() < NfTokenProtocol::TOKEN_PROTOCOL_NAME_MIN
        || nftProto.tokenProtocolName.size() > NfTokenProtocol::TOKEN_PROTOCOL_NAME_MAX)
            return state.Invalid(TxValidationResult::TX_CONSENSUS,  "bad-nft-proto-reg-tx-proto-name");

        if (nftProto.tokenMetadataMimeType.size() > NfTokenProtocol::TOKEN_METADATA_MIMETYPE_MAX)
            return state.Invalid(TxValidationResult::TX_CONSENSUS,  "bad-nft-proto-reg-tx-metadata-mime-type");

        if (nftProto.tokenMetadataSchemaUri.size() > NfTokenProtocol::TOKEN_METADATA_SCHEMA_URI_MAX)
            return state.Invalid(TxValidationResult::TX_CONSENSUS,  "bad-nft-proto-reg-tx-metadata-schema-uri");

        if (nftProto.maxMetadataSize > NfTokenProtocol::TOKEN_METADATA_ABSOLUTE_MAX)
            return state.Invalid(TxValidationResult::TX_CONSENSUS,  "bad-nft-proto-reg-tx-metadata-max-size-too-big");

        //if (pindexLast != nullptr)
        //{
        //    if (NftProtocolsManager::Instance().Contains(nftProto.tokenProtocolId, pindexLast->nHeight))
        //        return state.Invalid(TxValidationResult::TX_CONSENSUS, false, REJECT_DUPLICATE, "bad-nft-proto-reg-tx-dup-protocol");
        //}

        //if (!CheckInputsHashAndSig(tx, nftProtoRegTx, nftProto.tokenProtocolOwnerId, state))
        //    return state.DoS(50,  "bad-nft-proto-reg-tx-invalid-signature");

        return true;
    }

    bool NfTokenProtocolRegTx::ProcessTx(const CTransaction & tx, const CBlockIndex * pindex, TxValidationState & state)
    {
        NfTokenProtocolRegTx nftProtoRegTx;
        bool result = GetNftTxPayload(tx, nftProtoRegTx);
        // should have been checked already
        assert(result);

        auto nftProto = nftProtoRegTx.GetNftProto();

        if (!NftProtocolsManager::Instance().AddNftProto(nftProto, tx, pindex))
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "nft-proto-reg-tx-conflict");
        NfTokensManager::Instance().OnNewProtocolRegistered(nftProto.tokenProtocolId);
        return true;
    }

    bool NfTokenProtocolRegTx::UndoTx(const CTransaction & tx, const CBlockIndex * pindex)
    {
        NfTokenProtocolRegTx nftProtoRegTx;
        bool result = GetNftTxPayload(tx, nftProtoRegTx);
        // should have been checked already
        assert(result);

        auto nftProto = nftProtoRegTx.GetNftProto();
        return NftProtocolsManager::Instance().Delete(nftProto.tokenProtocolId, pindex->nHeight);
    }

    void NfTokenProtocolRegTx::ToJson(UniValue& result) const
    {
        result.pushKV("version", m_version);
        result.pushKV("nftProtocolId", ProtocolName{m_nfTokenProtocol.tokenProtocolId}.ToString());
        result.pushKV("tokenProtocolName", m_nfTokenProtocol.tokenProtocolName);
        result.pushKV("tokenMetadataSchemaUri", m_nfTokenProtocol.tokenMetadataSchemaUri);
        result.pushKV("tokenMetadataMimeType", m_nfTokenProtocol.tokenMetadataMimeType);
        result.pushKV("isTokenTransferable", m_nfTokenProtocol.isTokenTransferable);
        result.pushKV("isMetadataEmbedded", m_nfTokenProtocol.isMetadataEmbedded);
        auto nftRegSignStr = NftRegSignToString(static_cast<NftRegSign>(m_nfTokenProtocol.nftRegSign));
        result.pushKV("nftRegSign", nftRegSignStr);
        result.pushKV("maxMetadataSize", m_nfTokenProtocol.maxMetadataSize);
        result.pushKV("tokenProtocolOwnerId", EncodeDestination(PKHash(m_nfTokenProtocol.tokenProtocolOwnerId)));
    }


    std::string NfTokenProtocolRegTx::ToString() const
    {
        std::ostringstream out;
        out << "NfTokenProtocolRegTx(nft protocol ID=" << ProtocolName{m_nfTokenProtocol.tokenProtocolId}.ToString()
        << ", nft protocol name=" << m_nfTokenProtocol.tokenProtocolName << ", nft metadata schema url=" << m_nfTokenProtocol.tokenMetadataSchemaUri
        << ", nft metadata mimetype=" << m_nfTokenProtocol.tokenMetadataMimeType << ", transferable=" << m_nfTokenProtocol.isTokenTransferable
        << ", is metadata embedded" << m_nfTokenProtocol.isMetadataEmbedded << ", max metadata size=" << m_nfTokenProtocol.maxMetadataSize
        << ", nft Protocol owner ID=" << EncodeDestination(PKHash(m_nfTokenProtocol.tokenProtocolOwnerId)) << ")";
        return out.str();
    }
}

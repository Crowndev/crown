// Copyright (c) 2020-2021 Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <nft/token.h>

#include <chainparams.h>
#include <clientversion.h>
#include <consensus/validation.h>
#include <hash.h>
#include <key_io.h>
#include <nft/nftdb.h>
#include <nft/specialtx.h>
#include <nft/util.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <util/system.h>

class NftDb;
class NfToken;
class NfTokenRegTx;
class NfTokenProtocol;
class NfTokenProtocolRegTx;

//! token database
NftDb defaultDb;

//! token register
bool ContextualCheckTokenRegister(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state)
{
    AssertLockHeld(cs_main);
    if (tx.nType != TRANSACTION_NF_TOKEN_REGISTER) {
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, "bad-protx-type");
    }
    return true;
}

bool CheckTokenRegister(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state)
{
    AssertLockHeld(cs_main);
    NfTokenRegTx nftRegTx;
    if (!GetNftTxPayload(tx, nftRegTx)) {
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, "bad-tx-payload");
    }
    NfToken tokenExtract = nftRegTx.getToken();
    return CheckToken(tokenExtract, pindexPrev, state);
}

bool CheckToken(NfToken token, const CBlockIndex* pindexPrev, TxValidationState& state)
{
    return true;
}

//! token protocol register
bool ContextualCheckTokenProtocolRegister(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state)
{
    AssertLockHeld(cs_main);
    if (tx.nType != TRANSACTION_NF_TOKEN_PROTOCOL_REGISTER) {
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, "bad-protx-type");
    }
    return true;
}

bool CheckTokenProtocolRegister(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state)
{
    AssertLockHeld(cs_main);

    NfTokenProtocolRegTx nftProtoRegTx;
    if (!GetNftTxPayload(tx, nftProtoRegTx)) {
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, "bad-tx-payload");
    }

    NfTokenProtocol protocolExtract = nftProtoRegTx.getTokenProtocol();
    uint64_t protocolId = protocolExtract.getTokenProtocolId();
    std::string protocolName = convertToUpper(protocolExtract.getTokenProtocolName());
    if (nftProtoRegTx.getTokenProtocolVersion() != NfTokenProtocolRegTx::CURRENT_VERSION) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-nft-proto-reg-tx-version");
    }

    if (protocolExtract.getTokenProtocolOwnerId().IsNull()) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-nft-proto-reg-tx-owner-key-null");
    }

    if (protocolExtract.getNftRegSign() < static_cast<uint8_t>(NftRegSignMin) ||
        protocolExtract.getNftRegSign() > static_cast<uint8_t>(NftRegSignMax)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-nft-proto-reg-tx-reg-sign");
    }

    if (protocolExtract.getTokenProtocolName().size() < NfTokenProtocol::TOKEN_PROTOCOL_NAME_MIN ||
        protocolExtract.getTokenProtocolName().size() > NfTokenProtocol::TOKEN_PROTOCOL_NAME_MAX) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-nft-proto-reg-tx-proto-name");
    }

    if (defaultDb.haveProtocolName(protocolName)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-nft-proto-reg-tx-dup-name");
    }

    if (protocolExtract.getTokenMetadataMimeType().size() > NfTokenProtocol::TOKEN_METADATA_MIMETYPE_MAX) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-nft-proto-reg-tx-metadata-mime-type");
    }

    if (protocolExtract.getTokenMetadataSchemaUri().size() > NfTokenProtocol::TOKEN_METADATA_SCHEMA_URI_MAX) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-nft-proto-reg-tx-metadata-schema-uri");
    }

    if (protocolExtract.getMaxMetadataSize() > NfTokenProtocol::TOKEN_METADATA_ABSOLUTE_MAX) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-nft-proto-reg-tx-metadata-max-size-too-big");
    }

    if (defaultDb.haveProtocolId(protocolId)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-nft-proto-reg-tx-dup-protocol");
    }

    return CheckTokenProtocol(protocolExtract, pindexPrev, state);
}

bool CheckTokenProtocol(NfTokenProtocol tokenProtocol, const CBlockIndex* pindexPrev, TxValidationState& state)
{
    return true;
}

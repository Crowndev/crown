// Copyright (c) 2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_NFT_PROTOCOL_H
#define CROWN_NFT_PROTOCOL_H

#include <primitives/transaction.h>
#include <streams.h>
#include <validation.h>
#include <version.h>

enum NftRegSign : uint8_t {
    SelfSign = 1,
    SignByCreator,
    SignPayer,
    NftRegSignMin = SelfSign,
    NftRegSignMax = SignPayer
};

class NfToken {
public:
    SERIALIZE_METHODS(NfToken, obj)
    {
        READWRITE(obj.tokenProtocolId);
        READWRITE(obj.tokenId);
        READWRITE(obj.tokenOwnerKeyId);
        READWRITE(obj.metadataAdminKeyId);
        READWRITE(obj.metadata);
    }

private:
    uint64_t tokenProtocolId { 0 };
    uint256 tokenId {};
    CKeyID tokenOwnerKeyId {};
    CKeyID metadataAdminKeyId {};
    std::vector<unsigned char> metadata;

public:
    uint64_t getTokenProtocolId() { return tokenProtocolId; }
    uint256 getTokenId() { return tokenId; }
    CKeyID getTokenOwnerKeyId() { return tokenOwnerKeyId; }
    CKeyID getMetadataAdminKeyId() { return metadataAdminKeyId; }
    std::vector<unsigned char> getMetadata() { return metadata; }
};

class NfTokenRegTx {
public:
    static const int CURRENT_VERSION = 1;
    SERIALIZE_METHODS(NfTokenRegTx, obj)
    {
        READWRITE(obj.m_version);
        READWRITE(obj.m_tokendata);
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(obj.m_signature);
        }
    };

private:
    uint16_t m_version { CURRENT_VERSION };
    NfToken m_tokendata;
    std::vector<unsigned char> m_signature;

public:
    NfToken getToken()
    {
        return m_tokendata;
    }
    uint16_t getTokenVersion()
    {
        return m_version;
    }
    std::vector<unsigned char> getSignature()
    {
        return m_signature;
    }
};

class NfTokenProtocol {
public:
    static const unsigned TOKEN_PROTOCOL_ID_MIN = 3;
    static const unsigned TOKEN_PROTOCOL_ID_MAX = 12;
    static const unsigned TOKEN_PROTOCOL_NAME_MIN = 3;
    static const unsigned TOKEN_PROTOCOL_NAME_MAX = 24;
    static const unsigned TOKEN_METADATA_SCHEMA_URI_MAX = 128;
    static const unsigned TOKEN_METADATA_MIMETYPE_MAX = 32;
    static const unsigned TOKEN_METADATA_ABSOLUTE_MAX = 255;

    SERIALIZE_METHODS(NfTokenProtocol, obj)
    {
        READWRITE(obj.tokenProtocolId);
        READWRITE(obj.tokenProtocolName);
        READWRITE(obj.tokenMetadataSchemaUri);
        READWRITE(obj.tokenMetadataMimeType);
        READWRITE(obj.isTokenTransferable);
        READWRITE(obj.isMetadataEmbedded);
        READWRITE(obj.nftRegSign);
        READWRITE(obj.maxMetadataSize);
        READWRITE(obj.tokenProtocolOwnerId);
    }

private:
    uint64_t tokenProtocolId { 0 };
    std::string tokenProtocolName;
    std::string tokenMetadataSchemaUri;
    std::string tokenMetadataMimeType { "text/plain" };
    bool isTokenTransferable { true };
    bool isMetadataEmbedded { false };
    uint8_t nftRegSign { static_cast<uint8_t>(SignByCreator) };
    uint8_t maxMetadataSize { TOKEN_METADATA_ABSOLUTE_MAX };
    CKeyID tokenProtocolOwnerId {};

public:
    uint64_t getTokenProtocolId() { return tokenProtocolId; }
    std::string getTokenProtocolName() { return tokenProtocolName; }
    std::string getTokenMetadataSchemaUri() { return tokenMetadataSchemaUri; }
    std::string getTokenMetadataMimeType() { return tokenMetadataMimeType; }
    bool getIsTokenTransferable() { return isTokenTransferable; }
    bool getIsMetadataEmbedded() { return isMetadataEmbedded; }
    uint8_t getNftRegSign() { return nftRegSign; }
    uint8_t getMaxMetadataSize() { return maxMetadataSize; }
    CKeyID getTokenProtocolOwnerId() { return tokenProtocolOwnerId; }
};

class NfTokenProtocolRegTx {
public:
    static const int CURRENT_VERSION = 1;
    SERIALIZE_METHODS(NfTokenProtocolRegTx, obj)
    {
        READWRITE(obj.m_version);
        READWRITE(obj.m_tokendata);
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(obj.m_signature);
        }
    };

private:
    uint16_t m_version { CURRENT_VERSION };
    NfTokenProtocol m_tokendata;
    std::vector<unsigned char> m_signature;

public:
    NfTokenProtocol getTokenProtocol()
    {
        return m_tokendata;
    }
    uint16_t getTokenProtocolVersion()
    {
        return m_version;
    }
    std::vector<unsigned char> getSignature()
    {
        return m_signature;
    }
};

bool ContextualCheckTokenRegister(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state);
bool CheckTokenRegister(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state);
bool ContextualCheckTokenProtocolRegister(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state);
bool CheckTokenProtocolRegister(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state);
bool CheckToken(NfToken token, const CBlockIndex* pindexPrev, TxValidationState& state);
bool CheckTokenProtocol(NfTokenProtocol tokenProtocol, const CBlockIndex* pindexPrev, TxValidationState& state);

#endif //CROWN_NFT_PROTOCOL_H

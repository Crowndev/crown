// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NF_TOKEN_PROTOCOL_INDEX_H
#define CROWN_PLATFORM_NF_TOKEN_PROTOCOL_INDEX_H

#include <memory>
#include "nf-token-protocol.h"
#include "nf-token.h"

class CBlockIndex;

namespace Platform
{
    class NftProtoIndex
    {
    protected:
        const CBlockIndex * m_blockIndex;
        uint256 m_regTxHash;
        std::shared_ptr<NfTokenProtocol> m_nftProto;

    public:
        NftProtoIndex(const CBlockIndex * blockIndex,
                     const uint256 & regTxHash,
                     const std::shared_ptr<NfTokenProtocol> & nftProto)
                : m_blockIndex(blockIndex)
                , m_regTxHash(regTxHash)
                , m_nftProto(nftProto)
        {
        }

        NftProtoIndex()
        {
            SetNull();
        }
        SERIALIZE_METHODS(NftProtoIndex, obj) {
            std::shared_ptr<NfTokenProtocol> m_nftProto;
            READWRITE(obj.m_regTxHash);
            if (ser_action.ForRead())
                SER_READ(obj, obj.m_nftProto = m_nftProto);
            else
                SER_WRITE(obj, m_nftProto = obj.m_nftProto);
        }
        const CBlockIndex * BlockIndex() const { return m_blockIndex; }
        const uint256 & RegTxHash() const { return m_regTxHash; }
        const std::shared_ptr<NfTokenProtocol> & NftProtoPtr() const { return m_nftProto; }

        void SetNull()
        {
            m_blockIndex = nullptr;
            m_regTxHash.SetNull();
            m_nftProto.reset(new NfTokenProtocol());
        }

        bool IsNull()
        {
            return m_blockIndex == nullptr && m_regTxHash.IsNull() && m_nftProto->tokenProtocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL;
        }
    };

    class NftProtoDiskIndex : public NftProtoIndex
    {
    private:
        uint256 m_blockHash;

    public:
        NftProtoDiskIndex(const uint256 & blockHash,
                         const CBlockIndex * blockIndex,
                         const uint256 & regTxHash,
                         const std::shared_ptr<NfTokenProtocol> & nftProto)
                : NftProtoIndex(blockIndex, regTxHash, nftProto)
                , m_blockHash(blockHash)
        {
        }

        NftProtoDiskIndex()
        {
            m_blockHash.SetNull();
            SetNull();
        }

        const uint256 & BlockHash() const { return m_blockHash; }

    public:
        SERIALIZE_METHODS(NftProtoDiskIndex, obj)
        {
            READWRITE(obj.m_blockHash);
            READWRITEAS(NftProtoIndex, obj);
        }
    };
}

#endif //CROWN_PLATFORM_NF_TOKEN_PROTOCOL_INDEX_H

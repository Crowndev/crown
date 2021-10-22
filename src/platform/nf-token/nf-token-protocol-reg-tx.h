// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NF_TOKEN_PROTOCOL_REG_TX_H
#define CROWN_PLATFORM_NF_TOKEN_PROTOCOL_REG_TX_H

#include "key.h"
#include "serialize.h"
#include "nf-token-protocol.h"
#include "univalue.h"

class CTransaction;
class CMutableTransaction;
class CBlockIndex;
class TxValidationState;

namespace Platform
{
    class NfTokenProtocolRegTx //TODO: create common nf token tx interface
    {
    public:
        NfTokenProtocolRegTx() = default;
        explicit NfTokenProtocolRegTx(NfTokenProtocol nfTokenProtocol)
            : m_nfTokenProtocol(std::move(nfTokenProtocol))
        {}

        inline const NfTokenProtocol & GetNftProto() const
        {
            return m_nfTokenProtocol;
        }

        // TODO: encapsulate signing
        // bool Sign(CKey & privKey, CPubKey & pubKey);

        std::string ToString() const;
        void ToJson(UniValue& result) const;

        static bool CheckTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state);
        static bool ProcessTx(const CTransaction & tx, const CBlockIndex * pindex, TxValidationState & state);
        static bool UndoTx(const CTransaction & tx, const CBlockIndex * pindex);

    public:
    SERIALIZE_METHODS(NfTokenProtocolRegTx, obj)
    {
            READWRITE(obj.m_version);
            READWRITE(obj.m_nfTokenProtocol);
            if (!(s.GetType() & SER_GETHASH))
            {
                READWRITE(obj.signature);
            }
        }

    public:
        static const int CURRENT_VERSION = 1;
        std::vector<unsigned char> signature; // TODO: temp public to conform the template signing function

    private:
        uint16_t m_version{CURRENT_VERSION};
        NfTokenProtocol m_nfTokenProtocol;
        // TODO: std::vector<unsigned char> m_signature;
    };
}

#endif // CROWN_PLATFORM_NF_TOKEN_PROTOCOL_REG_TX_H

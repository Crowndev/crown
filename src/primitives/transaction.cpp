// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>

#include <arith_uint256.h>
#include <hash.h>
#include <tinyformat.h>
#include <util/strencodings.h>

#include <assert.h>

std::string GetVersionName(int nVersion)
{
    switch (nVersion) {
       case (LEGACY) :
          return "LEGACY";
       case (SEGWIT) :
          return "SEGWIT";
       case (NFT) :
          return "NFT";
       case (EVO) :
          return "EVO";
    }
    return "UNKNOWN";
};

std::string GetTypeName(int nType)
{
    switch (nType) {
       case (TRANSACTION_NORMAL):
          return "TRANSACTION_NORMAL";
       case (TRANSACTION_NF_TOKEN_REGISTER):
          return "TRANSACTION_NF_TOKEN_REGISTER";
       case (TRANSACTION_NF_TOKEN_PROTOCOL_REGISTER):
          return "TRANSACTION_NF_TOKEN_PROTOCOL_REGISTER";
    }
    return "UNKNOWN";
};

bool IsValidTypeAndVersion(int nVersion, int nType)
{
    if (nVersion == LEGACY)
        return true;
    if (nVersion == SEGWIT)
        return true;
    if (nVersion == NFT) {
        if (nType >= TRANSACTION_NF_TOKEN_REGISTER && nType <= TRANSACTION_NF_TOKEN_PROTOCOL_REGISTER)
            return true;
    }
    //! debug
    return false;
}

std::string COutPoint::ToString() const
{
    return strprintf("COutPoint(%s, %u)", hash.ToString()/*.substr(0,10)*/, n);
}

std::string COutPoint::ToStringShort() const
{
    return strprintf("%s-%u", hash.ToString().substr(0,10), n);
}

uint256 COutPoint::GetHash() const
{
    return SerializeHash(*this);
}

CTxIn::CTxIn(COutPoint prevoutIn, CScript scriptSigIn, uint32_t nSequenceIn)
{
    prevout = prevoutIn;
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

CTxIn::CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn, uint32_t nSequenceIn)
{
    prevout = COutPoint(hashPrevTx, nOut);
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

std::string CTxIn::ToString() const
{
    std::string str;
    str += "CTxIn(";
    str += prevout.ToString();
    if (prevout.IsNull())
        str += strprintf(", coinbase %s", HexStr(scriptSig));
    else
        str += strprintf(", scriptSig=%s", HexStr(scriptSig).substr(0, 24));
    if (nSequence != SEQUENCE_FINAL)
        str += strprintf(", nSequence=%u", nSequence);
    str += ")";
    return str;
}

CTxOut::CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn)
{
    nValue = nValueIn;
    scriptPubKey = scriptPubKeyIn;
}

uint256 CTxOut::GetHash() const
{
    return SerializeHash(*this);
}

std::string CTxOut::ToString() const
{
    return strprintf("CTxOut(nValue=%d.%08d, scriptPubKey=%s)", nValue / COIN, nValue % COIN, HexStr(scriptPubKey).substr(0, 30));
}

CMutableTransaction::CMutableTransaction() : nVersion(CTransaction::CURRENT_VERSION), nType(TRANSACTION_NORMAL), nLockTime(0) {}
CMutableTransaction::CMutableTransaction(const CTransaction& tx) : vin(tx.vin), vout(tx.vout), nVersion(tx.nVersion), nType(tx.nType), nLockTime(tx.nLockTime), extraPayload(tx.extraPayload) {}

uint256 CMutableTransaction::GetHash() const
{
    return SerializeHash(*this, SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
}

uint256 CTransaction::ComputeHash() const
{
    return SerializeHash(*this, SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
}

uint256 CTransaction::ComputeWitnessHash() const
{
    if (!HasWitness()) {
        return hash;
    }
    return SerializeHash(*this, SER_GETHASH, 0);
}

/* For backward compatibility, the hash is initialized to 0. TODO: remove the need for this default constructor entirely. */
CTransaction::CTransaction() : vin(), vout(), nVersion(CTransaction::CURRENT_VERSION), nType(TRANSACTION_NORMAL), nLockTime(0), hash{}, m_witness_hash{} {}
CTransaction::CTransaction(const CMutableTransaction& tx) : vin(tx.vin), vout(tx.vout), nVersion(tx.nVersion), nType(tx.nType), nLockTime(tx.nLockTime), extraPayload(tx.extraPayload), hash{ComputeHash()}, m_witness_hash{ComputeWitnessHash()} {ComputeHash();}
CTransaction::CTransaction(CMutableTransaction&& tx) : vin(std::move(tx.vin)), vout(std::move(tx.vout)), nVersion(tx.nVersion), nType(tx.nType), nLockTime(tx.nLockTime), extraPayload(tx.extraPayload), hash{ComputeHash()}, m_witness_hash{ComputeWitnessHash()} {ComputeHash();}

CAmount CTransaction::GetValueOut() const
{
    CAmount nValueOut = 0;
    for (const auto& tx_out : vout) {
        if (!MoneyRange(tx_out.nValue) || !MoneyRange(nValueOut + tx_out.nValue))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
        nValueOut += tx_out.nValue;
    }
    assert(MoneyRange(nValueOut));
    return nValueOut;
}

unsigned int CTransaction::GetTotalSize() const
{
    return ::GetSerializeSize(*this, PROTOCOL_VERSION);
}

bool CTransaction::IsCoinBase() const
{
    return (vin.size() == 1 && vin[0].prevout.IsNull() && !vin[0].scriptSig.IsProofOfStakeMarker());
}

bool CTransaction::IsCoinStake() const
{
    if (vin.size() != 1 || vout.size() != 1)
        return false;

    if (!vin[0].prevout.IsNull())
        return false;

    return vin[0].scriptSig.IsProofOfStakeMarker();
}

std::string CTransaction::ToString() const
{
    std::string str;
    str += strprintf("CTransaction(hash=%s, ver=%d, type=%d, vin.size=%u, vout.size=%u, nLockTime=%u, extraPayload.size=%d)\n",
        GetHash().ToString().substr(0,10),
        nVersion,
        nType,
        vin.size(),
        vout.size(),
        nLockTime,
        extraPayload.size());
    for (const auto& tx_in : vin)
        str += "    " + tx_in.ToString() + "\n";
    for (const auto& tx_in : vin)
        str += "    " + tx_in.scriptWitness.ToString() + "\n";
    for (const auto& tx_out : vout)
        str += "    " + tx_out.ToString() + "\n";
    return str;
}

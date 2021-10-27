// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PRIMITIVES_TRANSACTION_H
#define CROWN_PRIMITIVES_TRANSACTION_H

#include <stdint.h>
#include <primitives/txout.h>
#include <primitives/txdata.h>
#include <amount.h>
#include <script/script.h>
#include <serialize.h>
#include <uint256.h>

#include <tuple>

#define TX_NFT_VERSION 3
#define TX_ELE_VERSION 4

enum TxVersion : int16_t {
    LEGACY = 1,
    SEGWIT = 2,
    NFT = 3,
    ELE = 4
};

enum TxType : int16_t {
    TRANSACTION_NORMAL = 0,
    TRANSACTION_GOVERNANCE_VOTE = 1000,
    TRANSACTION_NF_TOKEN_REGISTER = 1100,
    TRANSACTION_NF_TOKEN_PROTOCOL_REGISTER = 1200
};

bool IsValidTypeAndVersion(int nVersion, int nType);
std::string GetTypeName(int nType);
std::string GetVersionName(int nVersion);

/**
 * A flag that is ORed into the protocol version to designate that a transaction
 * should be (un)serialized without witness data.
 * Make sure that this does not collide with any of the values in `version.h`
 * or with `ADDRV2_FORMAT`.
 */
static const int SERIALIZE_TRANSACTION_NO_WITNESS = 0x40000000;

/** An outpoint - a combination of a transaction hash and an index n into its vout */
class COutPoint
{
public:
    uint256 hash;
    uint32_t n;

    static constexpr uint32_t NULL_INDEX = std::numeric_limits<uint32_t>::max();

    COutPoint(): n(NULL_INDEX) { }
    COutPoint(const uint256& hashIn, uint32_t nIn): hash(hashIn), n(nIn) { }

    SERIALIZE_METHODS(COutPoint, obj) { READWRITE(obj.hash, obj.n); }

    void SetNull() { hash.SetNull(); n = NULL_INDEX; }
    bool IsNull() const { return (hash.IsNull() && n == NULL_INDEX); }

    friend bool operator<(const COutPoint& a, const COutPoint& b)
    {
        int cmp = a.hash.Compare(b.hash);
        return cmp < 0 || (cmp == 0 && a.n < b.n);
    }

    friend bool operator==(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash == b.hash && a.n == b.n);
    }

    friend bool operator!=(const COutPoint& a, const COutPoint& b)
    {
        return !(a == b);
    }

    uint256 GetHash() const;
    std::string ToString() const;
    std::string ToStringShort() const;
};

/** An input of a transaction.  It contains the location of the previous
 * transaction's output that it claims and a signature that matches the
 * output's public key.
 */
class CTxIn
{
public:
    COutPoint prevout;
    CScript scriptSig;
    uint32_t nSequence;
    CScriptWitness scriptWitness; //!< Only serialized through CTransaction

    /* Setting nSequence to this value for every input in a transaction
     * disables nLockTime. */
    static const uint32_t SEQUENCE_FINAL = 0xffffffff;

    /* Below flags apply in the context of BIP 68*/
    /* If this flag set, CTxIn::nSequence is NOT interpreted as a
     * relative lock-time. */
    static const uint32_t SEQUENCE_LOCKTIME_DISABLE_FLAG = (1U << 31);

    /* If CTxIn::nSequence encodes a relative lock-time and this flag
     * is set, the relative lock-time has units of 512 seconds,
     * otherwise it specifies blocks with a granularity of 1. */
    static const uint32_t SEQUENCE_LOCKTIME_TYPE_FLAG = (1 << 22);

    /* If CTxIn::nSequence encodes a relative lock-time, this mask is
     * applied to extract that lock-time from the sequence field. */
    static const uint32_t SEQUENCE_LOCKTIME_MASK = 0x0000ffff;

    /* In order to use the same number of bits to encode roughly the
     * same wall-clock duration, and because blocks are naturally
     * limited to occur every 600s on average, the minimum granularity
     * for time-based relative lock-time is fixed at 512 seconds.
     * Converting from CTxIn::nSequence to seconds is performed by
     * multiplying by 512 = 2^9, or equivalently shifting up by
     * 9 bits. */
    static const int SEQUENCE_LOCKTIME_GRANULARITY = 9;

    CTxIn()
    {
        nSequence = SEQUENCE_FINAL;
    }

    explicit CTxIn(COutPoint prevoutIn, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);
    CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);

    SERIALIZE_METHODS(CTxIn, obj) { READWRITE(obj.prevout, obj.scriptSig, obj.nSequence); }

    friend bool operator==(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout   == b.prevout &&
                a.scriptSig == b.scriptSig &&
                a.nSequence == b.nSequence);
    }

    friend bool operator!=(const CTxIn& a, const CTxIn& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};

/** An output of a transaction.  It contains the public key that the next input
 * must be able to sign with to claim it.
 */
class CTxOut
{
public:
    CAsset nAsset;
    CAmount nValue;
    CScript scriptPubKey;

    CTxOut()
    {
        SetNull();
    }

    CTxOut(const CAsset& nAssetIn, const CAmount& nValueIn, CScript scriptPubKeyIn);
    CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn);

    SERIALIZE_METHODS(CTxOut, obj) {
        READWRITE(obj.nValue, obj.scriptPubKey);
        READWRITE(obj.nAsset);
    }

    void SetNull()
    {
        nAsset.SetNull();
        nValue=0;
        scriptPubKey.clear();
    }

    bool IsNull() const
    {
        return nAsset.IsNull() && nValue==0 && scriptPubKey.empty();
    }

    void SetEmpty()
    {
        nValue = 0;
        nAsset.SetNull();
        scriptPubKey.clear();
    }

    bool IsFee() const {
        return scriptPubKey == CScript() && !nValue==0 && !nAsset.IsNull();
    }

    bool IsEmpty() const
    {
        return (nValue == 0 && scriptPubKey.empty());
    }

    CAmount GetValue() const
    {
        return nValue;
    }

    uint256 GetHash() const;

    friend bool operator==(const CTxOut& a, const CTxOut& b)
    {
        return (a.nValue       == b.nValue &&
                a.scriptPubKey == b.scriptPubKey);
    }

    friend bool operator!=(const CTxOut& a, const CTxOut& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};

struct CMutableTransaction;

/**
 * Basic transaction serialization format:
 * - int32_t nVersion
 * - std::vector<CTxIn> vin
 * - std::vector<CTxOut> vout
 * - uint32_t nLockTime
 *
 * Extended transaction serialization format:
 * - int32_t nVersion
 * - unsigned char dummy = 0x00
 * - unsigned char flags (!= 0)
 * - std::vector<CTxIn> vin
 * - std::vector<CTxOut> vout
 * - if (flags & 1):
 *   - CTxWitness wit;
 * - uint32_t nLockTime
 */
template<typename Stream, typename TxType>
inline void UnserializeTransaction(TxType& tx, Stream& s) {
    const bool fAllowWitness = !(s.GetVersion() & SERIALIZE_TRANSACTION_NO_WITNESS);

    int32_t n32bitVersion = tx.nVersion | (tx.nType << 16);
    s >> n32bitVersion;

    tx.nVersion = (int16_t)(n32bitVersion & 0xffff);
    tx.nType = (int16_t)((n32bitVersion >> 16) & 0xffff);

    unsigned char flags = 0;
    tx.vin.clear();
    tx.vout.clear();
    tx.vpout.clear();
    tx.vdata.clear();
    /* Try to read the vin. In case the dummy is there, this will be read as an empty vector. */
    s >> tx.vin;
    if (tx.vin.size() == 0 && fAllowWitness) {
        /* We read a dummy or an empty vin. */
        s >> flags;
        if (flags != 0) {
            s >> tx.vin;

            if (tx.nVersion >= TX_ELE_VERSION) {
                size_t nOutputs = ReadCompactSize(s);
                tx.vpout.clear();
                tx.vpout.reserve(nOutputs);
                for (size_t k = 0; k < nOutputs; ++k) {
                    uint8_t bv;
                    s >> bv;
                    switch (bv) {
                        case OUT_STANDARD:
                            tx.vpout.push_back(MAKE_OUTPUT<CTxOutStandard>());
                            break;
                        case OUT_DATA:
                            tx.vpout.push_back(MAKE_OUTPUT<CTxOutData>());
                            break;
                        default:
                            throw std::ios_base::failure("Unknown transaction output type");
                    }
                    tx.vpout[k]->nVersion = bv;
                    s >> *tx.vpout[k];
                }
            }
            else
                s >> tx.vout;
        }
    } else {
        /* We read a non-empty vin. Assume a normal vout follows. */
        if (tx.nVersion >= TX_ELE_VERSION) {
            size_t nOutputs = ReadCompactSize(s);
            tx.vpout.clear();
            tx.vpout.reserve(nOutputs);
            for (size_t k = 0; k < nOutputs; ++k) {
                uint8_t bv;
                s >> bv;
                switch (bv) {
                    case OUT_STANDARD:
                        tx.vpout.push_back(MAKE_OUTPUT<CTxOutStandard>());
                        break;
                    case OUT_DATA:
                        tx.vpout.push_back(MAKE_OUTPUT<CTxOutData>());
                        break;
                    default:
                        throw std::ios_base::failure("Unknown transaction output type");
                }
                tx.vpout[k]->nVersion = bv;
                s >> *tx.vpout[k];
            }
        }
        else
            s >> tx.vout;
    }

    if ((flags & 1) && fAllowWitness) {
        /* The witness flag is present, and we support witnesses. */
        flags ^= 1;
        for (size_t i = 0; i < tx.vin.size(); i++) {
            s >> tx.vin[i].scriptWitness.stack;
        }
        if (!tx.HasWitness()) {
            /* It's illegal to encode witnesses when all witness stacks are empty. */
            throw std::ios_base::failure("Superfluous witness record");
        }
    }
    if (flags) {
        /* Unknown flag in the serialization */
        throw std::ios_base::failure("Unknown transaction optional data");
    }
    s >> tx.nLockTime;
    if (tx.nVersion >= TX_ELE_VERSION) {
        size_t nOutputs = ReadCompactSize(s);
        tx.vdata.reserve(nOutputs);
        for (size_t k = 0; k < nOutputs; ++k) {
            uint8_t bv;
            s >> bv;
            switch (bv) {
                case OUTPUT_DATA:
                    tx.vdata.push_back(MAKE_OUTPUT<CTxData>());
                    break;
                case OUTPUT_CONTRACT:
                    tx.vdata.push_back(MAKE_OUTPUT<CContract>());
                    break;
                case OUTPUT_ID:
                    tx.vdata.push_back(MAKE_OUTPUT<CChainID>());
                    break;
                default:
                    throw std::ios_base::failure("Unknown transaction output type");
            }
            tx.vdata[k]->nVersion = bv;
            s >> *tx.vdata[k];
        }
    } else if (tx.nVersion >= 3 && tx.nType != TRANSACTION_NORMAL) {
        s >> tx.extraPayload;
    }
}

template<typename Stream, typename TxType>
inline void SerializeTransaction(const TxType& tx, Stream& s) {
    const bool fAllowWitness = !(s.GetVersion() & SERIALIZE_TRANSACTION_NO_WITNESS);

    int32_t n32bitVersion = tx.nVersion | (tx.nType << 16);
    s << n32bitVersion;

    unsigned char flags = 0;
    // Consistency check
    if (fAllowWitness) {
        /* Check whether witnesses need to be serialized. */
        if (tx.HasWitness()) {
            flags |= 1;
        }
    }
    if (flags) {
        /* Use extended format in case witnesses are to be serialized. */
        std::vector<CTxIn> vinDummy;
        s << vinDummy;
        s << flags;
    }
    s << tx.vin;

    if (tx.nVersion >= TX_ELE_VERSION) {
        WriteCompactSize(s, tx.vpout.size());
        for (size_t k = 0; k < tx.vpout.size(); ++k) {
            s << tx.vpout[k]->nVersion;
            s << *tx.vpout[k];
        }
    }
    else
        s << tx.vout;

    if (flags & 1) {
        for (size_t i = 0; i < tx.vin.size(); i++) {
            s << tx.vin[i].scriptWitness.stack;
        }
    }
    s << tx.nLockTime;

    if (tx.nVersion >= TX_ELE_VERSION) {
        WriteCompactSize(s, tx.vdata.size());
        for (size_t k = 0; k < tx.vdata.size(); ++k) {
            s << tx.vdata[k]->nVersion;
            s << *tx.vdata[k];
        }
    } else if (tx.nVersion >= 3 && tx.nType != TRANSACTION_NORMAL) {
        s << tx.extraPayload;
    }
}


/** The basic transaction that is broadcasted on the network and contained in
 * blocks.  A transaction can contain multiple inputs and outputs.
 */
class CTransaction
{
public:
    // Default transaction version.
    static const int32_t CURRENT_VERSION=2;
    static const int32_t MAX_STANDARD_VERSION=2;

    // The local variables are made const to prevent unintended modification
    // without updating the cached hash value. However, CTransaction is not
    // actually immutable; deserialization and assignment are implemented,
    // and bypass the constness. This is safe, as they update the entire
    // structure, including the hash.
    const std::vector<CTxIn> vin;
    const std::vector<CTxOut> vout;
    const std::vector<CTxOutBaseRef> vpout;
    const std::vector<CTxDataBaseRef> vdata;
    const int16_t nVersion;
    const int16_t nType;
    const uint32_t nLockTime;
    const std::vector<uint8_t> extraPayload;

private:
    /** Memory only. */
    const uint256 hash;
    const uint256 m_witness_hash;

    uint256 ComputeHash() const;
    uint256 ComputeWitnessHash() const;

public:
    /** Construct a CTransaction that qualifies as IsNull() */
    CTransaction();

    /** Convert a CMutableTransaction into a CTransaction. */
    explicit CTransaction(const CMutableTransaction& tx);
    CTransaction(CMutableTransaction&& tx);

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        SerializeTransaction(*this, s);
    }

    /** This deserializing constructor is provided instead of an Unserialize method.
     *  Unserialize is not possible, since it would require overwriting const fields. */
    template <typename Stream>
    CTransaction(deserialize_type, Stream& s) : CTransaction(CMutableTransaction(deserialize, s)) {ComputeHash();}

    bool IsNull() const {
        return vin.empty() && vout.empty() && vpout.empty();
    }

    size_t GetNumVOuts() const
    {
        return nVersion >= TX_ELE_VERSION ? vpout.size() : vout.size();
    }

    const uint256& GetHash() const { return hash; }
    const uint256& GetWitnessHash() const { return m_witness_hash; };

    // Return sum of txouts.
    CAmount GetValueOut() const;
    CAmountMap GetValueOutMap() const;

    /**
     * Get the total transaction size in bytes, including witness data.
     * "Total Size" defined in BIP141 and BIP144.
     * @return Total transaction size in bytes
     */
    unsigned int GetTotalSize() const;

    bool IsCoinBase() const;
    bool IsCoinStake() const;

    friend bool operator==(const CTransaction& a, const CTransaction& b)
    {
        return a.hash == b.hash;
    }

    friend bool operator!=(const CTransaction& a, const CTransaction& b)
    {
        return a.hash != b.hash;
    }

    std::string ToString() const;

    bool HasWitness() const
    {
        for (size_t i = 0; i < vin.size(); i++) {
            if (!vin[i].scriptWitness.IsNull()) {
                return true;
            }
        }
        return false;
    }
};

/** A mutable version of CTransaction. */
struct CMutableTransaction
{
    std::vector<CTxIn> vin;
    std::vector<CTxOut> vout;
    std::vector<CTxOutBaseRef> vpout;
    std::vector<CTxDataBaseRef> vdata;
    int16_t nVersion;
    int16_t nType;
    uint32_t nLockTime;
    std::vector<uint8_t> extraPayload;

    CMutableTransaction();
    explicit CMutableTransaction(const CTransaction& tx);

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        SerializeTransaction(*this, s);
    }

    template <typename Stream>
    inline void Unserialize(Stream& s) {
        UnserializeTransaction(*this, s);
    }

    template <typename Stream>
    CMutableTransaction(deserialize_type, Stream& s) {
        Unserialize(s);
    }

    size_t GetNumVOuts() const
    {
        return nVersion >= TX_ELE_VERSION ? vpout.size() : vout.size();
    }

    /** Compute the hash of this CMutableTransaction. This is computed on the
     * fly, as opposed to GetHash() in CTransaction, which uses a cached result.
     */
    uint256 GetHash() const;

    std::string ToString() const;

    bool HasWitness() const
    {
        for (size_t i = 0; i < vin.size(); i++) {
            if (!vin[i].scriptWitness.IsNull()) {
                return true;
            }
        }
        return false;
    }
};

typedef std::shared_ptr<const CTransaction> CTransactionRef;
static inline CTransactionRef MakeTransactionRef() { return std::make_shared<const CTransaction>(); }
template <typename Tx> static inline CTransactionRef MakeTransactionRef(Tx&& txIn) { return std::make_shared<const CTransaction>(std::forward<Tx>(txIn)); }

/** A generic txid reference (txid or wtxid). */
class GenTxid
{
    bool m_is_wtxid;
    uint256 m_hash;
    uint32_t m_type;
public:
    GenTxid(bool is_wtxid, const uint256& hash, const uint32_t& type) : m_is_wtxid(is_wtxid), m_hash(hash), m_type(type) {}
    GenTxid(bool is_wtxid, const uint256& hash) : m_is_wtxid(is_wtxid), m_hash(hash), m_type(0) {}
    bool IsWtxid() const { return m_is_wtxid; }
    const uint256& GetHash() const { return m_hash; }
    const uint32_t& GetType() const { return m_type; }
    friend bool operator==(const GenTxid& a, const GenTxid& b) { return a.m_is_wtxid == b.m_is_wtxid && a.m_hash == b.m_hash; }
    friend bool operator<(const GenTxid& a, const GenTxid& b) { return std::tie(a.m_is_wtxid, a.m_hash) < std::tie(b.m_is_wtxid, b.m_hash); }
};

CAmountMap GetFeeMap(const CTransaction& tx);
// Check if explicit TX fees overflow or are negative
bool HasValidFee(const CTransaction& tx);


#endif // CROWN_PRIMITIVES_TRANSACTION_H

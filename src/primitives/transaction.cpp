// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>

#include <arith_uint256.h>
#include <hash.h>
#include <tinyformat.h>
#include <logging.h>
#include <util/strencodings.h>
#include <util/time.h>
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
       case (ELE) :
          return "ELE";
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

bool ExtractCoinStakeInt64(const std::vector<uint8_t> &vData, DataOutputTypes get_type, CAmount &out)
{
    if (vData.size() < 5) { // First 4 bytes will be height
        return false;
    }
    uint64_t nv = 0;
    size_t nb = 0;
    size_t ofs = 4;
    while (ofs < vData.size()) {
        uint8_t current_type = vData[ofs];
        if (current_type == DataOutputTypes::DO_SMSG_DIFFICULTY) {
            ofs += 5;
        } else
        if (current_type == DataOutputTypes::DO_SMSG_FEE) {
            ofs++;
            if  (0 != part::GetVarInt(vData, ofs, nv, nb)) {
                return false;
            }
            if (get_type == current_type) {
                out = nv;
                return true;
            }
            ofs += nb;
        } else {
            break; // Unknown identifier byte
        }
    }
    return false;
}

bool ExtractCoinStakeUint32(const std::vector<uint8_t> &vData, DataOutputTypes get_type, uint32_t &out)
{
    if (vData.size() < 5) { // First 4 bytes will be height
        return false;
    }
    uint64_t nv = 0;
    size_t nb = 0;
    size_t ofs = 4;
    while (ofs < vData.size()) {
        uint8_t current_type = vData[ofs];
        if (current_type == DataOutputTypes::DO_SMSG_DIFFICULTY) {
            if (vData.size() < ofs+5) {
                return false;
            }
            if (get_type == current_type) {
                memcpy(&out, &vData[ofs + 1], 4);
                out = le32toh(out);
                return true;
            }
            ofs += 5;
        } else
        if (current_type == DataOutputTypes::DO_SMSG_FEE) {
            ofs++;
            if  (0 != part::GetVarInt(vData, ofs, nv, nb)) {
                return false;
            }
            ofs += nb;
        } else {
            break; // Unknown identifier byte
        }
    }
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

CTxOutAsset::CTxOutAsset(const CAsset& nAssetIn, const CAmount& nValueIn, CScript scriptPubKeyIn)
{
    nAsset = nAssetIn;
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

std::string CTxOutAsset::ToString() const
{
    return strprintf("CTxOutAsset \n%s \n(nValue=%s, scriptPubKey=%s)\n", nAsset.ToString(false), strprintf("%d.%08d", nValue / COIN, nValue % COIN), scriptPubKey.ToString());
}

std::string CTxDataBase::ToString() const
{
    switch (nVersion) {
        case OUTPUT_DATA:
            {
            CTxData *data_output = (CTxData*)this;
            return strprintf("%s", data_output->ToString());
            }
        default:
            break;
    }
    return strprintf("CTxDataBase unknown version %d", nVersion);
}

uint256 CTxDataBase::GetHash() const
{
    switch (nVersion) {
        default:
            break;
    }
    return uint256();
}

uint256 CTxDataBase::GetHashWithoutSign() const
{
    switch (nVersion) {
        default:
            break;
    }
    return uint256();
}

std::string CTxData::ToString() const
{
    return strprintf("CTxData(%s)\n", HexStr(vData));
}

void DeepCopy(CTxDataBaseRef &to, const CTxDataBaseRef &from)
{
    switch (from->GetVersion()) {
        case OUTPUT_DATA:
            to = MAKE_OUTPUT<CTxData>();
            *((CTxData*)to.get()) = *((CTxData*)from.get());
            break;
        default:
            break;
    }
    return;
}

std::vector<CTxDataBaseRef> DeepCopy(const std::vector<CTxDataBaseRef> &from)
{
    std::vector<CTxDataBaseRef> vdata;
    vdata.resize(from.size());
    for (size_t i = 0; i < from.size(); ++i) {
        DeepCopy(vdata[i], from[i]);
    }

    return vdata;
}

CMutableTransaction::CMutableTransaction() : nTime(GetTime()), nVersion(CTransaction::CURRENT_VERSION), nType(TRANSACTION_NORMAL), nLockTime(0) {}
CMutableTransaction::CMutableTransaction(const CTransaction& tx) : nTime(tx.nTime), vin(tx.vin), vout(tx.vout), vpout(tx.vpout), vdata{DeepCopy(tx.vdata)}, nVersion(tx.nVersion), nType(tx.nType), nLockTime(tx.nLockTime), extraPayload(tx.extraPayload), witness(tx.witness) {}

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

uint256 CTransaction::GetWitnessOnlyHash() const
{
    std::vector<uint256> leaves;
    leaves.reserve(std::max(vin.size(), vout.size()));
    /* Inputs */
    for (size_t i = 0; i < vin.size(); ++i) {
        // Input has no witness OR is null input(coinbase)
        const CTxInWitness& txinwit = (witness.vtxinwit.size() <= i || vin[i].prevout.IsNull()) ? CTxInWitness() : witness.vtxinwit[i];
        leaves.push_back(txinwit.GetHash());
    }
    uint256 hashIn = ComputeFastMerkleRoot(leaves);
    leaves.clear();
    leaves.push_back(hashIn);
    return ComputeFastMerkleRoot(leaves);
}

/* For backward compatibility, the hash is initialized to 0. TODO: remove the need for this default constructor entirely. */
CTransaction::CTransaction() :  nTime(0), vin(), vout(), nVersion(CTransaction::CURRENT_VERSION), nType(TRANSACTION_NORMAL), nLockTime(0), hash{}, m_witness_hash{} {}
CTransaction::CTransaction(const CMutableTransaction& tx) : nTime(tx.nTime), vin(tx.vin), vout(tx.vout), vpout(tx.vpout), vdata{DeepCopy(tx.vdata)}, nVersion(tx.nVersion), nType(tx.nType), nLockTime(tx.nLockTime), extraPayload(tx.extraPayload), witness(tx.witness), hash{ComputeHash()}, m_witness_hash{ComputeWitnessHash()} {ComputeHash();}
CTransaction::CTransaction(CMutableTransaction&& tx) : nTime(tx.nTime), vin(std::move(tx.vin)), vout(std::move(tx.vout)), vpout(std::move(tx.vpout)), vdata(std::move(tx.vdata)), nVersion(tx.nVersion), nType(tx.nType), nLockTime(tx.nLockTime), extraPayload(tx.extraPayload), witness(std::move(tx.witness)), hash{ComputeHash()}, m_witness_hash{ComputeWitnessHash()} {ComputeHash();}

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

CAmountMap CTransaction::GetValueOutMap() const {

    CAmountMap values;
    for (const auto& tx_out : vpout) {
        values[tx_out.nAsset] += tx_out.nValue;
        if (!MoneyRange(tx_out.nValue) || !MoneyRange(values[tx_out.nAsset]))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }
    return values;
}

CAmount CTransaction::GetTotalSMSGFees() const
{
    CAmount smsg_fees = 0;
    for (const auto &v : vdata) {
        if (!v->IsVersion(OUTPUT_DATA))
            continue;
        CTxData *txd = (CTxData*) v.get();
        if (txd->vData.size() < 25 || txd->vData[0] != DO_FUND_MSG)
            continue;
        size_t n = (txd->vData.size()-1) / 24;
        for (size_t k = 0; k < n; ++k) {
            uint32_t nAmount;
            memcpy(&nAmount, &txd->vData[1+k*24+20], 4);
            nAmount = le32toh(nAmount);
            smsg_fees += nAmount;
        }
    }
    return smsg_fees;
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
    if (vin.size() != 1 || (nVersion >= TX_ELE_VERSION ? vpout.size() != 1 : vout.size() != 1))
        return false;

    if (!vin[0].prevout.IsNull())
        return false;

    return vin[0].scriptSig.IsProofOfStakeMarker();
}

std::string CTransaction::ToString() const
{
    std::string str;
    str += strprintf("CTransaction(hash=%s, ver=%d, type=%d, nTime=%d, vin.size=%u, vout.size=%u, nLockTime=%u, extraPayload.size=%d)\n",
        GetHash().ToString().substr(0,10),
        nVersion,
        nType,
        nTime,
        vin.size(),
        (nVersion >= TX_ELE_VERSION ? vpout.size() : vout.size()),
        nLockTime,
        (nVersion >= TX_ELE_VERSION ? 0 : extraPayload.size()));
    for (const auto& tx_in : vin)
        str +=tx_in.ToString() + "\n";
    for (const auto& tx_in : witness.vtxinwit)
        str += tx_in.scriptWitness.ToString() + "\n";
    for (const auto& tx_out : vout)
        str += tx_out.ToString() + "\n";
    for (unsigned int i = 0; i < vpout.size(); i++)
        str += "    " + vpout[i].ToString() + "\n";
    for (unsigned int i = 0; i < vdata.size(); i++)
        str += vdata[i].get()->ToString() + "\n";
    return str;
}

bool CTransaction::GetSmsgFeeRate(CAmount &fee_rate) const
{
    if (vdata.size() > 0){
        uint8_t vers = vdata[0].get()->GetVersion();
        if (vers != OUTPUT_DATA) 
            return false;
    
        return vdata[0].get()->GetSmsgFeeRate(fee_rate);
    }
    return false;
}

bool CTransaction::GetSmsgDifficulty(uint32_t &compact) const
{
    if (vdata.size() > 0){
		uint8_t vers = vdata[0].get()->GetVersion();
		if (vers != OUTPUT_DATA)
			return false;
		
		return vdata[0].get()->GetSmsgDifficulty(compact);
    }
    return false;
}

std::string CMutableTransaction::ToString() const
{
    std::string str;
    str += strprintf("CTransaction(hash=%s, ver=%d, type=%d, nTime=%d, vin.size=%u, vout.size=%u, nLockTime=%u, vExtraPayload.size=%d)\n",
        GetHash().ToString().substr(0,10),
        nVersion,
        nType,
        nTime,
        vin.size(),
        (nVersion >= TX_ELE_VERSION ? vpout.size() : vout.size()),
        nLockTime,
        (nVersion >= TX_ELE_VERSION ? 0 : extraPayload.size()));
    for (const auto& tx_in : vin)
        str += tx_in.ToString() + "\n";
    for (const auto& tx_in : witness.vtxinwit)
        str += tx_in.scriptWitness.ToString() + "\n";
    for (const auto& tx_out : vout)
        str += tx_out.ToString() + "\n";
    for (unsigned int i = 0; i < vpout.size(); i++)
        str += "    " + vpout[i].ToString() + "\n";
    for (unsigned int i = 0; i < vdata.size(); i++)
        str += vdata[i].get()->ToString() + "\n";
    return str;
}

std::string dataTypeToString(DataTypes &dt)
{
    switch (dt) {
        case OUTPUT_DATA:
            return "DATA";
        default:
            break;
    }
    return "";
}

bool HasValidFee(const CTransaction& tx) {
    CAmountMap totalFee;

    for (unsigned int k = 0; k < (tx.nVersion >= TX_ELE_VERSION ? tx.vpout.size() : tx.vout.size()) ; k++){
        const CTxOutAsset &txout = (tx.nVersion >= TX_ELE_VERSION ? tx.vpout[k] : tx.vout[k]);
        CAmount fee = 0;
        if (txout.IsFee()) {
            fee = txout.nValue;
            if (fee == 0 || !MoneyRange(fee))
                return false;
            totalFee[txout.nAsset] += fee;
            if (!MoneyRange(totalFee)) {
                return false;
            }
        }
    }
    return true;
}

CAmountMap GetFeeMap(const CTransaction& tx) {
    CAmountMap fee;
    if(tx.nVersion >= TX_ELE_VERSION){
        for (unsigned int k = 0; k < tx.vpout.size(); k++)
            if (tx.vpout[k].IsFee())
                fee[tx.vpout[k].nAsset] += tx.vpout[k].nValue;
    }
    return fee;
}

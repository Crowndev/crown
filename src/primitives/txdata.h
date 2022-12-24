#ifndef CROWN_PRIMITIVES_TXDATA_H
#define CROWN_PRIMITIVES_TXDATA_H

#include <script/script.h>

#include <serialize.h>
#include <uint256.h>
#include <pubkey.h>
#include <amount.h>

class CTxData;

enum DataTypes
{
    OUTPUT_NULL             = 0, // Marker for CCoinsView
    OUTPUT_DATA             = 1,
    OUTPUT_CONTRACT         = 2,
    OUTPUT_ID               = 3,
    OUTPUT_VOTE             = 4,
};

enum DataOutputTypes
{
    DO_NULL                 = 0, // Reserved
    DO_FUND_MSG             = 1,
    DO_SMSG_FEE             = 2,
    DO_SMSG_DIFFICULTY      = 3,
};

bool ExtractCoinStakeInt64(const std::vector<uint8_t> &vData, DataOutputTypes get_type, CAmount &out);
bool ExtractCoinStakeUint32(const std::vector<uint8_t> &vData, DataOutputTypes get_type, uint32_t &out);

std::string dataTypeToString(DataTypes &dt);

class CTxDataBase
{
public:
    explicit CTxDataBase(uint8_t v) : nVersion(v) {};
    explicit CTxDataBase() {};
    virtual ~CTxDataBase() {};
    uint8_t nVersion;

    SERIALIZE_METHODS(CTxDataBase, obj)
    {
        READWRITE(obj.nVersion);
        switch (obj.nVersion) {
            case OUTPUT_DATA:
                READWRITE(*(CTxData*) &obj);
                break;
            //default:
            //    assert(false);
        }
    }

    uint8_t GetVersion() const
    {
        return nVersion;
    }

    bool IsVersion(uint8_t _nVersion) const
    {
        return nVersion == _nVersion;
    }

    virtual std::vector<uint8_t> *GetPData() { return nullptr; };
    virtual const std::vector<uint8_t> *GetPData() const { return nullptr; };

    virtual bool GetSmsgFeeRate(CAmount &nCfwd) const { return false; };
    virtual bool GetSmsgDifficulty(uint32_t &compact) const { return false; };

    virtual void SetEmpty() {};

    virtual bool IsEmpty() const { return false; };

    uint256 GetHashWithoutSign() const;

    uint256 GetHash() const;

    std::string ToString() const;
};

#define OUTPUT_PTR std::shared_ptr
typedef OUTPUT_PTR<CTxDataBase> CTxDataBaseRef;
#define MAKE_OUTPUT std::make_shared

class CTxData : public CTxDataBase
{
public:
    CTxData() : CTxDataBase(OUTPUT_DATA) {};
    explicit CTxData(const std::vector<uint8_t> &vData_) : CTxDataBase(OUTPUT_DATA), vData(vData_) {};

    uint8_t nType;
    std::vector<uint8_t> vData;

    SERIALIZE_METHODS(CTxData, obj) { READWRITE(obj.nType, obj.vData); }

    bool GetSmsgFeeRate(CAmount &fee_rate) const override
    {
        return ExtractCoinStakeInt64(vData, DO_SMSG_FEE, fee_rate);
    }

    bool GetSmsgDifficulty(uint32_t &compact) const override
    {
        return ExtractCoinStakeUint32(vData, DO_SMSG_DIFFICULTY, compact);
    }

    std::vector<uint8_t> *GetPData() override
    {
        return &vData;
    }
    const std::vector<uint8_t> *GetPData() const override
    {
        return &vData;
    }

    uint8_t GetType() const
    {
        return nType;
    }

    bool IsType(uint8_t _nType) const
    {
        return nType == _nType;
    }

    void SetEmpty() override
    {
        vData.clear();
    }

    bool IsEmpty() const override
    {
        return vData.empty();
    }

    uint256 GetHash() const;

    std::string ToString() const;
};

#endif

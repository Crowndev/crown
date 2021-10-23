#ifndef CROWN_PRIMITIVES_TXOUT_H
#define CROWN_PRIMITIVES_TXOUT_H

#include <primitives/asset.h>

class CTxOutBasic;
class CTxOutStandard;
class CTxOutData;

enum OutputTypes
{
    OUT_BASIC            = 0,
    OUT_STANDARD         = 1,
    OUT_DATA             = 2,
};

class CTxOutBase
{
public:
    explicit CTxOutBase(uint8_t v) : nVersion(v) {};
    virtual ~CTxOutBase() {};
    uint8_t nVersion;

    SERIALIZE_METHODS(CTxOutBase, obj)
    {
        READWRITE(obj.nVersion);
        switch (obj.nVersion) {
            case OUT_STANDARD:
                READWRITE(*(CTxOutStandard*) &obj);
                break;
            case OUT_DATA:
                READWRITE(*(CTxOutData*) &obj);
                break;
            default:
                assert(false);
        }
    }

    uint8_t GetType() const
    {
        return nVersion;
    }

    bool IsType(uint8_t nType) const
    {
        return nVersion == nType;
    }

    bool IsStandardOutput() const
    {
        return nVersion == OUT_STANDARD;
    }

    const CTxOutStandard *GetStandardOutput() const
    {
        assert(nVersion == OUT_STANDARD);
        return (CTxOutStandard*)this;
    }

    virtual bool IsEmpty() const { return false;}

    void SetValue(CAmount value);

    virtual CAmount GetValue() const;

    virtual bool PutValue(std::vector<uint8_t> &vchAmount) const { return false; };

    virtual bool GetScriptPubKey(CScript &scriptPubKey_) const { return false; };
    virtual const CScript *GetPScriptPubKey() const { return nullptr; };

    virtual std::vector<uint8_t> *GetPRangeproof() { return nullptr; };
    virtual std::vector<uint8_t> *GetPData() { return nullptr; };
    virtual const std::vector<uint8_t> *GetPRangeproof() const { return nullptr; };
    virtual const std::vector<uint8_t> *GetPData() const { return nullptr; };

    std::string ToString() const;
};

#define OUTPUT_PTR std::shared_ptr
typedef OUTPUT_PTR<CTxOutBase> CTxOutBaseRef;
#define MAKE_OUTPUT std::make_shared

class CTxOutBasic : public CTxOutBase
{
public:
    CTxOutBasic() : CTxOutBase(OUT_STANDARD) {};
    CTxOutBasic(const CAmount& nValueIn, CScript scriptPubKeyIn);

    CAmount nValue;
    CScript scriptPubKey;

    SERIALIZE_METHODS(CTxOutBasic, obj) { READWRITE(obj.nValue, obj.scriptPubKey); }

    void SetNull()
    {
        nValue=0;
        scriptPubKey.clear();
    }

    bool IsNull() const
    {
        return nValue==0 && scriptPubKey.empty();
    }

    void SetEmpty()
    {
        nValue = 0;
        scriptPubKey.clear();
    }

    bool IsFee() const {
        return scriptPubKey == CScript() && !nValue==0;
    }

    bool IsEmpty() const override
    {
        return (nValue == 0 && scriptPubKey.empty());
    }

    friend bool operator==(const CTxOutBasic& a, const CTxOutBasic& b)
    {
        return (a.nValue == b.nValue &&
                a.scriptPubKey == b.scriptPubKey);
    }

    friend bool operator!=(const CTxOutBasic& a, const CTxOutBasic& b)
    {
        return !(a == b);
    }

    uint256 GetHash() const;

    std::string ToString() const;

    CAmount GetValue() const override
    {
        return nValue;
    }

    bool GetScriptPubKey(CScript &scriptPubKey_) const override
    {
        scriptPubKey_ = scriptPubKey;
        return true;
    }

    virtual const CScript *GetPScriptPubKey() const override
    {
        return &scriptPubKey;
    }
};

class CTxOutStandard : public CTxOutBase
{
public:
    CTxOutStandard() : CTxOutBase(OUT_STANDARD) {};
    CTxOutStandard(const CAsset& nAssetIn, const CAmount& nValueIn, CScript scriptPubKeyIn);

    CAsset nAsset;
    CAmount nValue;
    CScript scriptPubKey;

    SERIALIZE_METHODS(CTxOutStandard, obj) { READWRITE(obj.nAsset, obj.nValue, obj.scriptPubKey); }

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

    bool IsEmpty() const override
    {
        return (nValue == 0 && scriptPubKey.empty());
    }

    friend bool operator==(const CTxOutStandard& a, const CTxOutStandard& b)
    {
        return (a.nAsset == b.nAsset &&
                a.nValue == b.nValue &&
                a.scriptPubKey == b.scriptPubKey);
    }

    friend bool operator!=(const CTxOutStandard& a, const CTxOutStandard& b)
    {
        return !(a == b);
    }

    uint256 GetHash() const;

    std::string ToString() const;

    CAmount GetValue() const override
    {
        return nValue;
    }

    bool GetScriptPubKey(CScript &scriptPubKey_) const override
    {
        scriptPubKey_ = scriptPubKey;
        return true;
    }

    virtual const CScript *GetPScriptPubKey() const override
    {
        return &scriptPubKey;
    }
};

class CTxOutData : public CTxOutBase
{
public:
    CTxOutData() : CTxOutBase(OUT_DATA) {};
    explicit CTxOutData(const std::vector<uint8_t> &vData_) : CTxOutBase(OUT_DATA), vData(vData_) {};

    std::vector<uint8_t> vData;

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s << vData;
    }

    template<typename Stream>
    void Unserialize(Stream &s)
    {
        s >> vData;
    }

    std::vector<uint8_t> *GetPData() override
    {
        return &vData;
    }
    const std::vector<uint8_t> *GetPData() const override
    {
        return &vData;
    }
};


#endif

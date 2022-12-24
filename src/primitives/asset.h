
#ifndef CROWN_ASSET_H
#define CROWN_ASSET_H

#include <uint256.h>

#include <amount.h>
#include <script/script.h>
#include <tinyformat.h>
/**
 *  Native Asset Issuance
 *
 *  An asset identifier tag, a 256 bits serialized hash (sha256) defined
 *  by the issuance transaction from which the output’s coins are derived.
 *  Each output contains coins from a single asset/currency.
 *  For the host currency, the similarly-calculated hash of the chain’s genesis
 *  block is used instead.
**/
typedef uint256 CAssetID;

class AssetMetadata
{

public:
    static const uint32_t CURRENT_VERSION = 1;

    uint32_t nVersion;
    uint32_t nFlags;
    uint32_t nType;
    uint32_t nExpiry;
    unsigned char sAssetName[11] = {};
    unsigned char sAssetShortName[5] = {};
    std::string contract_url; //online human readable contract
    std::string sIssuingaddress;
    CScript scriptcode;

    /** Asset flags */
    enum AssetFlags : uint64_t {
        // Nothing
        ASSET_NONE = 0,
        // ASSET_TRANSFERABLE means that the asset can be transfered to other addresses after initial creation
        ASSET_TRANSFERABLE = (1 << 0),
        // ASSET_CONVERTABLE means the asset can be converted to another asset.
        ASSET_CONVERTABLE = (1 << 1),
        // ASSET_LIMITED means other assets cannot be converted to this one
        ASSET_LIMITED = (1 << 2),
        // ASSET_RESTRICTED means the assets can only be issued by one address
        ASSET_RESTRICTED = (1 << 3),
        // ASSET_STAKEABLE means the asset can be staked
        ASSET_STAKEABLE = (1 << 4),
        // ASSET_INFLATABLE means the asset supply can grow
        ASSET_INFLATABLE = (1 << 5),
        // ASSET_DIVISIBLE means the asset (unit) can be broken down 
        ASSET_DIVISIBLE = (1 << 6)
    };

    enum AssetType : uint32_t
    {
        TOKEN = 1,
        UNIQUE = 2,
        EQUITY = 3,
        POINTS = 4,
        CREDITS = 5,
    };

    AssetMetadata(){ SetEmpty();}

    SERIALIZE_METHODS(AssetMetadata, obj) { READWRITE(obj.nVersion, obj.nFlags, obj.nType, obj.nExpiry, obj.sAssetName, obj.sAssetShortName, obj.contract_url, obj.sIssuingaddress, obj.scriptcode); }

    void SetEmpty()
    {
        nVersion=0;
        nFlags=0;
        nType=0;
        nExpiry=0;
        contract_url=""; //online human readable contract
        sIssuingaddress="";
        scriptcode= CScript();
    }

    bool IsEmpty() const ;

    uint32_t GetVersion() const { return nVersion; }
    uint32_t GetType() const { return nType; }
    uint32_t GetFlags() const { return nFlags; }
    uint32_t GetExpiry() const { return nExpiry; }

    const std::string getAssetName() const;
    const std::string getShortName() const;

    bool isTransferable() const;

    bool isConvertable() const;

    bool isLimited() const;

    bool isRestricted() const;

    bool isStakeable() const;

    bool isInflatable() const;
    
    bool isDivisible() const;

    /** Compute the hash of this CAsset. This is computed on the fly.*/
    uint256 GetMetaHash() const;

    void setName(const std::string& _sAssetName);

    void setShortName(const std::string& _sAssetName);

};

struct CAsset : public AssetMetadata
{
    CAssetID assetID;
    std::vector<unsigned char> vchAssetSig; // signature

    CAsset(){SetNull();}
    explicit CAsset(const uint256& assetIDIn) : assetID(assetIDIn) { }
    explicit CAsset(const std::vector<unsigned char>& vchassetIDIn) : assetID(vchassetIDIn) { }

    CAsset(const AssetMetadata &meta)
    {
		SetNull();
        *(static_cast<AssetMetadata*>(this)) = meta;
        assetID = meta.GetMetaHash();
    }

    AssetMetadata GetAssetMetadata() const
    {
        AssetMetadata metadata;
        metadata.nVersion=nVersion;
        metadata.nFlags=nFlags;
        metadata.nType=nType;
        metadata.nExpiry=nExpiry;
        for(unsigned int i = 0; i < sizeof (sAssetName) / sizeof (sAssetName[0]);i++)
          metadata.sAssetName[i] = sAssetName[i];
        for(unsigned int i = 0; i < sizeof (sAssetShortName) / sizeof (sAssetShortName[0]); i++)
          metadata.sAssetShortName[i] = sAssetShortName[i];
        return metadata;
    }

    SERIALIZE_METHODS(CAsset, obj)
    {
        READWRITEAS(AssetMetadata, obj);
        READWRITE(obj.assetID);
        READWRITE(obj.vchAssetSig);
    }

    bool IsNull() const { return assetID.IsNull() && IsEmpty(); }

    void SetNull() {
        assetID.SetNull();
        AssetMetadata::SetEmpty();
    }

    unsigned char* begin() { return assetID.begin(); }
    unsigned char* end() { return assetID.end(); }
    const unsigned char* begin() const { return assetID.begin(); }
    const unsigned char* end() const { return assetID.end(); }

    std::string GetHex() const { return assetID.GetHex(); }
    void SetHex(const std::string& str) { assetID.SetHex(str); }

    friend bool operator==(const CAsset& a, const CAsset& b)
    {
        return a.assetID == b.assetID;
    }

    friend bool operator!=(const CAsset& a, const CAsset& b)
    {
        return !(a == b);
    }

    friend bool operator<(const CAsset& a, const CAsset& b)
    {
        return a.assetID < b.assetID;
    }

    uint256 GetHash() const;

    uint256 GetHashWithoutSign() const;
    const std::string getSignature() const;

    CAssetID GetAssetID() const
    {
        return assetID;
    }

    std::string ToString(bool mini = true) const;
};

/** Used for consensus fee and general wallet accounting*/
typedef std::map<CAsset, CAmount> CAmountMap;

CAmountMap& operator+=(CAmountMap& a, const CAmountMap& b);
CAmountMap& operator-=(CAmountMap& a, const CAmountMap& b);
CAmountMap operator+(const CAmountMap& a, const CAmountMap& b);
CAmountMap operator-(const CAmountMap& a, const CAmountMap& b);

CAmountMap operator/(const CAmountMap& a, const CAmountMap& b);
CAmountMap operator/=(const CAmountMap& a, const CAmountMap& b);
CAmountMap operator*(const CAmountMap& a, const CAmountMap& b);
CAmountMap operator*=(const CAmountMap& a, const CAmountMap& b);


CAmountMap operator*(const CAmountMap& a, const CAmount& b);
CAmountMap operator*=(const CAmountMap& a, const CAmount& b);

CAmountMap operator/(const CAmountMap& a, const CAmount& b);
CAmountMap operator/=(const CAmountMap& a, const CAmount& b);

CAmountMap operator+(const CAmountMap& a, const CAmount& b);
CAmountMap operator+=(const CAmountMap& a, const CAmount& b);

CAmountMap operator-(const CAmountMap& a, const CAmount& b);
CAmountMap operator-=(const CAmountMap& a, const CAmount& b);

CAmountMap operator%(const CAmountMap& a, const CAmount& b);

std::ostream& operator <<(std::ostream& os,const CAmountMap& a);

template<class T>
const CAmountMap& max(const T& a, const T& b)
{
    return (a < b) ? b : a;
}

// WARNING: Comparisons are only looking for *complete* ordering.
// For strict inequality checks, if any entry would fail the non-strict
// inequality, the comparison will fail. Therefore it is possible
// that all inequality comparison checks may fail.
// Therefore if >/< fails against a CAmountMap(), this means there
// are all zeroes or one or more negative values.
//
// Examples: 1A + 2B <= 1A + 2B + 1C
//      and  1A + 2B <  1A + 2B + 1C
//                   but
//           !(1A + 2B == 1A + 2B + 1C)
//-------------------------------------
//           1A + 2B == 1A + 2B
//      and  1A + 2B <= 1A + 2B
//                   but
//           !(1A + 2B < 1A + 2B)
//-------------------------------------
//           !(1A + 2B == 2B - 1C)
//           !(1A + 2B >= 2B - 1C)
//                     ...
//           !(1A + 2B < 2B - 1C)
//      and   1A + 2B != 2B - 1C
bool operator<(const CAmountMap& a, const CAmountMap& b);
bool operator<=(const CAmountMap& a, const CAmountMap& b);
bool operator>(const CAmountMap& a, const CAmountMap& b);
bool operator>=(const CAmountMap& a, const CAmountMap& b);
bool operator==(const CAmountMap& a, const CAmountMap& b);
bool operator!=(const CAmountMap& a, const CAmountMap& b);
bool operator!(const CAmountMap& a); // Check if all values are 0

inline bool MoneyRange(const CAmountMap& mapValue) {
    for(CAmountMap::const_iterator it = mapValue.begin(); it != mapValue.end(); it++) {
        if (it->second < 0) {
            return false;
        }
    }    
   return true;
}

inline std::string mapToString(CAmountMap& map)
{
    std::string result = "";
    for (auto it = map.begin(); it != map.end(); it++) {
        result += it->first.getAssetName() + " " + strprintf("%d", it->second)+ "\n";
    }
    return result;
}

CAmount valueFor(const CAmountMap& mapValue, const CAsset& asset);
CAmount convertfloor(CAsset &a, CAsset &b);
std::string AssetTypeToString(uint32_t &type);

#endif //  CROWN_AMOUNT_H

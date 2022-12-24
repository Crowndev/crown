// Copyright (c) 2017-2017 Block Mechanic
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/asset.h>
#include <script/script.h>
#include <base58.h>
#include <hash.h>
#include <key_io.h>
#include <util/moneystr.h>
#include <util/system.h>
CAmountMap& operator+=(CAmountMap& a, const CAmountMap& b)
{
    for(std::map<CAsset, CAmount>::const_iterator it = b.begin(); it != b.end(); ++it)
        a[it->first] += it->second;
    return a;
}

CAmountMap& operator-=(CAmountMap& a, const CAmountMap& b)
{
    for(std::map<CAsset, CAmount>::const_iterator it = b.begin(); it != b.end(); ++it)
        a[it->first] -= it->second;
    return a;
}

CAmountMap operator+(const CAmountMap& a, const CAmountMap& b)
{
    CAmountMap c;
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it)
        c[it->first] += it->second;
    for(std::map<CAsset, CAmount>::const_iterator it = b.begin(); it != b.end(); ++it)
        c[it->first] += it->second;
    return c;
}

CAmountMap operator-(const CAmountMap& a, const CAmountMap& b)
{
    CAmountMap c;
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it)
        c[it->first] += it->second;
    for(std::map<CAsset, CAmount>::const_iterator it = b.begin(); it != b.end(); ++it)
        c[it->first] -= it->second;
    return c;
}

CAmountMap operator*=(const CAmountMap& a, const CAmount& b)
{
    CAmountMap c;
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it)
        c[it->first] = it->second*b;
    return c;
}

CAmountMap operator*(const CAmountMap& a, const CAmount& b)
{
    CAmountMap c;
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it)
        c[it->first] = it->second*b;
    return c;
}

CAmountMap operator/=(const CAmountMap& a, const CAmount& b)
{
    CAmountMap c;
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it)
        c[it->first] = it->second/b;
    return c;
}

CAmountMap operator/(const CAmountMap& a, const CAmount& b)
{
    CAmountMap c;
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it)
        c[it->first] = it->second/b;
    return c;
}

CAmountMap operator%(const CAmountMap& a, const CAmount& b)
{
    CAmountMap c;
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it)
        c[it->first] = it->second%b;
    return c;
}

CAmountMap operator+=(const CAmountMap& a, const CAmount& b)
{
    CAmountMap c;
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it)
        c[it->first] = it->second+b;
    return c;
}

CAmountMap operator+(const CAmountMap& a, const CAmount& b)
{
    CAmountMap c;
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it)
        c[it->first] = it->second+b;
    return c;
}

CAmountMap operator-=(const CAmountMap& a, const CAmount& b)
{
    CAmountMap c;
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it)
        c[it->first] = it->second-b;
    return c;
}

CAmountMap operator-(const CAmountMap& a, const CAmount& b)
{
    CAmountMap c;
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it)
        c[it->first] = it->second-b;
    return c;
}

bool operator<(const CAmountMap& a, const CAmountMap& b)
{
    bool smallerElement = false;
    for(std::map<CAsset, CAmount>::const_iterator it = b.begin(); it != b.end(); ++it) {
        CAmount aValue = a.count(it->first) ? a.find(it->first)->second : 0;
        if (aValue > it->second)
            return false;
        if (aValue < it->second)
            smallerElement = true;
    }
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it) {
        CAmount bValue = b.count(it->first) ? b.find(it->first)->second : 0;
        if (it->second > bValue)
            return false;
        if (it->second < bValue)
            smallerElement = true;
    }
    return smallerElement;
}

bool operator<=(const CAmountMap& a, const CAmountMap& b)
{
    for(std::map<CAsset, CAmount>::const_iterator it = b.begin(); it != b.end(); ++it) {
        CAmount aValue = a.count(it->first) ? a.find(it->first)->second : 0;
        if (aValue > it->second)
            return false;
    }
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it) {
        CAmount bValue = b.count(it->first) ? b.find(it->first)->second : 0;
        if (it->second > bValue)
            return false;
    }
    return true;
}

bool operator>(const CAmountMap& a, const CAmountMap& b)
{
    bool largerElement = false;
    for(std::map<CAsset, CAmount>::const_iterator it = b.begin(); it != b.end(); ++it) {
        CAmount aValue = a.count(it->first) ? a.find(it->first)->second : 0;
        if (aValue < it->second)
            return false;
        if (aValue > it->second)
            largerElement = true;
    }
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it) {
        CAmount bValue = b.count(it->first) ? b.find(it->first)->second : 0;
        if (it->second < bValue)
            return false;
        if (it->second > bValue)
            largerElement = true;
    }
    return largerElement;
}

bool operator>=(const CAmountMap& a, const CAmountMap& b)
{
    for(std::map<CAsset, CAmount>::const_iterator it = b.begin(); it != b.end(); ++it) {
        if ((a.count(it->first) ? a.find(it->first)->second : 0) < it->second)
            return false;
    }
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it) {
        if (it->second < (b.count(it->first) ? b.find(it->first)->second : 0))
            return false;
    }
    return true;
}

bool operator==(const CAmountMap& a, const CAmountMap& b)
{
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it) {
        if ((b.count(it->first) ? b.find(it->first)->second : 0) != it->second)
            return false;
    }
    for(std::map<CAsset, CAmount>::const_iterator it = b.begin(); it != b.end(); ++it) {
        if ((a.count(it->first) ? a.find(it->first)->second : 0) != it->second)
            return false;
    }
    return true;
}

bool operator!=(const CAmountMap& a, const CAmountMap& b)
{
    return !(a == b);
}

bool operator!(const CAmountMap& a)
{
    for (const auto& it : a) {
        if (it.second) return false;
    }
    return true;
}

std::ostream& operator <<(std::ostream& os, const CAmountMap& a){
    for(auto&b:a)
        os << b.first.ToString() << " : " << b.second << " ";
    return os;
}


bool hasNegativeValue(const CAmountMap& amount)
{
    for(std::map<CAsset, CAmount>::const_iterator it = amount.begin(); it != amount.end(); ++it) {
        if (it->second < 0)
            return true;
    }
    return false;
}

bool hasNonPostiveValue(const CAmountMap& amount)
{
    for(std::map<CAsset, CAmount>::const_iterator it = amount.begin(); it != amount.end(); ++it) {
        if (it->second <= 0)
            return true;
    }
    return false;
}

CAmount valueFor(const CAmountMap& mapValue, const CAsset& asset) {
    CAmountMap::const_iterator it = mapValue.find(asset);
    if (it != mapValue.end()) {
        return it->second;
    } else {
        return CAmount(0);
    }
}

bool AssetMetadata::IsEmpty() const
{
    return ( nVersion==0 && nFlags==0 && nType==0);
}

void AssetMetadata::setName(const std::string& _sAssetName)
{
    std::string padded = std::string( (10 - strlen(_sAssetName.c_str()) ), '0').append(_sAssetName);
    strcat( reinterpret_cast <char*>(sAssetName), padded.c_str());
}

void AssetMetadata::setShortName(const std::string& _sAssetShortName)
{
    std::string padded = std::string( (4 - strlen(_sAssetShortName.c_str()) ), '0').append(_sAssetShortName);
    strcat( reinterpret_cast <char*>(sAssetShortName), padded.c_str());
}

const std::string AssetMetadata::getAssetName() const
{
    std::string data (reinterpret_cast<const char *> (sAssetName),
                     sizeof (sAssetName) / sizeof (sAssetName[0]));
    data.erase(0, data.find_first_not_of('0'));
    data.erase(std::find(data.begin(), data.end(), '\0'), data.end());
    return data;
}

const std::string AssetMetadata::getShortName() const
{
    std::string data (reinterpret_cast<const char *> (sAssetShortName),
                     sizeof (sAssetShortName) / sizeof (sAssetShortName[0]));
    data.erase(0, data.find_first_not_of('0'));
    data.erase(std::find(data.begin(), data.end(), '\0'), data.end());
    return data;
}

bool AssetMetadata::isTransferable() const{
    return nFlags & AssetFlags::ASSET_TRANSFERABLE;
}

bool AssetMetadata::isLimited() const{
    return nFlags & AssetFlags::ASSET_LIMITED;
}

bool AssetMetadata::isConvertable() const{
    return nFlags & AssetFlags::ASSET_CONVERTABLE;
}

bool AssetMetadata::isRestricted() const{
    return nFlags & AssetFlags::ASSET_RESTRICTED;
}

bool AssetMetadata::isStakeable() const{
    return nFlags & AssetFlags::ASSET_STAKEABLE;
}

bool AssetMetadata::isInflatable() const{
    return nFlags & AssetFlags::ASSET_INFLATABLE;
}

bool AssetMetadata::isDivisible() const{
    return nFlags & AssetFlags::ASSET_DIVISIBLE;
}

uint256 AssetMetadata::GetMetaHash() const
{
    return SerializeHash(*this);
}

uint256 CAsset::GetHashWithoutSign() const
{
    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << GetAssetMetadata() << assetID ;
    return ss.GetHash();
}

uint256 CAsset::GetHash() const
{
    return SerializeHash(*this);
}

const std::string CAsset::getSignature() const
{
    //std::string data (vchAssetSig.begin(),vchAssetSig.end());
    return "";
}

std::string CAsset::ToString(bool mini) const
{
    std::string str ="";

    if(mini){
        str +=    strprintf("%s", getAssetName());
        return str;
    }

    str +=    strprintf("Name : %s\n", getAssetName());
    str +=    strprintf("Symbol : %s \n", getShortName());
    str +=    strprintf("CAssetID : %s \n",GetHex());
    //str +=    strprintf("Contract Hash : %s \n",contract_hash.ToString());
    str += strprintf("Details (ver=%d, transferable=%s, convertable=%s, limited=%s, restricted=%s, stakable=%s, infatable=%s, divisible=%s, type=%s, expiry=%d)\n", nVersion, isTransferable() ? "yes" : "no", isConvertable() ? "yes" : "no", isLimited() ? "yes" : "no", isRestricted() ? "yes" : "no", isStakeable() ? "yes" : "no", isInflatable() ? "yes" : "no", isDivisible() ? "yes" : "no", GetType(), GetExpiry());

    return str;
}

CAmount convertfloor(CAsset &a, CAsset &b)
{
    return 0;
}


CAmountMap operator*=(const CAmountMap& a, const CAmountMap& b)
{
    CAmountMap c;
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it)
        for(std::map<CAsset, CAmount>::const_iterator jt = b.begin(); jt != b.end(); ++jt)
            if(it->first == jt->first)
                c[it->first] = it->second*jt->second;
    return c;
}

CAmountMap operator*(const CAmountMap& a, const CAmountMap& b)
{
    CAmountMap c;
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it)
        for(std::map<CAsset, CAmount>::const_iterator jt = b.begin(); jt != b.end(); ++jt)
            if(it->first == jt->first)
                c[it->first] = it->second*jt->second;
    return c;
}

CAmountMap operator/=(const CAmountMap& a, const CAmountMap& b)
{
    CAmountMap c;
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it)
        for(std::map<CAsset, CAmount>::const_iterator jt = b.begin(); jt != b.end(); ++jt)
            if(it->first == jt->first)
                c[it->first] = it->second/jt->second;
    return c;
}

CAmountMap operator/(const CAmountMap& a, const CAmountMap& b)
{
    CAmountMap c;
    for(std::map<CAsset, CAmount>::const_iterator it = a.begin(); it != a.end(); ++it)
        for(std::map<CAsset, CAmount>::const_iterator jt = b.begin(); jt != b.end(); ++jt)
            if(it->first == jt->first)
                c[it->first] = it->second/jt->second;
    return c;
}

std::string AssetTypeToString(uint32_t &type){
    std::string ret;
    switch(type){
        case AssetMetadata::TOKEN   :
            ret = "Token";
        break;
        case AssetMetadata::UNIQUE  : 
            ret = "Unique";
        break;
        case AssetMetadata::EQUITY  : 
            ret = "Equity";
        break;
        case AssetMetadata::POINTS  : 
            ret = "Points";
        break;
        case AssetMetadata::CREDITS : 
            ret = "Credits";
        break;
    }
    return ret;
}

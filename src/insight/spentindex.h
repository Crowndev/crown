// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_SPENTINDEX_H
#define CROWN_SPENTINDEX_H

#include <uint256.h>
#include <amount.h>
#include "script/script.h"
#include "serialize.h"

struct CSpentIndexKey {
    uint256 txid;
    unsigned int outputIndex;

    SERIALIZE_METHODS(CSpentIndexKey, obj) { READWRITE(obj.txid, obj.outputIndex); }

    CSpentIndexKey(uint256 t, unsigned int i) {
        txid = t;
        outputIndex = i;
    }

    CSpentIndexKey() {
        SetNull();
    }

    void SetNull() {
        txid.SetNull();
        outputIndex = 0;
    }

};

struct CSpentIndexValue {
    uint256 txid;
    unsigned int inputIndex;
    int blockHeight;
    CAmount satoshis;
    int addressType;
    uint160 addressHash;

    SERIALIZE_METHODS(CSpentIndexValue, obj) {READWRITE(obj.txid, obj.inputIndex, obj.blockHeight, obj.satoshis, obj.addressType, obj.addressHash);}

    CSpentIndexValue(uint256 t, unsigned int i, int h, CAmount s, int type, uint160 a) {
        txid = t;
        inputIndex = i;
        blockHeight = h;
        satoshis = s;
        addressType = type;
        addressHash = a;
    }

    CSpentIndexValue() {
        SetNull();
    }

    void SetNull() {
        txid.SetNull();
        inputIndex = 0;
        blockHeight = 0;
        satoshis = 0;
        addressType = 0;
        addressHash.SetNull();
    }

    bool IsNull() const {
        return txid.IsNull();
    }
};

struct CSpentIndexKeyCompare
{
    bool operator()(const CSpentIndexKey& a, const CSpentIndexKey& b) const {
        if (a.txid == b.txid) {
            return a.outputIndex < b.outputIndex;
        } else {
            return a.txid < b.txid;
        }
    }
};

struct CSpentIndexTxInfo
{
    std::map<CSpentIndexKey, CSpentIndexValue, CSpentIndexKeyCompare> mSpentInfo;
};

struct CAddressUnspentKey {
    unsigned int type;
    uint160 hashBytes;
    std::string asset;
    uint256 txhash;
    size_t index;

    SERIALIZE_METHODS(CAddressUnspentKey, obj) {READWRITE(obj.type, obj.hashBytes, obj.asset, obj.txhash, obj.index);}

    CAddressUnspentKey(unsigned int addressType, uint160 addressHash, std::string sAssetName, uint256 txid, size_t indexValue) {
        type = addressType;
        hashBytes = addressHash;
        asset = sAssetName;
        txhash = txid;
        index = indexValue;
    }

    CAddressUnspentKey() {
        SetNull();
    }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
        asset.clear();
        txhash.SetNull();
        index = 0;
    }
};

struct CAddressUnspentValue {
    CAmount satoshis;
    CScript script;
    int blockHeight;

    SERIALIZE_METHODS(CAddressUnspentValue, obj) { READWRITE(obj.satoshis, obj.script, obj.blockHeight); }

    CAddressUnspentValue(CAmount sats, CScript scriptPubKey, int height) {
        satoshis = sats;
        script = scriptPubKey;
        blockHeight = height;
    }

    CAddressUnspentValue() {
        SetNull();
    }

    void SetNull() {
        satoshis = -1;
        script.clear();
        blockHeight = 0;
    }

    bool IsNull() const {
        return (satoshis == -1);
    }
};

struct CAddressIndexKey {
    unsigned int type;
    uint160 hashBytes;
    std::string asset;
    int blockHeight;
    unsigned int txindex;
    uint256 txhash;
    size_t index;
    bool spending;

    SERIALIZE_METHODS(CAddressIndexKey, obj) {READWRITE(obj.type, obj.hashBytes, obj.asset, obj.blockHeight, obj.txindex, obj.txhash, obj.index, obj.spending);}

    CAddressIndexKey(unsigned int addressType, uint160 addressHash, std::string sAssetName, int height, int blockindex,
                     uint256 txid, size_t indexValue, bool isSpending) {
        type = addressType;
        hashBytes = addressHash;
        asset = sAssetName;
        blockHeight = height;
        txindex = blockindex;
        txhash = txid;
        index = indexValue;
        spending = isSpending;
    }

    CAddressIndexKey() {
        SetNull();
    }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
        asset.clear();
        blockHeight = 0;
        txindex = 0;
        txhash.SetNull();
        index = 0;
        spending = false;
    }

};

struct CAddressIndexIteratorKey {
    unsigned int type;
    uint160 hashBytes;

    SERIALIZE_METHODS(CAddressIndexIteratorKey, obj) {READWRITE(obj.type, obj.hashBytes);}

    CAddressIndexIteratorKey(unsigned int addressType, uint160 addressHash) {
        type = addressType;
        hashBytes = addressHash;
    }

    CAddressIndexIteratorKey() {
        SetNull();
    }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
    }
};

struct CAddressIndexIteratorAssetKey {
    unsigned int type;
    uint160 hashBytes;
    std::string asset;

    SERIALIZE_METHODS(CAddressIndexIteratorAssetKey, obj) {READWRITE(obj.type, obj.hashBytes, obj.asset);}

    CAddressIndexIteratorAssetKey(unsigned int addressType, uint160 addressHash, std::string sAssetName) {
        type = addressType;
        hashBytes = addressHash;
        asset = sAssetName;
    }

    CAddressIndexIteratorAssetKey() {
        SetNull();
    }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
        asset.clear();
    }
};

struct CAddressIndexIteratorHeightKey {
    unsigned int type;
    uint160 hashBytes;
    std::string asset;
    int blockHeight;

    SERIALIZE_METHODS(CAddressIndexIteratorHeightKey, obj) {READWRITE(obj.type, obj.hashBytes, obj.asset, obj.blockHeight);}

    CAddressIndexIteratorHeightKey(unsigned int addressType, uint160 addressHash, std::string sAssetName, int height) {
        type = addressType;
        hashBytes = addressHash;
        asset = sAssetName;
        blockHeight = height;
    }

    CAddressIndexIteratorHeightKey() {
        SetNull();
    }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
        asset.clear();
        blockHeight = 0;
    }
};

#endif // CROWN_SPENTINDEX_H

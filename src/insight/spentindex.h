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
    CAsset asset;
    int addressType;
    uint160 addressHash;

    SERIALIZE_METHODS(CSpentIndexValue, obj) {READWRITE(obj.txid, obj.inputIndex, obj.blockHeight, obj.satoshis, obj.asset, obj.addressType, obj.addressHash);}

    CSpentIndexValue(uint256 t, unsigned int i, int h, CAmount s, CAsset at, int type, uint160 a) {
        txid = t;
        inputIndex = i;
        blockHeight = h;
        satoshis = s;
        asset=at;
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
        asset.SetNull();
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
    CAsset asset;
    uint256 txhash;
    unsigned int  index;

    SERIALIZE_METHODS(CAddressUnspentKey, obj) {READWRITE(obj.type, obj.hashBytes, obj.asset, obj.txhash, obj.index);}

    CAddressUnspentKey(unsigned int addressType, uint160 addressHash, CAsset at, uint256 txid, unsigned int  indexValue) {
        type = addressType;
        hashBytes = addressHash;
        asset = at;
        txhash = txid;
        index = indexValue;
    }

    CAddressUnspentKey() {
        SetNull();
    }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
        asset.SetNull();
        txhash.SetNull();
        index = 0;
    }
};

struct CAddressUnspentValue {
    CAmount satoshis;
    CAsset asset;
    CScript script;
    int blockHeight;

    SERIALIZE_METHODS(CAddressUnspentValue, obj) { READWRITE(obj.satoshis, obj.asset, obj.script, obj.blockHeight); }

    CAddressUnspentValue(CAmount sats, CAsset at, CScript scriptPubKey, int height) {
        satoshis = sats;
        asset = at;
        script = scriptPubKey;
        blockHeight = height;
    }

    CAddressUnspentValue() {
        SetNull();
    }

    void SetNull() {
        satoshis = -1;
        asset.SetNull();
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
    CAsset asset;
    int blockHeight;
    unsigned int txindex;
    uint256 txhash;
    unsigned int  index;
    bool spending;

    SERIALIZE_METHODS(CAddressIndexKey, obj) {READWRITE(obj.type, obj.hashBytes, obj.asset, obj.blockHeight, obj.txindex, obj.txhash, obj.index, obj.spending);}

    CAddressIndexKey(unsigned int addressType, uint160 addressHash, CAsset at, int height, int blockindex,
                     uint256 txid, unsigned int indexValue, bool isSpending) {
        type = addressType;
        hashBytes = addressHash;
        asset = at;
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
        asset.SetNull();
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
    CAsset asset;

    SERIALIZE_METHODS(CAddressIndexIteratorAssetKey, obj) {READWRITE(obj.type, obj.hashBytes, obj.asset);}

    CAddressIndexIteratorAssetKey(unsigned int addressType, uint160 addressHash, CAsset at) {
        type = addressType;
        hashBytes = addressHash;
        asset = at;
    }

    CAddressIndexIteratorAssetKey() {
        SetNull();
    }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
        asset.SetNull();
    }
};

struct CAddressIndexIteratorHeightKey {
    unsigned int type;
    uint160 hashBytes;
    CAsset asset;
    int blockHeight;

    SERIALIZE_METHODS(CAddressIndexIteratorHeightKey, obj) {READWRITE(obj.type, obj.hashBytes, obj.asset, obj.blockHeight);}

    CAddressIndexIteratorHeightKey(unsigned int addressType, uint160 addressHash, CAsset at, int height) {
        type = addressType;
        hashBytes = addressHash;
        asset = at;
        blockHeight = height;
    }

    CAddressIndexIteratorHeightKey() {
        SetNull();
    }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
        asset.SetNull();
        blockHeight = 0;
    }
};

#endif // CROWN_SPENTINDEX_H

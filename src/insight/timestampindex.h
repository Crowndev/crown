// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_TIMESTAMPINDEX_H
#define CROWN_TIMESTAMPINDEX_H

#include <uint256.h>

struct CTimestampIndexIteratorKey {
    unsigned int timestamp;
    SERIALIZE_METHODS(CTimestampIndexIteratorKey, obj) { READWRITE(obj.timestamp); }

    explicit CTimestampIndexIteratorKey(unsigned int time) {
        timestamp = time;
    }

    CTimestampIndexIteratorKey() {
        SetNull();
    }

    void SetNull() {
        timestamp = 0;
    }
};

struct CTimestampIndexKey {
    unsigned int timestamp;
    uint256 blockHash;

    SERIALIZE_METHODS(CTimestampIndexKey, obj) { READWRITE(obj.timestamp, obj.blockHash); }

    explicit CTimestampIndexKey(unsigned int time, uint256 hash) {
        timestamp = time;
        blockHash = hash;
    }

    CTimestampIndexKey() {
        SetNull();
    }

    void SetNull() {
        timestamp = 0;
        blockHash.SetNull();
    }
};

struct CTimestampBlockIndexKey {
    uint256 blockHash;
    SERIALIZE_METHODS(CTimestampBlockIndexKey, obj) { READWRITE(obj.blockHash); }

    explicit CTimestampBlockIndexKey(uint256 hash) {
        blockHash = hash;
    }

    CTimestampBlockIndexKey() {
        SetNull();
    }

    void SetNull() {
        blockHash.SetNull();
    }
};

struct CTimestampBlockIndexValue {
    unsigned int ltimestamp;

    SERIALIZE_METHODS(CTimestampBlockIndexValue, obj) { READWRITE(obj.ltimestamp); }

    explicit CTimestampBlockIndexValue (unsigned int time) {
        ltimestamp = time;
    }

    CTimestampBlockIndexValue() {
        SetNull();
    }

    void SetNull() {
        ltimestamp = 0;
    }
};

#endif // CROWN_TIMESTAMPINDEX_H

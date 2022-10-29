// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_ADDRESSINDEX_H
#define CROWN_ADDRESSINDEX_H

#include <primitives/asset.h>
#include <chrono>

enum AddressIndexType {
    ADDR_INDT_UNKNOWN                = 0,
    ADDR_INDT_PUBKEY_ADDRESS         = 1,
    ADDR_INDT_SCRIPT_ADDRESS         = 2,
    ADDR_INDT_WITNESS_V0_KEYHASH     = 5,
    ADDR_INDT_WITNESS_V0_SCRIPTHASH  = 6,
};

struct CMempoolAddressDelta
{
    std::chrono::seconds time;
    CAmount amount;
    CAsset asset;
    uint256 prevhash;
    unsigned int prevout;

    CMempoolAddressDelta(std::chrono::seconds t, CAmount a, CAsset at, uint256 hash, unsigned int out) {
        time = t;
        amount = a;
        asset = at;
        prevhash = hash;
        prevout = out;
    }

    CMempoolAddressDelta(std::chrono::seconds t, CAmount a, CAsset at) {
        time = t;
        amount = a;
        asset = at;
        prevhash.SetNull();
        prevout = 0;
    }
};

struct CMempoolAddressDeltaKey
{
    int type;
    uint160 addressBytes;
    uint256 txhash;
    unsigned int index;
    int spending;

    CMempoolAddressDeltaKey(int addressType, uint160 addressHash,
                            uint256 hash, unsigned int i, int s) {
        type = addressType;
        addressBytes = addressHash;
        txhash = hash;
        index = i;
        spending = s;
    }

    CMempoolAddressDeltaKey(int addressType, uint160 addressHash) {
        type = addressType;
        addressBytes = addressHash;
        txhash.SetNull();
        index = 0;
        spending = 0;
    }
};

struct CMempoolAddressDeltaKeyCompare
{
    bool operator()(const CMempoolAddressDeltaKey& a, const CMempoolAddressDeltaKey& b) const {
        if (a.type == b.type) {
            if (a.addressBytes == b.addressBytes) {
                if (a.txhash == b.txhash) {
                    if (a.index == b.index) {
                        return a.spending < b.spending;
                    } else {
                        return a.index < b.index;
                    }
                } else {
                    return a.txhash < b.txhash;
                }
            } else {
                return a.addressBytes < b.addressBytes;
            }
        } else {
            return a.type < b.type;
        }
    }
};

#endif // CROWN_ADDRESSINDEX_H

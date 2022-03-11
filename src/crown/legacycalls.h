// Copyright (c) 2020 The Crown developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_LEGACYCALLS_H
#define CROWN_LEGACYCALLS_H

class CTxIn;
class uint256;

int GetIXConfirmations(uint256 nTXHash);
int GetTransactionAge(const uint256& txid);
int GetInputAge(const CTxIn& vin);
int GetInputHeight(const CTxIn& vin);

#endif // CROWN_LEGACYCALLS_H

// Copyright (c) 2016-2020 The Elements developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PRIMITIVES_TXWITNESS_H
#define CROWN_PRIMITIVES_TXWITNESS_H

#include <script/script.h>
#include <uint256.h>

class CTxInWitness
{
public:
    CScriptWitness scriptWitness;
    
    SERIALIZE_METHODS(CTxInWitness, obj)
    {
        READWRITE(obj.scriptWitness.stack);
    }

    CTxInWitness() {SetNull();}

    bool IsNull() const
    {
        return scriptWitness.IsNull();
    }
    void SetNull()
    {
        scriptWitness.stack.clear();
    }

    uint256 GetHash() const;
};

class CTxWitness
{
public:
    /** In case vtxinwit is missing, all entries are treated as if they were empty CTxInWitnesses */
    std::vector<CTxInWitness> vtxinwit;

    CTxWitness() { SetNull();}

    SERIALIZE_METHODS(CTxWitness, obj)
    {
        for (size_t n = 0; n < obj.vtxinwit.size(); n++) {
            READWRITE(obj.vtxinwit[n]);
        }
        if (obj.IsNull()) {
            /* It's illegal to encode a witness when all vtxinwit and vtxoutwit entries are empty. */
            throw std::ios_base::failure("Superfluous witness record null");
        }
    }

    bool IsEmpty() const
    {
        return vtxinwit.empty();
    }

    bool IsNull() const
    {
        for (size_t n = 0; n < vtxinwit.size(); n++) {
            if (!vtxinwit[n].IsNull()) {
                return false;
            }
        }
        return true;
    }

    void SetNull()
    {
        vtxinwit.clear();
    }
};


/*
 * Compute the Merkle root of the transactions in a block using mid-state only.
 * Note that the merkle root calculated with this method is not the same as the
 * one computed by ComputeMerkleRoot.
 */
uint256 ComputeFastMerkleRoot(const std::vector<uint256>& hashes);


#endif //CROWN_PRIMITIVES_TXWITNESS_H

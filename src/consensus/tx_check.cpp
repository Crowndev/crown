// Copyright (c) 2017-2019 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/tx_check.h>
#include <logging.h>

#include <consensus/consensus.h>
#include <primitives/transaction.h>
#include <consensus/validation.h>
#include <chainparams.h>

static bool CheckData(TxValidationState &state, const CTxData *p)
{
    if (p->vData.size() < 1) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-output-data-size-small");
    }

    const size_t MAX_DATA_OUTPUT_SIZE = 1024; // (max 1024 bytes) 1kb
    if (p->vData.size() > MAX_DATA_OUTPUT_SIZE) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("bad-output-data-size-large %d vs %d\n", p->vData.size(), MAX_DATA_OUTPUT_SIZE));
    }

    return true;
}


static bool CheckContract(TxValidationState &state, const CContract *p)
{
    return true;
}

static bool CheckAsset(TxValidationState &state, CAsset& asset){

    //check version

    if (asset.nVersion > 1)
        return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("invalid asset version %d \n", asset.nVersion));

    if(asset.getAssetName() == "" || asset.getAssetName().length() < 4)
        return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("invalid asset name %s \n", asset.sAssetName));

    if(asset.getShortName() == "" || asset.getShortName().length() < 3)
        return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("invalid asset symbol %s \n", asset.sAssetShortName));

    //check type and it's rules

    if (asset.nType < 1 || asset.nType > 5)
        return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("invalid asset type %d \n", asset.nVersion));

    //check properties
    //token
    if (asset.nType == 1){
        if(!asset.isTransferable())
            return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("invalid properties, asset type is %s, but is not marked Transferable \n", AssetTypeToString(asset.nType)));
    }

    //unique
    if (asset.nType == 2){

        if(asset.isInflatable())
            return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("invalid properties, asset type is %s, but is marked Inflatable \n", AssetTypeToString(asset.nType)));

        if(asset.isStakeable())
            return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("invalid properties, asset type is %s, but is marked Stakeable \n", AssetTypeToString(asset.nType)));

        if(asset.isConvertable())
            return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("invalid properties, asset type is %s, but is marked convertible \n", AssetTypeToString(asset.nType)));

        if(!asset.isLimited())
            return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("invalid properties, asset type is %s, but is not marked limited \n", AssetTypeToString(asset.nType)));

        if(!asset.isRestricted())
            return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("invalid properties, asset type is %s, but is not marked restricted \n", AssetTypeToString(asset.nType)));

        //check expiry
        //if (asset.nExpiry != 0 && asset.nExpiry < GetTime())

    }

    //equity
    if (asset.nType == 3){
        if(asset.contract_hash == uint256())
            return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("invalid properties, asset type is %s, but has no contract \n", AssetTypeToString(asset.nType)));

    }

    //points
    if (asset.nType == 4){
        if(asset.isInflatable())
            return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("invalid properties, asset type is %s, but is marked Inflatable \n", AssetTypeToString(asset.nType)));

    }

    //credits
    if (asset.nType == 5){
        if(asset.isInflatable())
            return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("invalid properties, asset type is %s, but is marked Inflatable \n", AssetTypeToString(asset.nType)));

    }
}

bool CheckTransaction(const CTransaction& tx, TxValidationState& state)
{
    // Basic checks that don't depend on any context
    if (tx.vin.empty())
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-vin-empty");
        
    if (tx.nVersion < TX_ELE_VERSION && tx.vout.empty())
        return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("%s , bad-txns-vout-empty, %s", __func__, tx.ToString()));

    if(tx.nVersion >= TX_ELE_VERSION && tx.vpout.empty())
        return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("%s , bad-txns-vpout-empty, %s", __func__, tx.ToString()));
    
    // Size limits (this doesn't take the witness into account, as that hasn't been checked for malleability)
    if (::GetSerializeSize(tx, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * WITNESS_SCALE_FACTOR > MAX_BLOCK_WEIGHT)
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-oversize");
    if (tx.extraPayload.size() > MAX_TX_EXTRA_PAYLOAD)
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-payload-oversize");

    // Check for negative or overflow output values (see CVE-2010-5139)
    CAmount nValueOut = 0;
    int q = (tx.nVersion == TX_ELE_VERSION ? tx.vpout.size() : tx.vout.size());
    for (unsigned int k = 0; k < q; k++) {
        CTxOutAsset txout = (tx.nVersion == TX_ELE_VERSION ? tx.vpout[k] : tx.vout[k]);

        if (txout.nValue < 0)
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-vout-negative");
        if (txout.nValue > MAX_MONEY)
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-vout-toolarge");

        nValueOut += txout.nValue;
        if (!MoneyRange(nValueOut))
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-txouttotal-toolarge");
    }


    if(tx.nVersion >= TX_ELE_VERSION){
		for (unsigned int k = 0; k < tx.vpout.size(); k++) {
            if(tx.vpout[k].nAsset.IsEmpty())
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-vout-not-explicit-asset", strprintf("%s: %s", __func__, tx.ToString()));
         }
     }

    if(tx.nVersion >= TX_ELE_VERSION){
        size_t nContractOutputs = 0, nDataOutputs = 0, nIDOutputs = 0;
        for (unsigned int i = 0; i < tx.vdata.size(); i++){
            uint8_t vers = tx.vdata[i].get()->GetVersion();
            switch (vers) {
                case OUTPUT_CONTRACT:
                    if (!CheckContract(state, (CContract*) tx.vdata[i].get())) {
                        return false;
                    }
                    nContractOutputs++;
                    break;
                case OUTPUT_DATA:
                    if (!CheckData(state, (CTxData*) tx.vdata[i].get())) {
                        return false;
                    }
                    nDataOutputs++;
                    break;
                default:
                    return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("bad-txns-unknown-output-version got %d", vers), strprintf("%s", __func__));
            }
        }

        if (nDataOutputs > 1 || nContractOutputs > 1 || nIDOutputs > 1) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "too-many-data-outputs");
        }
    }

    // Check for duplicate inputs (see CVE-2018-17144)
    // While Consensus::CheckTxInputs does check if all inputs of a tx are available, and UpdateCoins marks all inputs
    // of a tx as spent, it does not check if the tx has duplicate inputs.
    // Failure to run this check will result in either a crash or an inflation bug, depending on the implementation of
    // the underlying coins database.
    std::set<COutPoint> vInOutPoints;
    for (const auto& txin : tx.vin) {
        if (!vInOutPoints.insert(txin.prevout).second)
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-inputs-duplicate");
    }

    if (tx.IsCoinBase())
    {
        if (tx.vin[0].scriptSig.size() < 2 || tx.vin[0].scriptSig.size() > 100)
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-cb-length");
    }
    else if (tx.IsCoinStake())
    {
        if (tx.vin[0].scriptSig.size() > 100)
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-cs-length");
    }
    else
    {
        for (const auto& txin : tx.vin)
            if (txin.prevout.IsNull())
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-prevout-null");
    }

    return true;
}


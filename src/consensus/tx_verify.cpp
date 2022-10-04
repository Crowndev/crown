// Copyright (c) 2017-2019 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/tx_verify.h>

#include <consensus/consensus.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <consensus/validation.h>
#include <assetdb.h>
#include <contractdb.h>
#include <key_io.h>

// TODO remove the following dependencies
#include <chain.h>
#include <coins.h>
#include <util/moneystr.h>
#include <logging.h>

bool IsFinalTx(const CTransaction &tx, int nBlockHeight, int64_t nBlockTime)
{
    if (tx.nLockTime == 0)
        return true;
    if ((int64_t)tx.nLockTime < ((int64_t)tx.nLockTime < LOCKTIME_THRESHOLD ? (int64_t)nBlockHeight : nBlockTime))
        return true;
    for (const auto& txin : tx.vin) {
        if (!(txin.nSequence == CTxIn::SEQUENCE_FINAL))
            return false;
    }
    return true;
}

std::pair<int, int64_t> CalculateSequenceLocks(const CTransaction &tx, int flags, std::vector<int>& prevHeights, const CBlockIndex& block)
{
    assert(prevHeights.size() == tx.vin.size());

    // Will be set to the equivalent height- and time-based nLockTime
    // values that would be necessary to satisfy all relative lock-
    // time constraints given our view of block chain history.
    // The semantics of nLockTime are the last invalid height/time, so
    // use -1 to have the effect of any height or time being valid.
    int nMinHeight = -1;
    int64_t nMinTime = -1;

    // tx.nVersion is signed integer so requires cast to unsigned otherwise
    // we would be doing a signed comparison and half the range of nVersion
    // wouldn't support BIP 68.
    bool fEnforceBIP68 = static_cast<uint32_t>(tx.nVersion) >= 2
                      && flags & LOCKTIME_VERIFY_SEQUENCE;

    // Do not enforce sequence numbers as a relative lock time
    // unless we have been instructed to
    if (!fEnforceBIP68) {
        return std::make_pair(nMinHeight, nMinTime);
    }

    for (size_t txinIndex = 0; txinIndex < tx.vin.size(); txinIndex++) {
        const CTxIn& txin = tx.vin[txinIndex];

        // Sequence numbers with the most significant bit set are not
        // treated as relative lock-times, nor are they given any
        // consensus-enforced meaning at this point.
        if (txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_DISABLE_FLAG) {
            // The height of this input is not relevant for sequence locks
            prevHeights[txinIndex] = 0;
            continue;
        }

        int nCoinHeight = prevHeights[txinIndex];

        if (txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG) {
            int64_t nCoinTime = block.GetAncestor(std::max(nCoinHeight-1, 0))->GetMedianTimePast();
            // NOTE: Subtract 1 to maintain nLockTime semantics
            // BIP 68 relative lock times have the semantics of calculating
            // the first block or time at which the transaction would be
            // valid. When calculating the effective block time or height
            // for the entire transaction, we switch to using the
            // semantics of nLockTime which is the last invalid block
            // time or height.  Thus we subtract 1 from the calculated
            // time or height.

            // Time-based relative lock-times are measured from the
            // smallest allowed timestamp of the block containing the
            // txout being spent, which is the median time past of the
            // block prior.
            nMinTime = std::max(nMinTime, nCoinTime + (int64_t)((txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_MASK) << CTxIn::SEQUENCE_LOCKTIME_GRANULARITY) - 1);
        } else {
            nMinHeight = std::max(nMinHeight, nCoinHeight + (int)(txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_MASK) - 1);
        }
    }

    return std::make_pair(nMinHeight, nMinTime);
}

bool EvaluateSequenceLocks(const CBlockIndex& block, std::pair<int, int64_t> lockPair)
{
    assert(block.pprev);
    int64_t nBlockTime = block.pprev->GetMedianTimePast();
    if (lockPair.first >= block.nHeight || lockPair.second >= nBlockTime)
        return false;

    return true;
}

bool SequenceLocks(const CTransaction &tx, int flags, std::vector<int>& prevHeights, const CBlockIndex& block)
{
    return EvaluateSequenceLocks(block, CalculateSequenceLocks(tx, flags, prevHeights, block));
}

unsigned int GetLegacySigOpCount(const CTransaction& tx)
{
    unsigned int nSigOps = 0;
    for (const auto& txin : tx.vin)
    {
        nSigOps += txin.scriptSig.GetSigOpCount(false);
    }

    for(unsigned int k = 0; k < (tx.nVersion >= TX_ELE_VERSION ? tx.vpout.size() : tx.vout.size()) ; k++){
        CTxOutAsset txout = (tx.nVersion >= TX_ELE_VERSION ? tx.vpout[k] : tx.vout[k]);
        nSigOps += txout.scriptPubKey.GetSigOpCount(false);
    }
    return nSigOps;
}

unsigned int GetP2SHSigOpCount(const CTransaction& tx, const CCoinsViewCache& inputs)
{
    if (tx.IsCoinBase() || tx.IsCoinStake())
        return 0;

    unsigned int nSigOps = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const Coin& coin = inputs.AccessCoin(tx.vin[i].prevout);
        assert(!coin.IsSpent());
        const auto &prevout = coin.out; //(tx.nVersion >= TX_ELE_VERSION ? coin.out2 : coin.out);
        if (prevout.scriptPubKey.IsPayToScriptHash())
            nSigOps += prevout.scriptPubKey.GetSigOpCount(tx.vin[i].scriptSig);
    }
    return nSigOps;
}

int64_t GetTransactionSigOpCost(const CTransaction& tx, const CCoinsViewCache& inputs, int flags)
{
    int64_t nSigOps = GetLegacySigOpCount(tx) * WITNESS_SCALE_FACTOR;

    if (tx.IsCoinBase() || tx.IsCoinStake())
        return nSigOps;

    if (flags & SCRIPT_VERIFY_P2SH) {
        nSigOps += GetP2SHSigOpCount(tx, inputs) * WITNESS_SCALE_FACTOR;
    }

    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const Coin& coin = inputs.AccessCoin(tx.vin[i].prevout);
        assert(!coin.IsSpent());
        const auto &prevout =coin.out; // (tx.nVersion >= TX_ELE_VERSION ? coin.out2 : coin.out);
        const CScriptWitness* pScriptWitness = tx.witness.vtxinwit.size() > i ? &tx.witness.vtxinwit[i].scriptWitness : nullptr;
        nSigOps += CountWitnessSigOps(tx.vin[i].scriptSig, prevout.scriptPubKey, pScriptWitness, flags);
    }
    return nSigOps;
}

bool Consensus::CheckTxInputs(const CTransaction& tx, TxValidationState& state, const CCoinsViewCache& inputs, int nSpendHeight, CAmount& txfee)
{
    // are the actual inputs available?
    if (!inputs.HaveInputs(tx)) {
        return state.Invalid(TxValidationResult::TX_MISSING_INPUTS, "bad-txns-inputs-missing or spent",
                         strprintf("%s: inputs missing/spent", __func__));
    }

    CAmountMap inputAssets;
    CAmountMap outputAssets;

    std::vector<CTxOutAsset> spent_inputs;
    std::vector<CScript> input_addresses;
    CAmount nValueIn = 0;

    for (unsigned int i = 0; i < tx.vin.size(); ++i) {
        const COutPoint &prevout = tx.vin[i].prevout;
        const Coin& coin = inputs.AccessCoin(prevout);
        assert(!coin.IsSpent());

        input_addresses.push_back(coin.out.scriptPubKey);

        // If prev is coinbase, check that it's matured
        if ((coin.IsCoinBase() || coin.IsCoinStake()) && nSpendHeight - coin.nHeight < COINBASE_MATURITY) {
            return state.Invalid(TxValidationResult::TX_PREMATURE_SPEND, "bad-txns-premature-spend-of-coinbase",
                strprintf("tried to spend coinbase at depth %d", nSpendHeight - coin.nHeight));
        }

        spent_inputs.push_back(coin.out);
        if(tx.nVersion >= TX_ELE_VERSION)
            inputAssets[coin.out.nAsset] += coin.out.nValue;
            //inputAssets.insert(std::make_pair(coin.out.nAsset, coin.out.nValue));

        //LogPrintf("Asset : %s\nAmount : %d", coin.out.nAsset.ToString(false), coin.out.nValue);


        // Check for negative or overflow input values
        nValueIn += coin.out.nValue;
        if (!MoneyRange(coin.out.nValue) || !MoneyRange(nValueIn)) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-inputvalues-out-of-range");
        }
    }

    if(tx.nVersion >= TX_ELE_VERSION){
        outputAssets = tx.GetValueOutMap();
    }

    //LogPrintf("Input assets size : %d  outputs assets size : %d", inputAssets.size(), outputAssets.size());

    if(tx.nVersion >= TX_ELE_VERSION && inputAssets.size() < 1)
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-input-size");

    // enforce asset rules
    {
        CAsset subsidy_asset = GetSubsidyAsset();
        //prevent asset merging
        if(inputAssets.size() > 1)
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-input-asset-multiple", strprintf("found (%d) , expected 1", inputAssets.size()));

        // only two outputs assets, at most new/converted asset plus fee in input asset
        if(outputAssets.size() > 2)
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-output-asset", strprintf("found (%d) , expected 2", outputAssets.size()));

        //check asset conversion
        if (outputAssets.size() == 2 && !inputAssets.begin()->first.isConvertable())
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-input-asset-not-convertable ");

        //check asset transfer
        if(!inputAssets.begin()->first.isTransferable())
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-input-asset-not-transferable ", strprintf("Asset : (%s)", inputAssets.begin()->first.ToString(false)));

        for(auto & a : outputAssets){
            CAsset asset = a.first;
            bool exists = assetExists(asset);

            //Asset exists , check for output rules
            if (exists && asset != subsidy_asset) {
                // check asset limited
/*
                if(asset.isLimited() && inputAssets.begin()->first != asset)
                    return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-output-asset-is-limited", strprintf("cannot convert other assets to (%s)", asset.getAssetName()));
*/
                // check asset restricted
                // get contract hash, retrieve contract, get issuer , compare input address to issueraddress
                const CContract &contract = GetContractByHash(asset.contract_hash);
/*
                if(contract.IsEmpty())
                    return state.Invalid(TxValidationResult::TX_CONSENSUS, "contract-not-found", strprintf("contract retrieval failed %s", asset.contract_hash.ToString()));
*/
                CTxDestination address1;
                ExtractDestination(input_addresses.front(), address1);
                if(asset.isRestricted()){
                    if(input_addresses.size() != 1)
                        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-input-issuer", strprintf("Inputs for this restricted asset must come from (%s) only", contract.sIssuingaddress));

                    if(contract.sIssuingaddress != EncodeDestination(address1))
                        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-issuer-mismatch", strprintf("(%s) vs (%s) only", contract.sIssuingaddress, EncodeDestination(address1)));
                }

                // check asset inflation
                if(asset.isInflatable()){
                    if(contract.sIssuingaddress != EncodeDestination(address1))
                        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-inflation-issuer-mismatch", strprintf("(%s) vs (%s) only", contract.sIssuingaddress, EncodeDestination(address1)));

                }
                else
                    return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-ouput-asset-not-inflatable");

            }

            //check asset creation
            if(!exists) {
                //Prevent stakable assets from non dev address
                if(asset.isStakeable())
                    return state.Invalid(TxValidationResult::TX_CONSENSUS, "new-asset-stakable");

                if(inputAssets.begin()->first != subsidy_asset)
                    return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-input-asset", strprintf("found (%s) , expected (%s)", asset.getAssetName(), subsidy_asset.getAssetName()));

                //if(inputAssets[subsidy_asset] < 10 * COIN)
                //    return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-input-amount", strprintf("found (%d) (%s), min (%d) CRW", inputAssets[subsidy_asset], inputAssets.begin()->first.getShortName(), 10));

                if(assetNameExists(asset.getAssetName()) || assetNameExists(asset.getShortName()))
                    return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-name", strprintf("asset name/shortname %s / %s  already in use", asset.getAssetName(), asset.getShortName()));

               // if(asset.nExpiry != 0 && asset.nExpiry < tx.nTime + 84000)// TODO (Why on earth do transactions not have time ?
               //     return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-expiry");

                if(asset.nType == 2){
                    if(asset.isConvertable())
                        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-unique-asset-convertable");
                    if(asset.isInflatable())
                        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-unique-asset-inflatable");
                }

            }
        }
    }

    if(tx.nVersion >= TX_ELE_VERSION){
        if(outputAssets.size() == 1)
            if(inputAssets.begin()->second < outputAssets.begin()->second)
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-in-below-out", strprintf("value in (%s) < value out (%s)", FormatMoney(inputAssets.begin()->second), FormatMoney(outputAssets.begin()->second)));

        // Tally transaction fees
        CAmountMap txfee_aux = inputAssets - outputAssets;

        //identify the fee asset
        CAmount mfee = txfee_aux[inputAssets.begin()->first];
        if (!MoneyRange(mfee)) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-fee-out-of-range");
        }
        txfee = mfee;
    }
    else{
        const CAmount value_out = tx.GetValueOut();
        if (nValueIn < value_out)
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-in-below-out",
            strprintf("value in (%s) < value out (%s)", FormatMoney(nValueIn), FormatMoney(value_out)));

        // Tally transaction fees
        const CAmount txfee_aux = nValueIn - value_out;
        if (!MoneyRange(txfee_aux)) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-fee-out-of-range");
        }
        txfee = txfee_aux;
    }

    return true;
}

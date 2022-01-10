// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <key_io.h>

#include <rpc/protocol.h>
#include <consensus/validation.h>
#include <rpc/server.h>
#include <platform/specialtx.h>
#include <platform/rpc/specialtx-rpc-utils.h>

#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif //ENABLE_WALLET

namespace Platform
{

    // Allows to specify Crown address or priv key. In case of Crown address, the priv key is taken from the wallet
    CKey ParsePrivKeyOrAddress(const std::string & strKeyOrAddress, const std::string & paramName, bool allowAddresses /* = true */)
    {
		CKey key;
        if (allowAddresses)
        {
            key = PullPrivKeyFromWallet(strKeyOrAddress, paramName);
            if (key.IsValid())
                return key;
            else 
            throw std::runtime_error(strprintf("invalid priv-key/address %s", strKeyOrAddress));
        }
        return key;
    }

    CKey GetPrivKeyFromWallet(const CKeyID & keyId)
    {
#ifdef ENABLE_WALLET
        CKey key;
        //if (keyId.IsNull() || !pwalletMain->GetKey(keyId, key))
         //   throw std::runtime_error(strprintf("non-wallet or invalid address %s", keyId.ToString()));
        return key;
#else//ENABLE_WALLET
        throw std::runtime_error("addresses not supported in no-wallet builds");
#endif//ENABLE_WALLET
    }

    CKey PullPrivKeyFromWallet(const std::string & strAddress, const std::string & paramName)
    {
		CTxDestination dest;
		dest = DecodeDestination(strAddress);
		
        if (!IsValidDestination(dest))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid P2PKH address, not %s", paramName, strAddress));

        CKeyID keyId = ToKeyID(std::get<PKHash>(dest));

//#ifdef ENABLE_WALLET
        CKey key;
//        if (!address.GetKeyID(keyId) || !pwalletMain->GetKey(keyId, key))
  //          throw std::runtime_error(strprintf("non-wallet or invalid address %s", strAddress));
        return key;
//#else//ENABLE_WALLET
  //      throw std::runtime_error("addresses not supported in no-wallet builds");
//#endif//ENABLE_WALLET
    }

    CKeyID ParsePubKeyIDFromAddress(const std::string & strAddress, const std::string & paramName)
    {
		CTxDestination dest;
		dest = DecodeDestination(strAddress);
		
        if (!IsValidDestination(dest))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid P2PKH address, not %s", paramName, strAddress));

        CKeyID keyID = ToKeyID(std::get<PKHash>(dest));
        return keyID;
    }

    std::string GetCommand(const UniValue& params, const std::string & errorMessage)
    {
        if (params.empty())
            throw std::runtime_error(errorMessage);

        return params[0].get_str();
    }

    std::string SignAndSendSpecialTx(const CMutableTransaction & tx)
    {
        LOCK(cs_main);
        TxValidationState state;
        if (!CheckNftTx(CTransaction(tx), NULL, state))
        return "";
           // throw std::runtime_error(FormatStateMessage(state));

        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << tx;
        
        UniValue signReqeust(UniValue::VARR);

        signReqeust.push_back(HexStr(ds));
        //json_spirit::Value signResult = signrawtransaction(signReqeust, false);
        //UniValue sendRequest(UniValue::VARR);

        //sendRequest.push_back(find_value(signResult.get_obj(), "hex").get_str());
        std::string r ="";
        return r;
        //return sendrawtransaction(sendRequest, false).get_str();
    }

    bool GetPayerPrivKeyForNftTx(const CMutableTransaction & tx, CKey & payerKey)
    {
        CKeyID payerKeyId;
        if (GetPayerPubKeyIdForNftTx(tx, payerKeyId))
        {
            payerKey = GetPrivKeyFromWallet(payerKeyId);
            return true;
        }
        return false;
    }

    bool GetPayerPubKeyIdForNftTx(const CMutableTransaction & tx, CKeyID & payerKeyId)
    {
        if (tx.vin.empty())
            return false;

        uint256 hashBlockFrom;
        CTransactionRef txFrom = GetTransaction(nullptr, nullptr, tx.vin[0].prevout.hash, Params().GetConsensus(), hashBlockFrom);

        auto txFromOutIdx = tx.vin[0].prevout.n;
        assert(txFromOutIdx < txFrom->vout.size());

        CTxDestination payer;
        if (ExtractDestination(txFrom->vout[txFromOutIdx].scriptPubKey, payer))
        {
			CKeyID keyID = ToKeyID(std::get<PKHash>(payer));
			return !keyID.IsNull();
        }

        return false;
    }
}

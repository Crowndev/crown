// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <hash.h> // for signet block challenge hash
#include <tinyformat.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <versionbitsinfo.h>
#include <arith_uint256.h>
#include <assert.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward, Consensus::Params& consensus)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    AssetMetadata basemeta;
    basemeta.nVersion = 1;
    basemeta.setName("Crown");
    basemeta.setShortName("CRW");
    basemeta.nFlags = AssetMetadata::AssetFlags::ASSET_TRANSFERABLE | AssetMetadata::AssetFlags::ASSET_CONVERTABLE | AssetMetadata::AssetFlags::ASSET_STAKEABLE;
    basemeta.nExpiry =0;
    basemeta.nType =1;
    CAsset baseasset = CAsset(basemeta);

    consensus.subsidy_asset = baseasset;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion.SetGenesisVersion(1);
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward, Consensus::Params& consensus)
{
    const char* pszTimestamp = "The inception of Crowncoin 10/Oct/2014";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward, consensus);
}


void static MineNewGenesisBlock(const Consensus::Params& consensus,CBlock &genesis)
{
    //fPrintToConsole = true;
    std::cout << "Searching for genesis block...\n";
    arith_uint256 hashTarget = UintToArith256(consensus.powLimit);

    while(true) {
        arith_uint256 thash = UintToArith256(genesis.GetHash());
        std::cout << "testHash = " << thash.ToString().c_str() << "\n";
        std::cout << "Hash Target = " << hashTarget.ToString().c_str() << "\n";

      if (thash <= hashTarget)
            break;
        if ((genesis.nNonce & 0xFFF) == 0)
            std::cout << strprintf("nonce %08X: hash = %s (target = %s)\n", genesis.nNonce, thash.ToString().c_str(), hashTarget.ToString().c_str());

        ++genesis.nNonce;
        if (genesis.nNonce == 0) {
            std::cout << ("NONCE WRAPPED, incrementing time\n");
            ++genesis.nTime;
        }
    }
    std::cout << "genesis.nTime = " << genesis.nTime << "\n";
    std::cout << "genesis.nNonce = " << genesis.nNonce<< "\n";
    std::cout << "genesis.nBits = " << genesis.nBits<< "\n";
    std::cout << "genesis.GetHash = " << genesis.GetHash().ToString().c_str()<< "\n";
    std::cout << "genesis.hashMerkleRoot = " << genesis.hashMerkleRoot.ToString().c_str()<< "\n";
//    std::cout << "genesis block \n" << genesis.ToString()<< std::endl;

    exit(1);
}

/**
 * Main network
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = CBaseChainParams::MAIN;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 2100000;
        consensus.BIP16Exception = uint256S("0x00000000000002dc756eebf4f49723ed8d30cc28a5f108eb94b1ba88ac4f9c22");
        consensus.BIP34Height = 227931;
        consensus.BIP34Hash = uint256S("0x000000000000024b89b42a942fe0d9fea3bb44ab7bd1b19115dd6a759c0808b8");
        consensus.BIP65Height = 388381; // 000000000000000004c2b624ed5d7756c508d90fd0da2c7c679febfa6c4735f0
        consensus.BIP66Height = 363725; // 00000000000000000379eaa19dce8c9b722d46ae6a57c2f1a988119488b50931
        consensus.CSVHeight = 3700000; // 000000000000000004a1b34462cb8aeebd5799177f7a29cf28f2d1961716b5b5
        consensus.SegwitHeight = 3700000; // 0000000000000000001c8018d9cb3b742ef25114f27563e3fc4a1902167f9893
        consensus.MinBIP9WarningHeight = 483840; // segwit activation height + miner confirmation window
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 1 * 60; // Crown: 1 minute
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Deployment of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = 1619222400; // April 24th, 2021
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = 1628640000; // August 11th, 2021
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 709632; // Approximately November 12th, 2021

        consensus.nMinimumChainWork = uint256S("0x00");
        consensus.defaultAssumeValid = uint256S("0x00");

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xb8;
        pchMessageStart[1] = 0xeb;
        pchMessageStart[2] = 0xb3;
        pchMessageStart[3] = 0xdf;
        nDefaultPort = 9340;
        nBlockPoSStart = 2330000;
        nPruneAfterHeight = 100000;
        nAuxpowChainId = 20;
        nChainStallDuration = 60*60;
        m_assumed_blockchain_size = 350;
        m_assumed_chain_state_size = 6;
        nMasternodeCollateral = 10000 * COIN;
        nSystemnodeCollateral = 500 * COIN;
        nStartMasternodePayments = 1403728576;
        nStakePointerValidityPeriod = 4320; //Stake pointers are valid to stake with for the next 3 day worth of blocks
        nMaxReorgDepth = 100;
        nKernelModifierOffset = 100;

        std::vector<unsigned char> addrdata(ParseHex("0014848a4a37883c295bc1dc021941e0f185fafc1aae"));
        CScript genscript(addrdata.begin(), addrdata.end());
        consensus.mandatory_coinbase_destination = genscript; 

        genesis = CreateGenesisBlock(1412760826, 1612467894, 0x1d00ffff, 1, 10 * COIN, consensus);

        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x0000000085370d5e122f64f4ab19c68614ff3df78c8d13cb814fd7e69a1dc6da"));
        assert(genesis.hashMerkleRoot == uint256S("0x80ad356118a9ab8db192db66ef77146cc36d958f959251feace550e4ca3d1446"));

        // Note that of those which support the service bits prefix, most only support a subset of
        // possible options.
        // This is fine at runtime as we'll fall back to using them as an addrfetch if they don't support the
        // service bits we want, but we should get them updated to support all service bits wanted by any
        // release ASAP to avoid it where possible.
        /*
        vSeeds.emplace_back("canada-01seedns.crowncoin.org");
        vSeeds.emplace_back("latam-01seedns.crowncoin.net");
        vSeeds.emplace_back("SEAsia-01seedns.crowncoin.org");
        vSeeds.emplace_back("pacific-01seedns.crowncoin.net");
*/
        base58Prefixes[PUBKEY_ADDRESS] = {0x01, 0x75, 0x07};
        base58Prefixes[SCRIPT_ADDRESS] = {0x01, 0x74, 0xF1};
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "crw";

        nFulfilledRequestExpireTime = 60*60;

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));


        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = false;
        m_is_mockable_chain = false;
        strSporkKey = "0440409BDACDCE03BFB6D5F16E2D414953038996B49BEE6697CFA400A0001D0837C885C5B57DAD10E5CAAAE36EE975005CC6CBD7001A2A8DE76FF12185904A9BB1";
        strDevfundAddress = "16tg5tuZrPKoBwfbmj2tmiEPhVPzyn3gtP";
        strLegacySignerDummyAddress = "18WTcWvwrNnfqeQAn6th9QQ2EpnXMq5Th8";
        checkpointData = {
            {
                { 150000, uint256S("0000000000012276edd1465babe6d059a3b65b2503b6d9bb4ae5839f5eabac06")},
                { 300000, uint256S("00000000000059901c2ea32bc486d91a355a8fe362e34fc3d10c45bb5e5ca79d")},
                { 450000, uint256S("0000000000001db791c8581be592b13f756913962b73514a3a636f7a1637a18f")},
                { 600000, uint256S("84e2277d1dc957ae41869498311937bdcedce7e48fa33f962b9b9e9c16df5410")},
                { 750000, uint256S("ac8bf6801d4e51201993d4c623f2e72000781087f508d60d4f307d7928b912d1")},
                { 900000, uint256S("bdf0bcfe3ada671b64526854af8cb7f6f52c1489446e26268e70fbee1e72be5f")},
                {1050000, uint256S("15c897620ab3ee017262add6a49df0510897498f71ec1bc251e3849bb44b633d")},
                {1200000, uint256S("0000000000192aecb746b879d868c75c075d8e061bef466b3393b88062fcb362")},
                {1350000, uint256S("af5001f0cbc13e47c7d3c6cea6f0db8dc92a03688031f3ac330c43f5b29f46b3")},
                {1500000, uint256S("278a333b28cbfa557362ab77e83815ee203cadd79d76a7a92be1782e6e70a356")},
                {1650000, uint256S("31885f425a43ea15804907001affd696d28d3e7b991dcadc937e71f52c09b6a7")},
                {1800000, uint256S("9ad4a139ff2e034bafd9647cae7651a34d860afdc483925293c9b7933880a959")},
                {1950000, uint256S("f1cdba6a857e230ffba2ac7ff6dfab66172255840f792525b758ca7a6bb1e191")},
                {2100000, uint256S("4657b72edb700d09c6a86429fdfa668a044441b7d68b5090a450363ec6b44395")},
                {2250000, uint256S("3eb75e85795321bbffcf1781f86fd3135fdc6a14efc71b55e41149ed2048d87d")},
                {2400000, uint256S("5aa66e017e798454a4b9774bf6c478047e7db0c405b171e6dff2cde9fb8cdd68")},
                {2550000, uint256S("09ba5b0cfb8aece2c08fc48c4dfc56af7a3d6e811ebf11de85e898894bd319f4")},
                {2700000, uint256S("81f44f3061a8a61e5729b85522176a931bea90bb27d638bf892c02373c4b554d")},
                {2850000, uint256S("c54e006c95c5412518dbd62bad1a51b308e70a0637e97bbcc53ee35b77de07a8")},
                {3000000, uint256S("d27d72dbb83ba7662ad1a022a61ae484e3853a336cd55578dfea3144470344da")},
                {3150000, uint256S("be94e6f027f0606739b8b7351989c06030313ae9978d8b4f56488dc75d3ec528")},
            }
        };

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 4096 0000000000000000000b9d2ec5a352ecba0592946514a92f14319dc2b367fc72
            /* nTime    */ 1532884444,
            /* nTxCount */ 331282217,
            /* dTxRate  */ 2.4
        };
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = CBaseChainParams::TESTNET;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 130000;
        consensus.BIP16Exception = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
        consensus.BIP34Height = std::numeric_limits<int>::max();
        consensus.BIP34Hash = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
        consensus.BIP65Height = std::numeric_limits<int>::max();
        consensus.BIP66Height = std::numeric_limits<int>::max();
        consensus.CSVHeight = 1; // 00000000025e930139bac5c6c31a403776da130831ab85be56578f3fa75369bb
        consensus.SegwitHeight = std::numeric_limits<int>::max(); // 00000000002b980fcd729daaa248fd9316a5200e9b367f4ff2c42453e84201ca
        consensus.MinBIP9WarningHeight = std::numeric_limits<int>::max();
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 60; // two weeks
        consensus.nPowTargetSpacing = 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Deployment of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = 1619222400; // April 24th, 2021
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = 1628640000; // August 11th, 2021
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        consensus.nMinimumChainWork = uint256S("0x00");
        consensus.defaultAssumeValid = uint256S("0x00"); // 1864000

        pchMessageStart[0] = 0x0b;
        pchMessageStart[1] = 0x11;
        pchMessageStart[2] = 0x09;
        pchMessageStart[3] = 0x07;
        nDefaultPort = 18333;
        nBlockPoSStart = 5000;
        nPruneAfterHeight = 100000;
        nAuxpowChainId = 20;
        nChainStallDuration = 60*60;
        nMasternodeCollateral = 10000 * COIN;
        nSystemnodeCollateral = 500 * COIN;
        nStartMasternodePayments = 1403728576;
        nStakePointerValidityPeriod = 4320; //Stake pointers are valid to stake with for the next 3 day worth of blocks
        nMaxReorgDepth = 100;
        nKernelModifierOffset = 100;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 1;
        m_assumed_chain_state_size = 1;

        std::vector<unsigned char> addrdata(ParseHex("0014848a4a37883c295bc1dc021941e0f185fafc1aae"));
        CScript genscript(addrdata.begin(), addrdata.end());
        consensus.mandatory_coinbase_destination = genscript; 

        genesis = CreateGenesisBlock(1659429370, 551064, 0x1e0ffff0, 1, 10 * COIN, consensus);
        consensus.hashGenesisBlock = genesis.GetHash();
        //MineNewGenesisBlock(consensus,genesis);
        assert(consensus.hashGenesisBlock == uint256S("0x000003e4d706ae2480a9b269042d8c9c493a216a328103a4bdb9656b93818307"));
        assert(genesis.hashMerkleRoot == uint256S("0x80ad356118a9ab8db192db66ef77146cc36d958f959251feace550e4ca3d1446"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("92.60.46.24");
        vSeeds.emplace_back("92.60.46.25");
        vSeeds.emplace_back("92.60.46.26");
        vSeeds.emplace_back("92.60.46.27");
        vSeeds.emplace_back("92.60.46.28");
        vSeeds.emplace_back("92.60.46.29");
        vSeeds.emplace_back("92.60.46.30");
        vSeeds.emplace_back("92.60.46.31");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tcr";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;
        m_is_mockable_chain = false;
        strSporkKey = "04fb22e1ca5304b7dd1e36dfdd03b61b82454573a02a86d875d98da840a33e86dc4709baf8aa2c45ccfcf31027451f6831dd080978fc90dbf411bf12fb15b60d7b";
        strDevfundAddress = "mr59c3aniaN3qHXej5L8UBsssRZbiUUMnz";
        strLegacySignerDummyAddress = "mr59c3aniaN3qHXej5L8UBsssRZbiUUMnz";
        checkpointData = {
            {
                {546, uint256S("000000002a936ca763904c3c35fce2f3556c559c0214345d31b1bcebf76acb70")},
            }
        };

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 4096 000000000000006433d1efec504c53ca332b64963c425395515b01977bd7b3b0
            /* nTime    */ 1603359686,
            /* nTxCount */ 58090238,
            /* dTxRate  */ 0.1232886622799463,
        };
    }
};

/**
 * Signet
 */
class SigNetParams : public CChainParams {
public:
    explicit SigNetParams(const ArgsManager& args) {
        std::vector<uint8_t> bin;
        vSeeds.clear();

        if (!args.IsArgSet("-signetchallenge")) {
            bin = ParseHex("512103ad5e0edad18cb1f0fc0d28a3d4f1f3e445640337489abb10404f2d1e086be430210359ef5021964fe22d6f8e05b2463c9540ce96883fe3b278760f048f5189f2e6c452ae");
            vSeeds.emplace_back("178.128.221.177");
            vSeeds.emplace_back("2a01:7c8:d005:390::5");
            vSeeds.emplace_back("v7ajjeirttkbnt32wpy3c6w3emwnfr3fkla7hpxcfokr3ysd3kqtzmqd.onion:38333");

            consensus.nMinimumChainWork = uint256S("0x00");
            consensus.defaultAssumeValid = uint256S("0x00"); // 9434
            m_assumed_blockchain_size = 1;
            m_assumed_chain_state_size = 0;
            chainTxData = ChainTxData{
                // Data from RPC: getchaintxstats 4096 0000002a1de0f46379358c1fd09906f7ac59adf3712323ed90eb59e4c183c020
                /* nTime    */ 1603986000,
                /* nTxCount */ 9582,
                /* dTxRate  */ 0.00159272030651341,
            };
        } else {
            const auto signet_challenge = args.GetArgs("-signetchallenge");
            if (signet_challenge.size() != 1) {
                throw std::runtime_error(strprintf("%s: -signetchallenge cannot be multiple values.", __func__));
            }
            bin = ParseHex(signet_challenge[0]);

            consensus.nMinimumChainWork = uint256{};
            consensus.defaultAssumeValid = uint256{};
            m_assumed_blockchain_size = 0;
            m_assumed_chain_state_size = 0;
            chainTxData = ChainTxData{
                0,
                0,
                0,
            };
            LogPrintf("Signet with challenge %s\n", signet_challenge[0]);
        }

        if (args.IsArgSet("-signetseednode")) {
            vSeeds = args.GetArgs("-signetseednode");
        }

        strNetworkID = CBaseChainParams::SIGNET;
        consensus.signet_blocks = true;
        consensus.signet_challenge.assign(bin.begin(), bin.end());
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Exception = uint256{};
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256{};
        consensus.BIP65Height = 1;
        consensus.BIP66Height = 1;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 1;
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1815; // 90% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("00000377ae000000000000000000000000000000000000000000000000000000");
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Activation of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // message start is defined as the first 4 bytes of the sha256d of the block script
        CHashWriter h(SER_DISK, 0);
        h << consensus.signet_challenge;
        uint256 hash = h.GetHash();
        memcpy(pchMessageStart, hash.begin(), 4);

        nDefaultPort = 38333;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1598918400, 52613770, 0x1e0377ae, 1, 50 * COIN, consensus);
        consensus.hashGenesisBlock = genesis.GetHash();
        //assert(consensus.hashGenesisBlock == uint256S("0x00000008819873e925422c1ff0f99f7cc9bbb232af63a077a480a3633bee1ef6"));
        //assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        vFixedSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tb";

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = false;
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ArgsManager& args) {
        strNetworkID =  CBaseChainParams::REGTEST;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 150;
        consensus.BIP16Exception = uint256();
        consensus.BIP34Height = 500; // BIP34 activated on regtest (Used in functional tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in functional tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in functional tests)
        consensus.CSVHeight = 432; // CSV activated on regtest (Used in rpc activation tests)
        consensus.SegwitHeight = 0; // SEGWIT is always activated on regtest unless overridden
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        consensus.nMinimumChainWork = uint256{};
        consensus.defaultAssumeValid = uint256{};

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateActivationParametersFromArgs(args);

        genesis = CreateGenesisBlock(1296688602, 2, 0x207fffff, 1, 50 * COIN, consensus);
        consensus.hashGenesisBlock = genesis.GetHash();
        //assert(consensus.hashGenesisBlock == uint256S("0x0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"));
        //assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = true;

        checkpointData = {
            {
                {0, uint256S("0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206")},
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "bcrt";
    }

    /**
     * Allows modifying the Version Bits regtest parameters.
     */
    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout, int min_activation_height)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
        consensus.vDeployments[d].min_activation_height = min_activation_height;
    }
    void UpdateActivationParametersFromArgs(const ArgsManager& args);
};

void CRegTestParams::UpdateActivationParametersFromArgs(const ArgsManager& args)
{
    if (args.IsArgSet("-segwitheight")) {
        int64_t height = args.GetArg("-segwitheight", consensus.SegwitHeight);
        if (height < -1 || height >= std::numeric_limits<int>::max()) {
            throw std::runtime_error(strprintf("Activation height %ld for segwit is out of valid range. Use -1 to disable segwit.", height));
        } else if (height == -1) {
            LogPrintf("Segwit disabled for testing\n");
            height = std::numeric_limits<int>::max();
        }
        consensus.SegwitHeight = static_cast<int>(height);
    }

    if (!args.IsArgSet("-vbparams")) return;

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams;
        boost::split(vDeploymentParams, strDeployment, boost::is_any_of(":"));
        if (vDeploymentParams.size() < 3 || 4 < vDeploymentParams.size()) {
            throw std::runtime_error("Version bits parameters malformed, expecting deployment:start:end[:min_activation_height]");
        }
        int64_t nStartTime, nTimeout;
        int min_activation_height = 0;
        if (!ParseInt64(vDeploymentParams[1], &nStartTime)) {
            throw std::runtime_error(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
        }
        if (!ParseInt64(vDeploymentParams[2], &nTimeout)) {
            throw std::runtime_error(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
        }
        if (vDeploymentParams.size() >= 4 && !ParseInt32(vDeploymentParams[3], &min_activation_height)) {
            throw std::runtime_error(strprintf("Invalid min_activation_height (%s)", vDeploymentParams[3]));
        }
        bool found = false;
        for (int j=0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
            if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].name) {
                UpdateVersionBitsParameters(Consensus::DeploymentPos(j), nStartTime, nTimeout, min_activation_height);
                found = true;
                LogPrintf("Setting version bits activation parameters for %s to start=%ld, timeout=%ld, min_activation_height=%d\n", vDeploymentParams[0], nStartTime, nTimeout, min_activation_height);
                break;
            }
        }
        if (!found) {
            throw std::runtime_error(strprintf("Invalid deployment (%s)", vDeploymentParams[0]));
        }
    }
}

static std::unique_ptr<CChainParams> globalChainParams;

CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const ArgsManager& args, const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN) {
        return std::unique_ptr<CChainParams>(new CMainParams());
    } else if (chain == CBaseChainParams::TESTNET) {
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    } else if (chain == CBaseChainParams::SIGNET) {
        return std::unique_ptr<CChainParams>(new SigNetParams(args));
    } else if (chain == CBaseChainParams::REGTEST) {
        return std::unique_ptr<CChainParams>(new CRegTestParams(args));
    }
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(gArgs, network);
}

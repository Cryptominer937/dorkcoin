// Copyright (c) 2026 The Dorkcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cashaddr.h>
#include <config/bitcoin-config.h>
#include <clientversion.h>
#include <chainparams.h>
#include <chainparamsbase.h>
#include <common/args.h>
#include <consensus/merkle.h>
#include <deploymentstatus.h>
#include <key_io.h>
#include <netmessagemaker.h>
#include <node/protocol_version.h>
#include <pow.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <validation.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>
#include <limits>

BOOST_FIXTURE_TEST_SUITE(dorkcoin_consensus_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(software_identity)
{
    BOOST_CHECK_EQUAL(PACKAGE_NAME, "Dork Core");
    BOOST_CHECK_EQUAL(PACKAGE_URL, "https://dorkcore.org");
    BOOST_CHECK_EQUAL(PACKAGE_BUGREPORT, "https://github.com/dork-core/dorkcoin/issues");
    BOOST_CHECK_EQUAL(CLIENT_NAME, "DorkCore");
    BOOST_CHECK_EQUAL(FormatSubVersion(CLIENT_NAME, CLIENT_VERSION, {}), "/DorkCore:28.4.0/");
    BOOST_CHECK_EQUAL(BITCOIN_CONF_FILENAME, "dorkcoin.conf");
    BOOST_CHECK(LicenseInfo().find("The Dorkcoin Developers") != std::string::npos);
    BOOST_CHECK(LicenseInfo().find("The Blackcoin Developers") != std::string::npos);
    const auto gui_license = LicenseInfo(false);
    BOOST_CHECK(gui_license.find("The Dorkcoin Developers") != std::string::npos);
    BOOST_CHECK(gui_license.find("The Bitcoin Core Developers") != std::string::npos);
    BOOST_CHECK(gui_license.find("Blackcoin") == std::string::npos);
    BOOST_CHECK(gui_license.find("<https://dorkcore.org>") != std::string::npos);
    BOOST_CHECK(gui_license.find("<https://github.com/dork-core/dorkcoin>") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(network_identity)
{
    const auto main = CChainParams::Main();
    const auto test = CChainParams::TestNet();
    const auto reg = CChainParams::RegTest({});
    BOOST_CHECK_EQUAL(main->GenesisBlock().GetHash().ToString(), "000007e104c49bac76127806f96d50f2510fe42c0c47126d511cea785248c1ef");
    BOOST_CHECK_EQUAL(test->GenesisBlock().GetHash().ToString(), "0000d14e23132387cd7d0eece87ad8a04469516f465e69b0679f4e12ee63f1e6");
    BOOST_CHECK(test->GenesisBlock().GetHash() == reg->GenesisBlock().GetHash());
    for (const auto* params : {main.get(), test.get(), reg.get()}) {
        const auto& consensus = params->GetConsensus();
        BOOST_CHECK_EQUAL(BlockMerkleRoot(params->GenesisBlock()).ToString(), "75bc1a80e13213ada9e18dac132c3c8d3c03ff3d1bcc7aec73f5f35155b8518a");
        BOOST_CHECK_EQUAL(consensus.posLimitV2.ToString(), "00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        BOOST_CHECK_EQUAL(GetBlockSubsidy(1, consensus, false), 100 * COIN);
        BOOST_CHECK_EQUAL(GetBlockSubsidy(10001, consensus, true), 100 * COIN);
        BOOST_CHECK_EQUAL(consensus.nTargetSpacing, 60);
        BOOST_CHECK_EQUAL(consensus.nStakeTimestampMask, 15);
        BOOST_CHECK(!DeploymentEnabled(consensus, Consensus::DEPLOYMENT_SEGWIT));
        BOOST_CHECK(!DeploymentEnabled(consensus, Consensus::DEPLOYMENT_TAPROOT));
        BOOST_CHECK_EQUAL(params->Base58Prefix(CChainParams::PUBKEY_ADDRESS).at(0), 30);
        BOOST_CHECK_EQUAL(params->Base58Prefix(CChainParams::SCRIPT_ADDRESS).at(0), 31);
    }
    BOOST_CHECK_EQUAL(main->GetDefaultPort(), 25516);
    BOOST_CHECK_EQUAL(test->GetDefaultPort(), 35516);
    BOOST_CHECK_EQUAL(reg->GetDefaultPort(), 35714);
    BOOST_CHECK_EQUAL(CreateBaseChainParams(ChainType::MAIN)->RPCPort(), 25515);
    BOOST_CHECK_EQUAL(CreateBaseChainParams(ChainType::TESTNET)->RPCPort(), 35515);
    BOOST_CHECK_EQUAL(CreateBaseChainParams(ChainType::REGTEST)->RPCPort(), 35715);
    BOOST_CHECK(main->MessageStart() == (MessageStartChars{0x22, 0x07, 0x5c, 0x42}));
    BOOST_CHECK(test->MessageStart() == (MessageStartChars{0xe6, 0x72, 0x1b, 0x89}));
    BOOST_CHECK(reg->MessageStart() == (MessageStartChars{0x22, 0x07, 0x5c, 0x06}));
    BOOST_CHECK_EQUAL(main->GetConsensus().nLastPOWBlock, 10000);
    BOOST_CHECK_EQUAL(test->GetConsensus().nLastPOWBlock, std::numeric_limits<int>::max());
    BOOST_CHECK_EQUAL(reg->GetConsensus().nLastPOWBlock, 1000);
    BOOST_CHECK_EQUAL(main->GetConsensus().nCoinbaseMaturity, 100);
    BOOST_CHECK_EQUAL(test->GetConsensus().nCoinbaseMaturity, 10);
    BOOST_CHECK_EQUAL(reg->GetConsensus().nCoinbaseMaturity, 10);
    BOOST_CHECK_EQUAL(main->GetBurnAddress(), "DBURN3nLyUMCRZbcZ3ENinyqUQYLsF5K7R");
    BOOST_CHECK_EQUAL(test->GetBurnAddress(), "DBURN3nLyUMCRZbcZ3ENinyqUQYLsF5K7R");
    BOOST_CHECK_EQUAL(reg->GetBurnAddress(), "DBURN3nLyUMCRZbcZ3ENinyqUQYLsF5K7R");
    BOOST_CHECK(IsValidDestinationString(main->GetBurnAddress(), *main));
    BOOST_CHECK(IsValidDestinationString(test->GetBurnAddress(), *test));
    BOOST_CHECK(IsValidDestinationString(reg->GetBurnAddress(), *reg));
    BOOST_CHECK_EQUAL(main->DNSSeeds().at(0), "213.181.99.70");
    BOOST_CHECK_EQUAL(main->DNSSeeds().at(1), "216.128.144.174");
    BOOST_CHECK(main->FixedSeeds().empty());
    BOOST_CHECK_EQUAL(reg->Checkpoints().GetHeight(), 0);
}

BOOST_AUTO_TEST_CASE(timestamp_activation_boundaries)
{
    const auto params = CChainParams::TestNet();
    const auto& consensus = params->GetConsensus();
    BOOST_CHECK(!consensus.IsProtocolV1RetargetingFixed(1773037267));
    BOOST_CHECK(consensus.IsProtocolV1RetargetingFixed(1773037268));
    BOOST_CHECK(!consensus.IsProtocolV2(1773037268));
    BOOST_CHECK(consensus.IsProtocolV2(1773037269));
    BOOST_CHECK(!consensus.IsProtocolV3(1773037269));
    BOOST_CHECK(consensus.IsProtocolV3(1773037270));
    CBlockIndex index;
    VersionBitsCache cache;
    index.nTime = 1773037270;
    BOOST_CHECK(!DeploymentActiveAt(index, consensus, Consensus::DEPLOYMENT_CSV, cache));
    ++index.nTime;
    BOOST_CHECK(DeploymentActiveAt(index, consensus, Consensus::DEPLOYMENT_CSV, cache));
}

BOOST_AUTO_TEST_CASE(segwit_height_boundaries)
{
    CChainParams::RegTestOptions options;
    options.activation_heights[Consensus::DEPLOYMENT_SEGWIT] = 12;
    const auto params = CChainParams::RegTest(options);
    const auto& consensus = params->GetConsensus();
    VersionBitsCache cache;
    CBlockIndex index;
    BOOST_CHECK(DeploymentEnabled(consensus, Consensus::DEPLOYMENT_SEGWIT));
    BOOST_CHECK(!DeploymentEnabled(consensus, Consensus::DEPLOYMENT_TAPROOT));
    BOOST_CHECK(!DeploymentActiveAfter(nullptr, consensus, Consensus::DEPLOYMENT_SEGWIT, cache));
    for (int height : {10, 11, 12, 13, 11, 10}) {
        index.nHeight = height;
        BOOST_CHECK_EQUAL(DeploymentActiveAt(index, consensus, Consensus::DEPLOYMENT_SEGWIT, cache), height >= 12);
        BOOST_CHECK_EQUAL(DeploymentActiveAfter(&index, consensus, Consensus::DEPLOYMENT_SEGWIT, cache), height + 1 >= 12);
    }
    for (const ChainType chain : {ChainType::MAIN, ChainType::TESTNET, ChainType::REGTEST}) {
        SelectParams(chain);
        const CTxDestination destination{WitnessV0KeyHash{uint160{}}};
        const auto address = EncodeDestination(destination);
        BOOST_CHECK(address.starts_with(Params().Bech32HRP() + "1q"));
        BOOST_CHECK(DecodeDestination(address) == destination);
    }
}

BOOST_AUTO_TEST_CASE(legacy_wire_header)
{
    const auto params = CChainParams::Main();
    auto header = params->GenesisBlock().GetBlockHeader();
    header.nFlags = 1;
    const auto message = NetMsg::Make("block", header);
    BOOST_CHECK_EQUAL(message.data.size(), 80);
    CDataStream stream(message.data, SER_NETWORK);
    CBlockHeader decoded;
    stream >> decoded;
    BOOST_CHECK(stream.empty());
    BOOST_CHECK(decoded.GetHash() == header.GetHash());
    BOOST_CHECK_EQUAL(decoded.nFlags, 0);
    BOOST_CHECK_LE(MIN_PEER_PROTO_VERSION, 70015);
}

BOOST_AUTO_TEST_CASE(legacy_signed_transaction_versions)
{
    for (const int32_t version : {-1, 0, 1, 2}) {
        CMutableTransaction original;
        original.version = version;
        original.nTime = 1773037267;
        original.vin.resize(1);
        original.vout.resize(1);
        CDataStream stream(SER_NETWORK);
        stream << TX_NO_WITNESS(original);
        const auto size = stream.size();
        CMutableTransaction decoded;
        stream >> TX_NO_WITNESS(decoded);
        BOOST_CHECK(stream.empty());
        BOOST_CHECK_EQUAL(decoded.version, version);
        BOOST_CHECK_EQUAL(decoded.nTime, version < 2 ? original.nTime : 0);
        BOOST_CHECK_EQUAL(size, version < 2 ? 64 : 60);
        BOOST_CHECK(CTransaction(original).GetHash() == CTransaction(decoded).GetHash());
    }
}

BOOST_AUTO_TEST_CASE(cashaddr_legacy_compatibility)
{
    SelectParams(ChainType::MAIN);
    const uint160 hash{ParseHex("76a04053bda0a88bda5177b86a15c3b29f559873")};
    gArgs.ForceSetArg("-usecashaddr", "1");
    BOOST_CHECK_EQUAL(EncodeDestination(PKHash{hash}), "dorkcoin:qpm2qsznhks23z7629mms6s4cwef74vcwvvd0yqt50");
    BOOST_CHECK_EQUAL(EncodeDestination(ScriptHash{hash}), "dorkcoin:ppm2qsznhks23z7629mms6s4cwef74vcwvmgjt8g0j");
    for (const ChainType chain : {ChainType::MAIN, ChainType::TESTNET, ChainType::REGTEST}) {
        SelectParams(chain);
        for (const CTxDestination destination : {
                 CTxDestination{PKHash{uint160{}}},
                 CTxDestination{ScriptHash{uint160{}}}}) {
            gArgs.ForceSetArg("-usecashaddr", "0");
            const auto base58 = EncodeDestination(destination);
            BOOST_CHECK(DecodeDestination(base58) == destination);
            gArgs.ForceSetArg("-usecashaddr", "1");
            const auto encoded = EncodeDestination(destination);
            BOOST_CHECK(encoded.starts_with(Params().CashAddrPrefix() + ":"));
            BOOST_CHECK(DecodeDestination(encoded) == destination);
            BOOST_CHECK(DecodeDestination(encoded.substr(encoded.find(':') + 1)) == destination);
            auto corrupt = encoded;
            corrupt.back() = corrupt.back() == 'q' ? 'p' : 'q';
            BOOST_CHECK(!IsValidDestinationString(corrupt));
        }
    }
    gArgs.ForceSetArg("-usecashaddr", "0");
}

BOOST_AUTO_TEST_CASE(legacy_initial_targets)
{
    for (const auto chain : {ChainType::MAIN, ChainType::TESTNET, ChainType::REGTEST}) {
        const auto params = CreateChainParams(gArgs, chain);
        CBlockIndex genesis{params->GenesisBlock()};
        const auto& consensus = params->GetConsensus();
        BOOST_CHECK_EQUAL(GetNextTargetRequired(&genesis, consensus, false), chain == ChainType::MAIN ? 0x1e0fffffU : 0x1f00ffffU);
        BOOST_CHECK_EQUAL(GetNextTargetRequired(&genesis, consensus, true), 0x1e0fffffU);
        BOOST_CHECK(CheckProofOfWork(params->GenesisBlock().GetPoWHash(), params->GenesisBlock().nBits, consensus));
        BOOST_CHECK(!CheckProofOfWork(params->GenesisBlock().GetPoWHash(), 0x207fffff, consensus));
    }
}

BOOST_AUTO_TEST_SUITE_END()

struct DorkcoinSegwitSetup : TestingSetup {
    DorkcoinSegwitSetup()
        : TestingSetup{ChainType::REGTEST, TestOpts{.extra_args = {"-testactivationheight=segwit@12"}}} {}
};

BOOST_FIXTURE_TEST_SUITE(dorkcoin_segwit_tests, DorkcoinSegwitSetup)

BOOST_AUTO_TEST_CASE(pow_pos_commitments)
{
    auto& chainman = *m_node.chainman;
    for (const bool proof_of_stake : {false, true}) {
        CMutableTransaction coinbase;
        coinbase.nTime = 1773037280;
        coinbase.vin.resize(1);
        coinbase.vin[0].scriptSig = CScript() << 12 << OP_0;
        coinbase.vout.emplace_back(100 * COIN, CScript() << OP_TRUE);
        if (proof_of_stake) coinbase.vout[0].SetEmpty();
        CBlock block;
        block.nVersion = VERSIONBITS_TOP_BITS;
        block.nTime = 1773037280;
        block.nBits = proof_of_stake ? 0x1e0fffff : 0x1f00ffff;
        block.vtx.push_back(MakeTransactionRef(coinbase));
        if (proof_of_stake) {
            CMutableTransaction coinstake;
            coinstake.nTime = block.nTime;
            coinstake.vin.emplace_back(COutPoint{Params().GenesisBlock().vtx[0]->GetHash(), 0});
            coinstake.vout.resize(1);
            coinstake.vout[0].SetEmpty();
            coinstake.vout.emplace_back(100 * COIN, CScript() << OP_TRUE);
            block.vtx.push_back(MakeTransactionRef(coinstake));
        }
        CBlockIndex previous;
        previous.nHeight = 10;
        BOOST_CHECK(chainman.GenerateCoinbaseCommitment(block, &previous).empty());
        BOOST_CHECK_EQUAL(block.vtx[0]->vout.size(), 1);
        BOOST_CHECK(!block.vtx[0]->HasWitness());
        previous.nHeight = 11;
        BOOST_CHECK(!chainman.GenerateCoinbaseCommitment(block, &previous).empty());
        BOOST_CHECK_EQUAL(block.vtx[0]->vout.size(), 2);
        BOOST_CHECK(block.vtx[0]->HasWitness());
        block.hashMerkleRoot = BlockMerkleRoot(block);
        BOOST_CHECK(!IsBlockMutated(block, /*check_witness_root=*/true));
        BlockValidationState state;
        BOOST_CHECK_MESSAGE(CheckBlock(block, state, chainman.GetConsensus(), chainman.ActiveChainstate(),
                                       /*fCheckPOW=*/false, /*fCheckMerkleRoot=*/false, /*fCheckSig=*/false),
                            state.ToString());
        CMutableTransaction mutated{*block.vtx[0]};
        mutated.vin[0].scriptWitness.stack[0][0] ^= 1;
        block.vtx[0] = MakeTransactionRef(mutated);
        block.m_checked_witness_commitment = false;
        BOOST_CHECK(IsBlockMutated(block, /*check_witness_root=*/true));
    }
}

BOOST_AUTO_TEST_SUITE_END()

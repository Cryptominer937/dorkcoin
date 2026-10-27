// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2024 The Bitcoin Core developers
// Copyright (c) 2026 The Dorkcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/chainparams.h>

#include <consensus/merkle.h>
#include <primitives/transaction.h>
#include <script/script.h>

#include <algorithm>
#include <cassert>
#include <limits>
#include <stdexcept>

namespace {
// Public-network activation heights require a coordinated release, not a local override.
constexpr int MAINNET_SEGWIT_HEIGHT{std::numeric_limits<int>::max()};
constexpr int TESTNET_SEGWIT_HEIGHT{std::numeric_limits<int>::max()};

CBlock CreateGenesisBlock(uint32_t nonce, uint32_t bits)
{
    const std::string timestamp{"Real Dorks Cryptocurrency"};
    CMutableTransaction tx;
    tx.version = 1;
    tx.nTime = 1773037267;
    tx.vin.resize(1);
    tx.vout.resize(1);
    tx.vin[0].scriptSig = CScript() << 0 << CScriptNum(42)
        << std::vector<unsigned char>(timestamp.begin(), timestamp.end());
    tx.vout[0].nValue = 0;

    CBlock block;
    block.nTime = tx.nTime;
    block.nBits = bits;
    block.nNonce = nonce;
    block.nVersion = 1;
    block.vtx.push_back(MakeTransactionRef(std::move(tx)));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    return block;
}

class DorkcoinParams final : public CChainParams
{
public:
    explicit DorkcoinParams(ChainType chain, const RegTestOptions& opts = {})
    {
        const bool main = chain == ChainType::MAIN;
        const bool regtest = chain == ChainType::REGTEST;
        m_chain_type = chain;
        consensus = {};
        consensus.nMaxReorganizationDepth = regtest ? 50 : 500;
        // CSV is timestamp-activated by protocol v3.1, not at a buried height.
        consensus.CSVHeight = std::numeric_limits<int>::max();
        consensus.SegwitHeight = main ? MAINNET_SEGWIT_HEIGHT : TESTNET_SEGWIT_HEIGHT;
        if (regtest) consensus.SegwitHeight = std::numeric_limits<int>::max();
        consensus.MinBIP9WarningHeight = std::numeric_limits<int>::max();
        consensus.powLimit = main
            ? uint256{"00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"}
            : uint256{"0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"};
        consensus.posLimit = uint256{"00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"};
        consensus.posLimitV2 = consensus.posLimit;
        consensus.nTargetTimespan = 60;
        consensus.nTargetSpacingV1 = regtest ? 64 : 60;
        consensus.nTargetSpacing = 60;
        consensus.fPowAllowMinDifficultyBlocks = !main;
        consensus.enforce_BIP94 = false;
        consensus.fPowNoRetargeting = regtest;
        consensus.fPoSNoRetargeting = regtest;
        consensus.nRuleChangeActivationThreshold = regtest ? 108 : 1;
        consensus.nMinerConfirmationWindow = regtest ? 144 : 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.nProtocolV1RetargetingFixedTime = 1773037267;
        consensus.nProtocolV2Time = 1773037268;
        consensus.nProtocolV3Time = 1773037269;
        consensus.nProtocolV3_1Time = chain == ChainType::TESTNET ? 1773037270 : 1713938400;
        consensus.nLastPOWBlock = main ? 10000 : (regtest ? 1000 : 0x7fffffff);
        consensus.nStakeTimestampMask = 0xf;
        consensus.nCoinbaseMaturity = main ? 100 : 10;
        consensus.nMinimumChainWork = regtest ? uint256{} :
            uint256{"0000000000000000000000000000000000000000000000000000000000100001"};
        consensus.defaultAssumeValid = uint256{};

        pchMessageStart = main ? MessageStartChars{0x22, 0x07, 0x5c, 0x42}
            : (regtest ? MessageStartChars{0x22, 0x07, 0x5c, 0x06}
                       : MessageStartChars{0xe6, 0x72, 0x1b, 0x89});
        nDefaultPort = main ? 25516 : (regtest ? 35714 : 35516);
        m_assumed_blockchain_size = 0;
        genesis = CreateGenesisBlock(main ? 1329319 : 106276, main ? 0x1e0fffff : 0x1f00ffff);
        consensus.hashGenesisBlock = genesis.GetHash();
        const uint256 expected = main
            ? uint256{"000007e104c49bac76127806f96d50f2510fe42c0c47126d511cea785248c1ef"}
            : uint256{"0000d14e23132387cd7d0eece87ad8a04469516f465e69b0679f4e12ee63f1e6"};
        assert(consensus.hashGenesisBlock == expected);
        assert(genesis.hashMerkleRoot == uint256{"75bc1a80e13213ada9e18dac132c3c8d3c03ff3d1bcc7aec73f5f35155b8518a"});

        if (main) {
            vSeeds = {"seeds.dorkcore.org", "seeds.dorkcoin.org"};
        }
        vFixedSeeds.clear();
        base58Prefixes[PUBKEY_ADDRESS] = {30};
        base58Prefixes[SCRIPT_ADDRESS] = {31};
        base58Prefixes[SECRET_KEY] = {static_cast<unsigned char>(chain == ChainType::TESTNET ? 239 : 158)};
        base58Prefixes[EXT_PUBLIC_KEY] = chain == ChainType::TESTNET
            ? std::vector<unsigned char>{0x04, 0x35, 0x87, 0xcf}
            : std::vector<unsigned char>{0x04, 0x88, 0xb2, 0x1e};
        base58Prefixes[EXT_SECRET_KEY] = chain == ChainType::TESTNET
            ? std::vector<unsigned char>{0x04, 0x35, 0x83, 0x94}
            : std::vector<unsigned char>{0x04, 0x88, 0xad, 0xe4};
        cashaddr_prefix = main ? "dorkcoin" : (regtest ? "dorkreg" : "dorktest");
        bech32_hrp = main ? "dork" : (regtest ? "rdork" : "tdork");
        fDefaultConsistencyChecks = regtest;
        m_is_mockable_chain = regtest;
        // Legacy regtest did not define a hardened checkpoint.
        if (!regtest) checkpointData = {{{0, expected}}};
        m_assumeutxo_data.clear();
        chainTxData = {regtest ? 0 : 1773037267, 0, 0};

        if (regtest) {
            for (const auto& [dep, height] : opts.activation_heights) {
                switch (dep) {
                case Consensus::DEPLOYMENT_CSV:
                    throw std::runtime_error("Dorkcoin CSV activation is timestamp-based; a CSV height override is unsupported.");
                case Consensus::DEPLOYMENT_SEGWIT:
                    consensus.SegwitHeight = height;
                    break;
                }
            }
            for (const auto& [dep, params] : opts.version_bits_parameters) {
                consensus.vDeployments[dep].nStartTime = params.start_time;
                consensus.vDeployments[dep].nTimeout = params.timeout;
                consensus.vDeployments[dep].min_activation_height = params.min_activation_height;
            }
        }
    }
};
} // namespace

std::unique_ptr<const CChainParams> CChainParams::Main()
{
    return std::make_unique<const DorkcoinParams>(ChainType::MAIN);
}

std::unique_ptr<const CChainParams> CChainParams::TestNet()
{
    return std::make_unique<const DorkcoinParams>(ChainType::TESTNET);
}

std::unique_ptr<const CChainParams> CChainParams::RegTest(const RegTestOptions& options)
{
    return std::make_unique<const DorkcoinParams>(ChainType::REGTEST, options);
}

std::unique_ptr<const CChainParams> CChainParams::SigNet(const SigNetOptions&)
{
    throw std::runtime_error("Dorkcoin 13.2.7 has no signet network.");
}

std::unique_ptr<const CChainParams> CChainParams::TestNet4()
{
    throw std::runtime_error("Dorkcoin 13.2.7 has no testnet4 network.");
}

std::vector<int> CChainParams::GetAvailableSnapshotHeights() const
{
    std::vector<int> heights;
    for (const auto& data : m_assumeutxo_data) heights.push_back(data.height);
    return heights;
}

std::optional<ChainType> GetNetworkForMagic(const MessageStartChars& magic)
{
    for (const auto chain : {ChainType::MAIN, ChainType::TESTNET, ChainType::REGTEST}) {
        const DorkcoinParams params{chain};
        if (params.MessageStart() == magic) return chain;
    }
    return std::nullopt;
}

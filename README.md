# Dork Core 28.4.0

**Dork Core** is the node and wallet software for **Dorkcoin**, the network and
currency. Dork Core is developed in the
[dork-core/dorkcoin](https://github.com/dork-core/dorkcoin) repository and its
software website is [dorkcore.org](https://dorkcore.org). The Dorkcoin network
and currency project is at [Dorkcoin.org](https://dorkcoin.org).

Version 28.4.0 upgrades the Dorkcoin 13.2.7 codebase to a Bitcoin Core 28.4-based
architecture while retaining the existing Dorkcoin chain and consensus rules.
Ongoing development is based on Dorkcoin, with compatible Bitcoin Core updates
reviewed for integration. It does not track Blackcoin network activations.

**Release validation is incomplete:** old-wallet migration, differential
invalid-block/reorganization tests, and Windows/Linux release builds still need
verification. Do not use the only copy of a wallet for migration or evaluation.

## Source provenance

- Dorkcoin consensus reference: the supplied Dorkcoin 13.2.7 source tree.
- Blackcoin More `28.x`: `57569507319529821855cf211614a86fb5dceb82`.
- Bitcoin Core v28.4 ancestor: `b110304705e1e1f97c88c4bb1455ab03c909450a`.

Blackcoin More is the historical porting base, not the ongoing project identity
or contribution destination. Upstream Git history and copyright notices are
retained. The local repository has not been published or tagged by this work.

## Preserved network identity

| Setting | Mainnet | Testnet | Regtest |
| --- | --- | --- | --- |
| P2P port | 25516 | 35516 | 35714 |
| RPC port | 25515 | 35515 | 35715 |
| Network magic | `22075c42` | `e6721b89` | `22075c06` |
| CashAddr prefix | `dorkcoin` | `dorktest` | `dorkreg` |
| Coinbase maturity | 100 | 10 | 10 |
| Last PoW height | 10000 | 2147483647 | 1000 |

The original genesis blocks, target limits, 60-second target spacing, 100-DORK
PoW/PoS subsidies, timestamp-based protocol transitions, legacy Base58 versions,
and legacy 80-byte P2P headers are retained. SegWit and Taproot are not enabled
on the existing networks. Signet and testnet4 are not Dorkcoin networks.

SegWit can be rehearsed on regtest with an explicit activation height. Mainnet
and public testnet remain unscheduled; upgrading or increasing staking weight
does not activate it. See [the height-activation guide](doc/segwit-activation.md)
for the test coverage, wallet safeguards, and coordinated-release procedure.

## Building

Build dependencies and platform guidance are described in
[doc/build-osx.md](doc/build-osx.md), [doc/build-unix.md](doc/build-unix.md), and
[doc/build-windows.md](doc/build-windows.md). For a headless build:

```sh
./autogen.sh
./configure --without-gui --disable-bench --disable-fuzz-binary
make -j4
```

Outputs include `src/dorkcoind`, `src/dorkcoin-cli`, `src/dorkcoin-tx`,
`src/dorkcoin-wallet`, and `src/test/test_dorkcoin`. Qt is optional.
With Qt 5 dependencies installed, configure with `--with-gui=qt5` instead of
`--without-gui` to also build `src/qt/dorkcoin-qt` and its test executable.

Focused port regression tests:

```sh
src/test/test_dorkcoin --run_test=dorkcoin_consensus_tests,dorkcoin_segwit_tests --log_level=test_suite
python3 test/functional/feature_dorkcoin_legacy_wire.py
```

The standalone smoke test uses disposable data directories and loopback-only
nodes on all three networks. It checks genesis/RPC identity and, on regtest,
legacy wallet addresses, a 100-DORK PoW block, and protocol-70015 handshake and
header framing. It does not connect to public peers or use existing wallets.
It also runs a separate regtest instance with SegWit activation at height 12,
tests a signed witness spend and invalid signature, and rolls the chain back
across activation before restoring it. A corrupted witness commitment block is
rejected without preventing acceptance of its unmodified counterpart.

Upstream test vectors and functional tests may assume Blackcoin network
parameters or deployed Bitcoin features. They require adaptation and must not
be treated as evidence of Dorkcoin historical compatibility merely because
the project builds.

### Historical mainnet validation

The consensus port was synchronized from genesis to the public Dorkcoin
mainnet tip at height **156650** with `-assumevalid=0`, no wallet, and staking
disabled. The tip hash was:

```text
26d75386f561ff9de30f8db6aa4c0e72232e627e2efbd4a227bab2adb1070271
```

Both original seed peers negotiated protocol 70015 and identified themselves
as `/Dorkcoin:13.2.2/`. The node exited initial block download. `verifychain 4 0`
returned `true`, and the log confirmed no coin database inconsistencies across
all 156650 blocks (319030 transactions). The UTXO set contained 331660 outputs
with total amount 15665000 DORK at that tip.

This is evidence of historical PoW/PoS acceptance and legacy-peer compatibility,
not proof of equivalence for all possible invalid blocks, future blocks,
reorganizations, or 13.2.7 wallet files. Twelve focused unit test cases and the
standalone all-network smoke test also passed. After the Dork Core rebrand,
thirteen focused unit cases (including software identity) and the all-network
smoke test passed. The final branded binary also reopened the historical
mainnet data offline and returned `true` for `verifychain 4 0` at the same tip,
advertising `/DorkCore:28.4.0/`. This second check reused the downloaded blocks;
it was not a new peer synchronization. The Qt URI tests were extended
for CashAddr on all three networks. The Qt application and test executable
built successfully on macOS; Qt options, URI/CashAddr, and nested-RPC tests
passed with `QT_QPA_PLATFORM=minimal`. Upstream skips the native application,
wallet, and address-book GUI tests on macOS with that platform, so those
interactive paths still need native-platform testing.

After height-gated SegWit preparation, fifteen focused C++ cases passed,
including synthetic PoW/PoS witness commitments. The expanded offline
functional test passed activation, witness spending, invalid witness and
rollback checks. Historical mainnet re-verification at the same tip also
returned `true`. This is not end-to-end PoS staking validation.

## Safe evaluation and migration

Use a separate, explicitly selected `-datadir`, disable networking for initial
evaluation, and keep independently verified offline copies of the original
wallet and data directory. Default names are `dorkcoin.conf`, `dorkcoind.pid`,
and the Dorkcoin data directory (`.dorkcoin` on Unix or
`Library/Application Support/Dorkcoin` on macOS).

Base58 remains the default address encoding. `-usecashaddr=1` selects the
existing Dorkcoin CashAddr representation of legacy destinations; decoding
accepts both representations. Wallet address generation rejects SegWit types
until the wallet has processed the SegWit activation block, including after
reorganizations below that height. Taproot addresses remain unavailable.

**Legacy wallet support requires Berkeley DB 6.2.38.** The first local DMG was
incorrectly linked to Berkeley DB 4.8, which reports legacy Btree version 10
wallets as corrupt because that database version is unsupported. Do not run
salvage based solely on that message. The corrected build uses Berkeley DB 6.2.
Configure now checks explicitly supplied BDB flags as well.

A disposable copy of an existing legacy wallet passed database verification,
offline wallet-tool loading, node loading, `backupwallet`, clean shutdown, and
reloading of both the wallet and its backup. The original was unchanged.
This does not establish compatibility with every historical wallet, encrypted
unlock/signing, staking, or reverse compatibility after a modern write.
Keep an independent old-wallet backup; never test against the only copy or
assume that copying chainstate is safe. The RPC interface and storage layout
also changed between these generations.

Before any production release, independently review consensus differences,
repeat historical replay for the exact release commit, test against a real
13.2.7 binary (including reorgs and activation boundaries), test wallet
migration and staking using disposable copies, and validate all supported
release platforms. Successful historical replay does not replace these checks.

## License

Released under the MIT license; see [COPYING](COPYING). The Bitcoin, Blackcoin,
and other upstream authors retain their original attributions.
Berkeley DB 6.2 has separate AGPLv3/commercial licensing terms. Its inclusion in
a binary does not make those terms MIT; preserve its license and the applicable
source/distribution obligations.

## Software identity and compatibility

Release names, GUI titles, and version output use **Dork Core**. The BIP14 peer
user agent uses `/DorkCore:28.4.0/` (without spaces as required by that format).
The macOS application bundle is `Dork-Core.app`, with display name Dork Core.
System icons and installer artwork use the existing Dorkcoin application logo.
Existing executable names, Qt settings identifiers, data directories, network
names, signed-message domain, and URI/address prefixes remain Dorkcoin-compatible.

`CLIENT_VERSION_IS_RELEASE` enables release metadata; it is not a certification
of production readiness. The validation limitations above still apply.

# Dorkcoin SegWit activation by height

## Current status

Mainnet and public testnet have **no scheduled SegWit activation**. Their
activation heights are `std::numeric_limits<int>::max()`, the disabled
sentinel. Upgrading to Dork Core 28.4.0 does not activate SegWit, and neither
staking weight nor software-version counts trigger activation.

The scheduling points are `MAINNET_SEGWIT_HEIGHT` and `TESTNET_SEGWIT_HEIGHT`
in [src/kernel/chainparams.cpp](../src/kernel/chainparams.cpp). The public
testnet height is independent of the mainnet height. Regtest remains disabled
by default and supports `-testactivationheight=segwit@H`. That override is
rejected on public networks; do not add per-node mainnet activation settings.

Taproot is a separate deployment and remains disabled. Enabling SegWit must
not enable Taproot, change genesis, or change the existing network identity.

## Activation semantics

For an agreed positive height `H`:

- Blocks below `H` retain the pre-SegWit rules.
- Block `H` and subsequent blocks enforce the SegWit v0 rules, including
  witness validation and commitments when witness data is present.
- The block assembler starts generating coinbase witness commitments at `H`
  for both PoW and PoS templates. PoS retains its empty first coinbase output;
  the zero-value witness commitment is a separate output.
- PoS coinbase structure is checked against activation height both on initial
  block acceptance and on chainstate connection/reindex.
- The wallet permits native and wrapped SegWit receive/change addresses only
  after processing block `H`. This is deliberately more conservative than the
  mempool's next-block activation check at tip `H-1`.
- Ordinary wallet sends to native witness destinations are blocked before
  wallet activation. Taproot and unknown witness versions are not offered.
- After a reorganization below `H`, SegWit address generation is blocked again.
  Existing addresses are not deleted. Applications must not interpret an
  activation-height rollback as permission to ignore already published addresses.

Legacy addresses remain the default and continue to work after activation.
Native SegWit v0 addresses use `dork1q...` on mainnet, `tdork1q...` on testnet,
and `rdork1q...` on regtest. CashAddr (`dorkcoin:...`) is not SegWit.

## Local regression test

Build the binaries and run:

```sh
src/test/test_dorkcoin \
  --run_test=dorkcoin_consensus_tests,dorkcoin_segwit_tests,v2_transaction_tests,minfee_tests,pos_tests \
  --log_level=message --color_output=no
python3 test/functional/feature_dorkcoin_legacy_wire.py
```

The standalone functional test creates disposable offline data directories.
In addition to the legacy three-network checks, it schedules regtest activation
at height 12 and checks:

- no commitment or SegWit receive/change addresses before activation;
- commitment and witness nonce in block 12;
- rejection of a block with a corrupted witness nonce, followed by acceptance
  of the unmodified block with the same header;
- native and wrapped receive-address availability and native change addresses;
- funding and spending a native SegWit output with a real wallet signature;
- rejection of an invalid witness signature;
- rollback below activation, rejection of premature witness relay, and
  restoration of the original chain;
- Taproot address rejection and full-chain `verifychain 4 0`;
- rejection of local public-network activation overrides.

Unit tests cover height boundaries, Bech32 round trips, PoW/PoS commitment
construction and malformed witness commitments. Synthetic PoS block tests do
not establish end-to-end staking or legacy-wallet migration compatibility.

## Scheduling a public activation

1. Complete review and end-to-end PoS testing, including witness-input
   coinstakes, legacy staking inputs, block signatures, invalid blocks, and
   reorganizations across activation. Validate migration, backups, and supported
   release platforms as described in [release-process.md](release-process.md).
2. Coordinate a **testnet** activation first. Choose a future height from the
   actual testnet tip, change only `TESTNET_SEGWIT_HEIGHT`, and update the tests
   that currently assert public-network SegWit is disabled. Publish a common
   testnet build and verify it across independent nodes.
3. Confirm readiness with stakers, exchanges, seed-node operators, wallets, and
   other services. Software-version strings are not authenticated proof of
   readiness; connected-node counts do not measure staking weight.
4. Agree on a future **mainnet** height with enough time for deployment.
   Replace `MAINNET_SEGWIT_HEIGHT` with that reviewed height in a coordinated
   release. Do not use a height already passed by the network or enable the
   regtest override on mainnet.
5. Update the network-identity tests to assert the exact agreed schedule and
   test `H-1`, `H`, `H+1`, and rollback. Replay historical mainnet with that
   exact release commit and `assumevalid=0`; record the tip and verification.
6. Publish the release, activation height, wallet guidance, and monitoring
   instructions before `H`. Old software must not be assumed safe for staking
   after activation. Do not encourage witness deposits before activation.

This implements a deterministic height schedule, not BIP9 voting. Once witness
outputs exist, disabling SegWit is not a routine rollback procedure.

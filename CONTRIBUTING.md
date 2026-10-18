# Contributing to Dork Core

Dork Core is the node and wallet software for **Dorkcoin**, the network and
currency. The software website is [dorkcore.org](https://dorkcore.org);
[Dorkcoin.org](https://dorkcoin.org) is for the Dorkcoin network and currency
project. Development, issues, and pull requests belong in the
[dork-core/dorkcoin](https://github.com/dork-core/dorkcoin) repository,
including GUI changes.

The project is maintained as Dorkcoin software. Blackcoin More provided the
historical base for the 28.4.0 port; it is not the target repository for ongoing
Dork Core development. Bitcoin Core changes may be integrated after review for
compatibility with Dorkcoin. Preserve upstream attribution and Git provenance.

## Getting started

- Read the [build documentation](doc/README.md) and
  [developer notes](doc/developer-notes.md).
- Look through [open issues](https://github.com/dork-core/dorkcoin/issues) and
  [pull requests](https://github.com/dork-core/dorkcoin/pulls) before starting work.
- Discuss substantial features and consensus-sensitive changes in a Dork Core
  issue before implementation. Do not post vulnerabilities publicly; follow
  [SECURITY.md](SECURITY.md).

## Contributor workflow

1. Fork https://github.com/dork-core/dorkcoin.
2. Create a topic branch from the Dork Core branch targeted by your change.
3. Make focused, atomic commits with clear descriptions of what changed and why.
4. Add regression tests and update directly related documentation.
5. Build and run the relevant tests, then open a pull request to this repository.
6. Respond to review and keep the branch current with its Dork Core target.

Do not rebase work onto Blackcoin as part of normal project maintenance.
Upstream Bitcoin fixes should retain their source references and be reviewed
against Dorkcoin's transaction format, PoW/PoS rules, and activation conditions.

## Scope and naming

- **Dork Core**: software, release names, GUI titles, and project documentation.
- **Dorkcoin**: blockchain, currency, network, addresses, and signed messages.
- Preserve existing executable names (`dorkcoind`, `dorkcoin-cli`,
  `dorkcoin-qt`), `dorkcoin.conf`, and data-directory conventions.
- Do not change network parameters, address prefixes, wire formats, rewards,
  or consensus rules as a side effect of branding or an upstream merge.
- Keep copyright notices and the MIT license for inherited code.

## Testing and review

Follow existing coding patterns and keep unrelated refactoring separate.
Use component prefixes such as `consensus:`, `net:`, `wallet:`, `qt:`, `rpc:`,
`test:`, `build:`, and `doc:` in pull-request titles where useful.

Run the smallest relevant tests first. For the Dorkcoin port regression suite:

```sh
src/test/test_dorkcoin --run_test=dorkcoin_consensus_tests,v2_transaction_tests,minfee_tests,pos_tests
python3 test/functional/feature_dorkcoin_legacy_wire.py
```

The standalone functional smoke test creates disposable local nodes and wallets.
It does not use existing wallets or public peers. See [README.md](README.md)
for validation evidence and limitations, and [test/functional/README.md](test/functional/README.md)
for the upstream functional-test framework.

Consensus-sensitive changes require explicit review, historical-chain replay,
activation-boundary and invalid-block tests, and reorganization testing.
Passing a historical replay does not establish correctness for every possible
block. Wallet changes need disposable migration, backup, restore, and staking
tests; never test against the only copy of a user's wallet.

Maintainers should document validation commands, platforms, skipped tests, and
known limitations in release notes. Do not describe an unverified platform or
migration path as production-ready.

## Translations and releases

See [translation_process.md](doc/translation_process.md) and
[release-process.md](doc/release-process.md). Submit translation changes to
this repository unless maintainers announce a verified external service.

Anyone may review a patch. Explain disagreements with concrete technical
reasoning, and treat contributors respectfully.

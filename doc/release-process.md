# Dork Core release process

Releases are maintained in https://github.com/dork-core/dorkcoin. Dork Core is
the software; Dorkcoin is the network and currency. Blackcoin More is a
historical source base only. Do not publish Dork Core releases through
upstream Bitcoin or Blackcoin infrastructure.

## Preparing a release

1. Agree on the Dork Core target branch and release scope in this repository.
2. Update version fields in `configure.ac` and the Windows configuration
   template where required. Use `CLIENT_VERSION_RC` for release candidates.
   `CLIENT_VERSION_IS_RELEASE` controls build metadata and warnings; setting it
   does not establish production readiness.
3. Update [README.md](../README.md), [CHANGELOG.md](../CHANGELOG.md), and release
   notes with changes, migration instructions, known issues, and test evidence.
4. Preserve existing Dorkcoin consensus/network parameters unless a separately
   reviewed and coordinated protocol change is explicitly intended.
5. Retain inherited copyright notices in [COPYING](../COPYING) and source files.
6. Rebuild and regenerate manuals and the example configuration:

   ```sh
   python3 contrib/devtools/gen-manpages.py
   contrib/devtools/gen-dorkcoin-conf.sh
   ```

   The manual generator expects all documented binaries, including Qt. Run it
   against the exact clean release commit, not stale or dirty binaries.

## Required validation

- Build node, CLI, wallet utilities, GUI, and test binaries on supported targets.
- Run the focused Dorkcoin tests and relevant upstream unit/functional tests.
  Record failing or skipped tests explicitly; do not claim the full suite passed
  when only selected tests ran.
- Replay mainnet with `assumevalid=0`, then run `verifychain 4 0`. Record height,
  tip hash, software commit, and the complete verification result.
- Test protocol-70015 legacy peers, PoW/PoS activation boundaries, invalid blocks,
  and reorganizations against the relevant older Dorkcoin version.
- Test wallet backup, restoration, migration, transaction creation, and staking
  using disposable wallet copies. Never test against the only wallet backup.
- Test native GUI behavior; macOS minimal-platform skips are not native GUI
  coverage. Validate installers, URI handlers, and clean data-directory startup.
- Review security-sensitive changes and release dependencies.

SegWit activation requires a separately coordinated height schedule. Follow
[segwit-activation.md](segwit-activation.md); the current mainnet and testnet
heights are unscheduled, and stake-weight majority does not trigger activation.

The historical replay recorded in README.md is useful evidence, but must not
substitute for verification of the exact release commit.

## Publishing

Only maintainers with release authority should tag and publish a release.
Publish approved artifacts and release notes to
https://github.com/dork-core/dorkcoin/releases. Produce checksums and signatures
with the project's established maintainer keys; do not invent keys or reuse
upstream projects' signing identities. Verify downloads and installation after
publication. Updating local source does not publish a GitHub release.

Docker publishing requires a maintainer-configured `DOCKERFILE_REPOSITORY`
repository variable identifying reviewed Dork Core Dockerfiles, along with the
existing registry secrets. Publishing is disabled by default; the historical
upstream Dockerfile repository is not used.

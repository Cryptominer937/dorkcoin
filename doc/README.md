Dork Core
=============

Setup
---------------------
Dork Core is node and wallet software for the Dorkcoin network. It downloads and
verifies Dorkcoin blocks and transactions. Storage requirements and synchronization
time depend on chain size, hardware, and connectivity.

For source, releases, and validation limitations, see the
[Dork Core repository](https://github.com/dork-core/dorkcoin) and the
[README](../README.md). Dork Core's software website is
[dorkcore.org](https://dorkcore.org); the Dorkcoin network and currency project
is at [Dorkcoin.org](https://dorkcoin.org).

Running
---------------------
The following are notes on how to run Dork Core on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/dorkcoin-qt` (GUI) or
- `bin/dorkcoind` (headless)

### Windows

Unpack the files into a directory, and then run dorkcoin-qt.exe.

### macOS

Move the application bundle to your Applications folder and run Dork Core.

### Need Help?

Use [Dork Core issues](https://github.com/dork-core/dorkcoin/issues) for ordinary
bugs and development questions. Follow [SECURITY.md](../SECURITY.md) for vulnerabilities.

Building
---------------------
The following are developer notes on how to build Dork Core. They include
inherited Bitcoin Core build guidance; consult the root README for tested targets.

- [Dependencies](dependencies.md)
- [macOS Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [FreeBSD Build Notes](build-freebsd.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [NetBSD Build Notes](build-netbsd.md)

Development
---------------------
The [root README](../README.md) and [CONTRIBUTING.md](../CONTRIBUTING.md)
describe Dork Core development and automated testing.

- [Developer Notes](developer-notes.md)
- [Productivity Notes](productivity.md)
- [Release Process](release-process.md)
- [Bitcoin Core Upstream Documentation (not Dork Core-specific)](https://doxygen.bitcoincore.org/)
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [JSON-RPC Interface](JSON-RPC-interface.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)
- [Internal Design Docs](design/)

### Resources
* [Dork Core pull requests](https://github.com/dork-core/dorkcoin/pulls)
* [Contribution guidelines](../CONTRIBUTING.md)

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [dorkcoin.conf Configuration File](bitcoin-conf.md)
- [CJDNS Support](cjdns.md)
- [Files](files.md)
- [Fuzz-testing](fuzzing.md)
- [I2P Support](i2p.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)
- [Managing Wallets](managing-wallets.md)
- [Multisig Tutorial](multisig-tutorial.md)
- [Offline Signing Tutorial](offline-signing-tutorial.md)
- [P2P bad ports definition and list](p2p-bad-ports.md)
- [PSBT support](psbt.md)
- [Reduce Memory](reduce-memory.md)
- [Reduce Traffic](reduce-traffic.md)
- [Tor Support](tor.md)
- [Transaction Relay Policy](policy/README.md)
- [ZMQ](zmq.md)

License
---------------------
Distributed under the [MIT software license](/COPYING).

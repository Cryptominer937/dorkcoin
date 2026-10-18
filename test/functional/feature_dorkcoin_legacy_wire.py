#!/usr/bin/env python3
"""Offline Dorkcoin startup, wallet, legacy wire and SegWit activation test.

Uses only the Python standard library and locally built binaries.
This does not replace historical-chain replay or a real 13.2.7 peer test.
"""

import hashlib
import json
from pathlib import Path
import socket
import struct
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parents[2]
DAEMON = ROOT / "src" / "dorkcoind"
CLI = ROOT / "src" / "dorkcoin-cli"


def free_port():
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def hash256(data):
    return hashlib.sha256(hashlib.sha256(data).digest()).digest()


def receive_exact(sock, length):
    result = b""
    while len(result) < length:
        chunk = sock.recv(length - len(result))
        if not chunk:
            raise AssertionError("Peer disconnected before completing a message")
        result += chunk
    return result


def send(sock, magic, command, payload=b""):
    sock.sendall(magic + command.ljust(12, b"\0") +
                 struct.pack("<I", len(payload)) + hash256(payload)[:4] + payload)


def receive(sock, magic):
    header = receive_exact(sock, 24)
    assert header[:4] == magic
    length = struct.unpack("<I", header[16:20])[0]
    assert length <= 4_000_000
    payload = receive_exact(sock, length)
    assert hash256(payload)[:4] == header[20:24]
    return header[4:16].rstrip(b"\0"), payload


def check_legacy_peer(port, magic, genesis, raw_header):
    address = struct.pack("<Q", 1) + b"\0" * 10 + b"\xff\xff\x7f\0\0\x01" + struct.pack(">H", port)
    user_agent = b"/Dorkcoin:13.2.7/"
    version = struct.pack("<iQq", 70015, 1, int(time.time())) + address * 2
    version += struct.pack("<Q", 123456789) + bytes([len(user_agent)]) + user_agent
    version += struct.pack("<iB", 0, 1)
    with socket.create_connection(("127.0.0.1", port), timeout=15) as sock:
        sock.settimeout(15)
        send(sock, magic, b"version", version)
        seen_version = False
        while True:
            command, payload = receive(sock, magic)
            if command == b"version":
                seen_version = True
                send(sock, magic, b"verack")
            if command == b"verack":
                assert seen_version
                break
        locator = struct.pack("<iB", 70015, 1) + bytes.fromhex(genesis)[::-1] + b"\0" * 32
        send(sock, magic, b"getheaders", locator)
        while True:
            command, payload = receive(sock, magic)
            if command == b"ping":
                send(sock, magic, b"pong", payload)
            if command == b"headers":
                # CompactSize(1), the legacy 80-byte header, empty tx and block-signature vectors.
                assert payload == b"\x01" + raw_header + b"\0\0", payload.hex()
                return


def check_segwit_activation(rpc, legacy, height):
    def assert_no_witness_addresses():
        for address_type in ("bech32", "p2sh-segwit"):
            for method, arguments in (("getnewaddress", ("", address_type)),
                                      ("getrawchangeaddress", (address_type,))):
                error = rpc(method, *arguments, expected_error=True)
                assert "not active" in error, error
        assert_no_taproot_addresses()

    def assert_no_taproot_addresses():
        for method, arguments in (("getnewaddress", ("", "bech32m")),
                                  ("getrawchangeaddress", ("bech32m",))):
            error = rpc(method, *arguments, expected_error=True)
            assert "not active" in error or "cannot provide bech32m" in error, error

    assert_no_witness_addresses()
    rpc("generatetoaddress", height - 3, legacy, 1_000_000)
    assert rpc("getblockcount") == height - 2
    last_legacy = rpc("generateblock", legacy, "[]")["hash"]
    before = rpc("getblock", last_legacy, 2)
    assert before["height"] == height - 1
    assert len(before["tx"][0]["vout"]) == 1
    assert_no_witness_addresses()

    candidate = rpc("generateblock", legacy, "[]", "false")
    raw_candidate = bytearray.fromhex(candidate["hex"])
    nonce = b"\x01\x20" + b"\0" * 32
    assert raw_candidate.count(nonce) == 1
    raw_candidate[raw_candidate.index(nonce) + 2] ^= 1
    assert rpc("submitblock", raw_candidate.hex()) == "bad-witness-merkle-match"
    assert rpc("getblockcount") == height - 1
    assert rpc("submitblock", candidate["hex"]) == ""
    activation = candidate["hash"]
    active_block = rpc("getblock", activation, 2)
    assert active_block["height"] == height
    assert active_block["tx"][0]["vout"][1]["scriptPubKey"]["hex"].startswith("6a24aa21a9ed")
    assert active_block["tx"][0]["vin"][0]["txinwitness"] == ["00" * 32]
    witness = rpc("getnewaddress", "", "bech32")
    assert witness.startswith("rdork1q")
    assert rpc("getaddressinfo", witness)["iswitness"]
    assert rpc("getaddressinfo", rpc("getrawchangeaddress", "bech32"))["iswitness"]
    assert rpc("getnewaddress", "", "p2sh-segwit")
    assert_no_taproot_addresses()
    assert rpc("settxfee", "0.01")
    funding = rpc("sendtoaddress", witness, 1)
    [funding_block] = rpc("generatetoaddress", 1, legacy, 1_000_000)
    funding_tx = next(tx for tx in rpc("getblock", funding_block, 2)["tx"] if tx["txid"] == funding)
    output = next(vout for vout in funding_tx["vout"] if vout["scriptPubKey"].get("address") == witness)
    unsigned = rpc("createrawtransaction", json.dumps([{"txid": funding, "vout": output["n"]}]),
                   json.dumps({legacy: 0.99}))
    signed = rpc("signrawtransactionwithwallet", unsigned)
    assert signed["complete"], signed
    decoded = rpc("decoderawtransaction", signed["hex"])
    signature = decoded["vin"][0]["txinwitness"][0]
    assert signature.startswith("30")
    corrupt_signature = signature[:20] + ("00" if signature[20:22] != "00" else "01") + signature[22:]
    invalid = signed["hex"].replace(signature, corrupt_signature, 1)
    [rejected] = rpc("testmempoolaccept", json.dumps([invalid]))
    assert not rejected["allowed"], rejected
    assert "script" in rejected["reject-reason"].lower(), rejected
    [accepted] = rpc("testmempoolaccept", json.dumps([signed["hex"]]))
    assert accepted["allowed"], accepted
    spend = rpc("sendrawtransaction", signed["hex"])
    [spend_block] = rpc("generatetoaddress", 1, legacy, 1_000_000)
    assert spend in [tx["txid"] for tx in rpc("getblock", spend_block, 2)["tx"]]
    assert rpc("verifychain", 4, 0)

    rpc("invalidateblock", activation)
    assert rpc("getblockcount") == height - 1
    assert_no_witness_addresses()
    assert "SegWit" in rpc("sendtoaddress", witness, 1, expected_error=True)
    rpc("invalidateblock", last_legacy)
    assert rpc("getblockcount") == height - 2
    [premature] = rpc("testmempoolaccept", json.dumps([signed["hex"]]))
    assert not premature["allowed"] and premature["reject-reason"] == "no-witness-yet", premature
    rpc("reconsiderblock", last_legacy)
    rpc("reconsiderblock", activation)
    assert rpc("getbestblockhash") == spend_block
    assert rpc("getnewaddress", "", "bech32").startswith("rdork1q")
    assert rpc("verifychain", 4, 0)


def run_network(chain, genesis, magic, activation_height=None):
    rpc_port, p2p_port = free_port(), free_port()
    while p2p_port == rpc_port:
        p2p_port = free_port()
    with tempfile.TemporaryDirectory(prefix="dorkcoin-port-test-") as directory:
        shared = [f"-datadir={directory}", f"-chain={chain}", f"-rpcport={rpc_port}"]

        def rpc(*arguments, expected_error=False):
            result = subprocess.run(
                [str(CLI), *shared, *map(str, arguments)],
                text=True, capture_output=True, timeout=180,
            )
            if expected_error:
                assert result.returncode != 0, result.stdout
                return result.stderr
            assert result.returncode == 0, result.stderr
            try:
                return json.loads(result.stdout)
            except json.JSONDecodeError:
                return result.stdout.strip()

        with open(Path(directory) / "console.log", "w+") as log:
            activation_args = [] if activation_height is None else [f"-testactivationheight=segwit@{activation_height}"]
            process = subprocess.Popen([
                str(DAEMON), *shared, "-server=1", "-listen=1", "-listenonion=0",
                f"-bind=127.0.0.1:{p2p_port}", "-dnsseed=0", "-connect=0",
                "-discover=0", "-upnp=0", "-natpmp=0", "-staking=0", "-deprecatedrpc=create_bdb",
                *activation_args,
            ], stdout=log, stderr=subprocess.STDOUT)
            try:
                info = rpc("-rpcwait", "-rpcwaittimeout=30", "getblockchaininfo")
                assert process.poll() is None
                assert info["bestblockhash"] == genesis
                assert info["blocks"] == 0
                network = rpc("getnetworkinfo")
                assert network["subversion"].startswith("/DorkCore:28.4.0")
                assert network["connections_out"] == 0
                if chain == "regtest":
                    rpc("createwallet", "disposable")
                    address = rpc("getnewaddress")
                    assert address.startswith("D")
                    assert "SegWit" in rpc("getnewaddress", "", "bech32", expected_error=True)
                    [block_hash] = rpc("generatetoaddress", 1, address, 1_000_000)
                    block = rpc("getblock", block_hash, 2)
                    assert block["bits"] == "1f00ffff"
                    assert len(block["tx"][0]["vout"]) == 1
                    assert block["tx"][0]["vout"][0]["value"] == 100
                    raw = bytes.fromhex(rpc("getblock", block_hash, 0))
                    check_legacy_peer(p2p_port, magic, genesis, raw[:80])
                    rpc("createwallet", "legacy", "false", "false", "", "false", "false")
                    assert rpc("-rpcwallet=legacy", "getwalletinfo")["format"] == "bdb"
                    legacy_address = rpc("-rpcwallet=legacy", "getnewaddress")
                    assert legacy_address.startswith("D")
                    backup = Path(directory) / "legacy-backup.dat"
                    rpc("-rpcwallet=legacy", "backupwallet", backup)
                    assert struct.unpack_from("<I", backup.read_bytes(), 16)[0] == 10
                    rpc("unloadwallet", "legacy")
                    rpc("loadwallet", "legacy")
                    assert rpc("-rpcwallet=legacy", "getaddressinfo", legacy_address)["ismine"]
                    rpc("unloadwallet", "legacy")
                    if activation_height is not None:
                        check_segwit_activation(rpc, address, activation_height)
                print(f"{chain}: genesis, RPC and identity OK" +
                      ("; wallet, 100-DORK PoW and protocol-70015 headers OK" if chain == "regtest" else "") +
                      ("; SegWit activation, witness spend, invalid signature and reorg OK" if activation_height is not None else ""))
            except BaseException:
                log.flush()
                log.seek(0)
                print(log.read())
                raise
            finally:
                if process.poll() is None:
                    process.terminate()
                try:
                    process.wait(timeout=30)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


if __name__ == "__main__":
    for public_chain in ("main", "test"):
        with tempfile.TemporaryDirectory(prefix="dorkcoin-activation-rejection-") as directory:
            rejected = subprocess.run(
                [str(DAEMON), f"-datadir={directory}", f"-chain={public_chain}",
                 "-testactivationheight=segwit@12", "-connect=0", "-listen=0", "-dnsseed=0"],
                text=True, capture_output=True, timeout=30,
            )
            assert rejected.returncode != 0, rejected.stdout
            assert "only supported on regtest" in rejected.stderr, rejected.stderr
    print("main/test: local activation-height overrides rejected")
    for network, block_hash, network_magic in [
        ("main", "000007e104c49bac76127806f96d50f2510fe42c0c47126d511cea785248c1ef", "22075c42"),
        ("test", "0000d14e23132387cd7d0eece87ad8a04469516f465e69b0679f4e12ee63f1e6", "e6721b89"),
        ("regtest", "0000d14e23132387cd7d0eece87ad8a04469516f465e69b0679f4e12ee63f1e6", "22075c06"),
    ]:
        run_network(network, block_hash, bytes.fromhex(network_magic))
    run_network("regtest", "0000d14e23132387cd7d0eece87ad8a04469516f465e69b0679f4e12ee63f1e6",
                bytes.fromhex("22075c06"), activation_height=12)

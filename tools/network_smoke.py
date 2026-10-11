#!/usr/bin/env python3
"""Real two-peer protocol + two terminal-client integration test (stdlib only)."""
import argparse
import json
import shlex
import socket
import struct
import subprocess
import time
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("build", type=Path)
parser.add_argument("--capture")
args = parser.parse_args()
build = args.build.resolve()
suffix = ".exe" if (build / "game_server.exe").exists() else ""
with socket.socket() as probe:
    probe.bind(("127.0.0.1", 0))
    port = probe.getsockname()[1]
server = subprocess.Popen([str(build / ("game_server" + suffix)), "--port", str(port), "--ticks", "900"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
connections = []
clients = []

def send(sock, text):
    payload = text.encode()
    sock.sendall(struct.pack("!I", len(payload)) + payload)

def exact(sock, count):
    data = b""
    while len(data) < count:
        block = sock.recv(count - len(data))
        if not block:
            raise AssertionError("connection closed unexpectedly")
        data += block
    return data

def receive(sock):
    size, = struct.unpack("!I", exact(sock, 4))
    assert size < 512 * 1024
    return exact(sock, size).decode()

def until(sock, predicate):
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        message = receive(sock)
        if predicate(message):
            return message
    raise AssertionError("expected message did not arrive")

def snapshot(sock, after=-1):
    return until(sock, lambda text: text.startswith("SNAP ") and int(text.split()[1]) > after)

def entity(state, identity):
    for line in state.splitlines():
        row = shlex.split(line)
        if row[0] == "E" and int(row[1]) == identity:
            return row
    raise AssertionError(f"missing entity {identity}")

try:
    ready = server.stdout.readline()
    assert "snapshots 20 Hz" in ready, server.stderr.read()
    for name, role in [("Alice", 1), ("Bob", 0)]:
        sock = socket.create_connection(("127.0.0.1", port), timeout=3)
        sock.settimeout(3)
        connections.append(sock)
        send(sock, f'HELLO 1 "{name}" {role}')
    alice, bob = connections
    aid = int(until(alice, lambda s: s.startswith("WELCOME ")).split()[1])
    bid = int(until(bob, lambda s: s.startswith("WELCOME ")).split()[1])
    initial = snapshot(alice)
    initial_tick = int(initial.split()[1])
    ax = float(entity(initial, aid)[5])
    send(alice, "MOVE 1 0")
    moved = snapshot(alice, initial_tick + 24)
    delta = float(entity(moved, aid)[5]) - ax
    assert 0.3 < delta < 2, f"server movement/timeout failed: {delta}"
    send(alice, "MOVE 9999 0")
    send(alice, "SET_HP 999999")
    send(alice, 'CHAT "hello from integration"')
    until(bob, lambda s: "Alice: hello from integration" in s)
    send(alice, f"INVITE {bid}")
    until(bob, lambda s: "invited you" in s)
    send(bob, "ACCEPT")
    until(alice, lambda s: "joined Alice's party" in s)
    state = snapshot(alice)
    assert entity(state, aid)[21] == entity(state, bid)[21], "party membership not authoritative"
    before_mp = float(entity(state, aid)[11])
    send(alice, "CAST 0 20 12 0")
    state = snapshot(alice, int(state.split()[1]) + 9)
    assert float(entity(state, aid)[11]) < before_mp, "server did not spend MP"
    assert float(entity(state, aid)[9]) <= float(entity(state, aid)[10]), "client altered HP"
    for index in range(2):
        command = [str(build / ("eternal_rpg" + suffix)), "--connect", f"127.0.0.1:{port}", "--name", f"Terminal{index}", "--headless", "--frames", "90"]
        if index == 0 and args.capture:
            command += ["--dump", args.capture]
        clients.append(subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True))
    for client in clients:
        output, error = client.communicate(timeout=10)
        assert client.returncode == 0, error
        assert "90 frames" in output
    if args.capture:
        assert json.loads(Path(args.capture).read_text())["width"] == 200
    print("PASS: two peers, authoritative movement/MP, chat, party invites, and two terminal clients")
finally:
    for client in clients:
        if client.poll() is None:
            client.terminate()
            client.wait(timeout=5)
    for sock in connections:
        sock.close()
    server.terminate()
    server.communicate(timeout=5)

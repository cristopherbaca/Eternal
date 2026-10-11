#!/usr/bin/env python3
"""POSIX PTY smoke tests: input, resize, signal restoration and frame throughput."""
import argparse
import errno
import fcntl
import json
import os
import resource
import select
import signal
import struct
import subprocess
import tempfile
import termios
import time
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("executable", type=Path)
parser.add_argument("--stats", type=Path)
args = parser.parse_args()
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))

def run(extra, end_signal=None, exercise=False, slow_reader=False, acknowledge=False):
    master, slave = os.openpty()
    fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack("HHHH", 60, 200, 0, 0))
    original = termios.tcgetattr(slave)
    environment = dict(os.environ)
    environment.pop("TERM_PROGRAM", None)
    if acknowledge:
        environment["TERM_PROGRAM"] = "Apple_Terminal"
    workspace = tempfile.TemporaryDirectory(prefix="eternal-session-")
    process = subprocess.Popen([str(args.executable.resolve())] + extra, stdin=slave, stdout=slave, stderr=slave, close_fds=True, env=environment, cwd=workspace.name)
    start = time.monotonic()
    tail = b""
    entered = restored = False
    sent = resized = False
    total = 0
    reply_at = None
    query_tail = b""
    queries = 0
    try:
        if slow_reader:
            # Let the shared nonblocking PTY fill before consuming output.
            time.sleep(0.25)
        while process.poll() is None:
            elapsed = time.monotonic() - start
            if elapsed > 12:
                raise AssertionError("terminal process hung")
            if reply_at is not None and time.monotonic() >= reply_at:
                os.write(master, b"\x1b[1;1R")
                reply_at = None
            if elapsed > 0.15 and not sent:
                sent = True
                if exercise:
                    os.write(master, b"\rwd\x1b[<0;120;30M1")
                if end_signal:
                    process.send_signal(end_signal)
            if exercise and elapsed > 0.3 and not resized:
                resized = True
                fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack("HHHH", 20, 60, 0, 0))
                process.send_signal(signal.SIGWINCH)
                os.write(master, b"q")
            ready, _, _ = select.select([master], [], [], 0.01)
            if ready:
                try:
                    block = os.read(master, 4096 if slow_reader else 65536)
                except OSError as exc:
                    if exc.errno == errno.EIO:
                        break
                    raise
                total += len(block)
                if acknowledge:
                    query_data = query_tail + block
                    count = query_data.count(b"\x1b[6n")
                    if count:
                        assert count == 1 and reply_at is None, "frames queued before terminal acknowledgement"
                        queries += count
                        reply_at = time.monotonic() + 0.08
                    query_tail = query_data[-3:]
                tail = (tail + block)[-131072:]
                entered |= b"\x1b[?1049h" in tail
                restored |= b"\x1b[?1049l" in tail and b"\x1b[?25h" in tail
                if slow_reader:
                    time.sleep(0.002)
        while select.select([master], [], [], 0)[0]:
            try:
                block = os.read(master, 65536)
            except OSError:
                break
            if not block:
                break
            total += len(block)
            tail = (tail + block)[-131072:]
            entered |= b"\x1b[?1049h" in tail
            restored |= b"\x1b[?1049l" in tail and b"\x1b[?25h" in tail
        process.wait(timeout=3)
        assert entered and restored, "alternate screen/cursor not restored"
        restored_mode = termios.tcgetattr(slave)
        # macOS sets PENDIN on the canonical-mode transition; it is kernel
        # pending-input state, not an application-controlled terminal setting.
        state_bit = getattr(termios, "PENDIN", 0)
        restored_mode[3] &= ~state_bit
        original[3] &= ~state_bit
        assert restored_mode == original, "terminal settings not restored"
        assert process.returncode in (0, 128 + signal.SIGABRT), tail[-2000:]
        if acknowledge:
            assert queries >= 5, "terminal acknowledgement path not exercised"
        return total
    finally:
        if process.poll() is None:
            process.kill()
            process.wait(timeout=3)
        os.close(master)
        os.close(slave)
        workspace.cleanup()

with tempfile.TemporaryDirectory(prefix="eternal-backpressure-") as directory:
    stats = Path(directory) / "metrics.json"
    received = run(["--frames", "1", "--stats", str(stats)], slow_reader=True)
    metrics = json.loads(stats.read_text())
    setup = b"\x1b[?1049h\x1b[?25l\x1b[?7l\x1b[?1003h\x1b[?1006h\x1b[2J"
    restore = b"\x1b[0m\x1b[?1003l\x1b[?1006l\x1b[?25h\x1b[?7h\x1b[?1049l"
    assert received == metrics["ansi_bytes"] + len(setup) + len(restore), "terminal output dropped under backpressure"
    print("PASS: slow terminal receives every frame byte")

with tempfile.TemporaryDirectory(prefix="eternal-ack-") as directory:
    stats = Path(directory) / "metrics.json"
    run(["--frames", "6", "--fps", "60", "--stats", str(stats)], acknowledge=True)
    assert json.loads(stats.read_text())["seconds"] >= 0.4, "presentation did not wait for slow terminal"
    print("PASS: delayed terminal acknowledgements prevent frame backlog")

run([], exercise=True)
for end_signal in (signal.SIGINT, signal.SIGTERM, signal.SIGABRT):
    run([], end_signal=end_signal)
with tempfile.TemporaryDirectory(prefix="eternal-pty-") as directory:
    stats = Path(directory) / "metrics.json"
    run(["--scene", "boss", "--demo", "--frames", "360", "--fps", "120", "--stats", str(stats)])
    metrics = json.loads(stats.read_text())
    if args.stats:
        args.stats.write_text(json.dumps(metrics, indent=2) + "\n")
    print(f"PASS: PTY input, resize, Ctrl-C, SIGTERM, crash restoration; {metrics['fps']:.1f} FPS, work p95 {metrics['work_p95_ms']:.2f} ms at 200x60")

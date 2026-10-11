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

def run(extra, end_signal=None, exercise=False):
    master, slave = os.openpty()
    fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack("HHHH", 60, 200, 0, 0))
    original = termios.tcgetattr(slave)
    process = subprocess.Popen([str(args.executable.resolve())] + extra, stdin=slave, stdout=slave, stderr=slave, close_fds=True)
    start = time.monotonic()
    tail = b""
    entered = restored = False
    sent = resized = False
    total = 0
    try:
        while process.poll() is None:
            elapsed = time.monotonic() - start
            if elapsed > 12:
                raise AssertionError("terminal process hung")
            if elapsed > 0.15 and not sent:
                sent = True
                if exercise:
                    os.write(master, b"wd\x1b[<0;120;30M1")
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
                    block = os.read(master, 65536)
                except OSError as exc:
                    if exc.errno == errno.EIO:
                        break
                    raise
                total += len(block)
                tail = (tail + block)[-131072:]
                entered |= b"\x1b[?1049h" in tail
                restored |= b"\x1b[?1049l" in tail and b"\x1b[?25h" in tail
        while select.select([master], [], [], 0)[0]:
            try:
                block = os.read(master, 65536)
            except OSError:
                break
            if not block:
                break
            tail = (tail + block)[-131072:]
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
        return total
    finally:
        if process.poll() is None:
            process.kill()
            process.wait(timeout=3)
        os.close(master)
        os.close(slave)

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

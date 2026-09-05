"""Launch PCSX-Redux configured for probing, and verify it came up.

Redux's debug facilities require the interpreter. Leaving the dynarec on
while the GDB server is armed produces visible GPU corruption and an
unstable emulator, and the trap is that `-gdb` *persists into the config*
-- dropping the flag on a later launch does not turn the server back off.
So the settings are enforced here rather than trusted.

Settings must also be written while Redux is not running: it rewrites
pcsx.json from memory on exit and would clobber anything changed under it.
"""

from __future__ import annotations

import argparse
import json
import os
import signal
import subprocess
import sys
import time
from pathlib import Path
from urllib.error import URLError
from urllib.request import urlopen

REPO_ROOT = Path(__file__).resolve().parent.parent
CONFIG = Path.home() / ".config/pcsx-redux/pcsx.json"
BINARY = REPO_ROOT / "bin/pcsx-redux"
PROBE_LUA = REPO_ROOT / "tools/redux_probe.lua"
DISC = REPO_ROOT / "disks/Final Fantasy VII (USA) (Disc 1).cue"

# Anything not in this shape has bitten us at least once.
REQUIRED = {
    ("emulator", "Dynarec"): False,
    ("emulator", "Debug", "Debug"): True,
    ("emulator", "Debug", "GdbServer"): False,
    ("emulator", "Debug", "WebServer"): True,
}


def running_pids() -> list[int]:
    """PIDs whose argv[0] is the emulator, not shells merely naming it."""
    pids = []
    for entry in Path("/proc").iterdir():
        if not entry.name.isdigit():
            continue
        try:
            argv = (entry / "cmdline").read_bytes().split(b"\0")
        except OSError:
            continue
        if argv and argv[0].decode(errors="replace").endswith("pcsx-redux"):
            pids.append(int(entry.name))
    return pids


def stop_running(timeout: float = 10.0) -> int:
    stopped = 0
    for pid in running_pids():
        try:
            os.kill(pid, signal.SIGTERM)
            stopped += 1
        except ProcessLookupError:
            pass
    deadline = time.time() + timeout
    while running_pids() and time.time() < deadline:
        time.sleep(0.25)
    return stopped


def apply_settings(
    extra: dict[tuple[str, ...], object] | None = None,
) -> dict[str, object]:
    """Write the required settings. Redux must not be running."""
    config = json.loads(CONFIG.read_text())
    changed = {}
    wanted_all = dict(REQUIRED)
    if extra is not None:
        wanted_all.update(extra)
    for path, wanted in wanted_all.items():
        node = config
        for key in path[:-1]:
            node = node.setdefault(key, {})
        if node.get(path[-1]) != wanted:
            changed["/".join(path)] = f"{node.get(path[-1])} -> {wanted}"
        node[path[-1]] = wanted
    CONFIG.write_text(json.dumps(config, indent=2))
    return changed


def wait_for_lua(
    host: str,
    timeout: float = 30.0,
) -> bool:
    """The Lua handler answering is the real proof it is usable."""
    url = f"http://{host}/api/v1/lua/eval?code=return%20%22up%22"
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with urlopen(url, timeout=2) as response:
                if response.read().decode(errors="replace") == "up":
                    return True
        except (URLError, OSError):
            pass
        time.sleep(1)
    return False


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="localhost:8080")
    parser.add_argument("--disc", type=Path, default=DISC)
    parser.add_argument("--no-launch", action="store_true")
    parser.add_argument(
        "--mcd1",
        type=Path,
        metavar="FILE",
        help="memory card for slot 1; .mcr and .mcd are the same raw format",
    )
    parser.add_argument("--mcd2", type=Path, metavar="FILE")
    args = parser.parse_args(argv)

    if args.no_launch:
        # Never stop a running emulator just to inspect settings: Redux
        # rewrites pcsx.json on exit, so a report here would be of the
        # file, not of what the running instance is actually using.
        for path in REQUIRED:
            node = json.loads(CONFIG.read_text())
            for key in path:
                node = node.get(key, {}) if isinstance(node, dict) else node
            state = "ok" if node == REQUIRED[path] else "WRONG"
            print(f"{'/'.join(path)} = {node} [{state}]")
        if running_pids():
            print("note: an emulator is running; it may not have "
                  "these settings loaded")
        return 0

    stopped = stop_running()
    if stopped:
        print(f"stopped {stopped} running emulator process(es)")
    cards: dict[tuple[str, ...], object] = {}
    if args.mcd1 is not None:
        cards[("emulator", "Mcd1")] = str(args.mcd1.expanduser().resolve())
    if args.mcd2 is not None:
        cards[("emulator", "Mcd2")] = str(args.mcd2.expanduser().resolve())
    changed = apply_settings(extra=cards)
    for key, transition in changed.items():
        print(f"setting {key}: {transition}")
    if not changed:
        print("settings already correct")

    command = [
        str(BINARY),
        "-iso", str(args.disc),
        "-run",
        "-stdout",
        "-interpreter",
        "-dofile", str(PROBE_LUA),
    ]
    subprocess.Popen(
        command,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        start_new_session=True,
    )
    if not wait_for_lua(args.host):
        print("emulator did not answer on the Lua endpoint", file=sys.stderr)
        return 1
    print(f"redux up, lua handler responding on {args.host}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

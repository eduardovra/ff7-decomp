"""Press buttons on controller 1 through Redux's pad override.

Battle probes need the game to reach an action, and a save state parked
on the command menu will sit there forever without input. Redux exposes
setOverride/clearOverride per button, so a tap is a press, a wait long
enough for the game to sample the pad, and a release.

    python3 tools/pad_input.py CROSS 4
    python3 tools/pad_input.py --hold 0.3 START
"""
import argparse
import subprocess
import sys
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
PAD = "PCSX.SIO0.slots[1].pads[1]"
BUTTONS = (
    "SELECT", "START", "UP", "RIGHT", "DOWN", "LEFT",
    "L2", "R2", "L1", "R1",
    "TRIANGLE", "CIRCLE", "CROSS", "SQUARE",
)
# battle samples the pad about once per rendered frame, and battle runs
# near 15fps, so a tap shorter than this can fall between two samples
DEFAULT_HOLD = 0.12


def lua(source: str) -> str:
    done = subprocess.run(
        [str(REPO_ROOT / ".venv/bin/python3"), str(REPO_ROOT / "tools/redux_lua.py")],
        input=source,
        capture_output=True,
        text=True,
        cwd=REPO_ROOT,
    )
    return done.stdout.strip()


def press(button: str) -> None:
    lua(f"{PAD}:setOverride(PCSX.CONSTS.PAD.BUTTON.{button}) {PAD}:map() return 'down'")


def release(button: str) -> None:
    lua(f"{PAD}:clearOverride(PCSX.CONSTS.PAD.BUTTON.{button}) {PAD}:map() return 'up'")


def tap(button: str, hold: float) -> None:
    press(button)
    time.sleep(hold)
    release(button)
    time.sleep(hold)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("button", choices=BUTTONS)
    parser.add_argument("count", nargs="?", type=int, default=1)
    parser.add_argument(
        "--hold",
        type=float,
        default=DEFAULT_HOLD,
        help="seconds to hold, and to wait after releasing",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    for _ in range(args.count):
        tap(button=args.button, hold=args.hold)
    print(f"tapped {args.button} x{args.count}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())

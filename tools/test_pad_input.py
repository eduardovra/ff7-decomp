import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import pad_input as pad


def test_every_button_the_pad_exposes_is_accepted() -> None:
    # the names must match PCSX.CONSTS.PAD.BUTTON, since they are pasted
    # straight into the Lua
    assert "CROSS" in pad.BUTTONS
    assert "START" in pad.BUTTONS
    assert len(pad.BUTTONS) == 14


def test_parser_defaults_to_a_single_tap() -> None:
    args = pad.build_parser().parse_args(["CROSS"])
    assert args.count == 1
    assert args.hold == pad.DEFAULT_HOLD


def test_parser_reads_a_repeat_count() -> None:
    args = pad.build_parser().parse_args(["START", "5"])
    assert args.button == "START"
    assert args.count == 5


def test_hold_is_long_enough_for_a_battle_frame() -> None:
    # battle runs near 15fps and samples the pad about once a frame, so a
    # tap shorter than one frame can land between two samples
    assert pad.DEFAULT_HOLD >= 1 / 15

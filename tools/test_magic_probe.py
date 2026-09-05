import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import magic_probe as mp


def test_arm_script_embeds_residency_head() -> None:
    head = bytes([0xE8, 0xFF, 0xBD, 0x27])
    script = mp.build_arm_script(
        address=0x801B0020,
        head=head,
        watches=[mp.Watch(name="x", addr=0x801B0CA0, size=4)],
    )
    assert "232,255,189,39" in script
    assert "0x%08X" in script
    # The watch address must be masked into the 2MB space getMemPtr uses.
    assert str(0x801B0CA0 & mp.RAM_MASK) in script
    assert "0x801B0CA0" not in script


def test_arm_script_keeps_the_breakpoint_alive() -> None:
    script = mp.build_arm_script(
        address=0x801B0020,
        head=b"\x00",
        watches=[mp.Watch(name="x", addr=0x80162978, size=2)],
    )
    # Returning false deletes the breakpoint, so it would fire once and
    # never again. Both of the callback's exits must return true: the
    # non-resident early-out and the normal path after recording.
    callback = script.split("PCSX.addBreakpoint", 1)[1]
    assert callback.count("return true") == 2
    assert "return false" not in callback


def test_parse_records_round_trip() -> None:
    text = "a=00FF b=1234\na=0100 b=1235\n"
    records = mp.parse_records(text)
    assert records == [
        {"a": "00FF", "b": "1234"},
        {"a": "0100", "b": "1235"},
    ]


def test_parse_records_ignores_blank_lines() -> None:
    assert mp.parse_records("\n\n") == []


def test_parse_watch_resolves_symbols_and_literals() -> None:
    symbols = {"D_80162978": 0x80162978}
    named = mp.parse_watch("D_80162978:32", symbols)
    assert (named.addr, named.size) == (0x80162978, 32)
    literal = mp.parse_watch("0x801B0CA0:4", symbols)
    assert (literal.addr, literal.size) == (0x801B0CA0, 4)


def test_overlay_heads_are_unique_across_magic_overlays() -> None:
    build_dir = mp.REPO_ROOT / "build" / "us"
    names = [
        "mabaria", "barrier", "brizad",
        "lv5deth", "thunder", "refrec", "fire",
    ]
    heads = {n: mp.overlay_head(n, build_dir) for n in names}
    assert len(set(heads.values())) == len(names)


def test_default_address_is_overlay_entry_plus_20() -> None:
    assert mp.resolve_address("mabaria", None, {}) == 0x801B0020


if __name__ == "__main__":
    for name, fn in sorted(globals().items()):
        if name.startswith("test_"):
            fn()
            print(f"ok  {name}")

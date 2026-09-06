import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import blend_probe as bp


def arm(page: int | None = 0x801B0CA8) -> str:
    if page is None:
        page_ptr = None
    else:
        page_ptr = 0x801D0CA8
    return bp.build_arm_script(
        address=0x801B0020,
        head=b"\xe8\xff\xbd\x27",
        page=page,
        page_ptr=page_ptr,
        cap=20000,
    )


def test_arm_script_keeps_the_breakpoint_alive() -> None:
    # Returning false deletes the breakpoint, so it would fire once and
    # then look like a spell that rendered a single frame.
    callback = arm().split("PCSX.addBreakpoint", 1)[1]
    assert callback.count("return true") == 2
    assert "return false" not in callback


def test_arm_script_masks_every_address_into_the_2mb_space() -> None:
    script = arm()
    assert str(bp.G_CDB & bp.RAM_MASK) in script
    assert str(0x801B0CA8 & bp.RAM_MASK) in script
    assert "0x801517C0" not in script
    # The breakpoint address is the exception: addBreakpoint takes a CPU
    # address, only the memory reads are indexes into getMemPtr.
    assert "PCSX.addBreakpoint(2149253152" in script


def test_arm_script_reads_the_ot_through_the_g_cdb_pointer() -> None:
    # g_cDb is a pointer, so the table is at *(g_cDb) + 0x70, not at
    # g_cDb + 0x70. Dereferencing is the whole point of the u32 call.
    script = arm()
    assert f"u32(mem, {bp.G_CDB & bp.RAM_MASK}) % 2097152 + {bp.OT_OFFSET}" \
        in script


def test_arm_script_uses_no_bit_operations() -> None:
    # Redux's Lua is not guaranteed to have them; the walk divides.
    script = arm()
    for operator in ("<<", ">>", "&", "|", "bit.", "bit32."):
        assert operator not in script


def test_arm_script_decodes_the_draw_mode_command() -> None:
    script = arm()
    assert "code == 225" in script  # GP0(0xE1)
    assert "math.floor(word / 32) % 4" in script  # bits 5-6


def test_arm_script_without_a_page_attributes_nothing() -> None:
    script = arm(page=None)
    assert "return false end" in script
    assert "pageLo" not in script


def test_parse_tally_reads_key_count_pairs() -> None:
    assert bp.parse_tally("1:3,0:2") == {1: 3, 0: 2}
    assert bp.parse_tally("") == {}


def test_summarize_names_the_rate() -> None:
    records = bp.parse_records("pkts=8 abe=2 ovl=2 ovlabe=2 e1=1:3 tp= "
                               "codes=E1:3 ovlcodes=3A:2")
    text = "\n".join(bp.summarize(records))
    assert "rate 1 (B+F)" in text
    assert "0x3A x2" in text


def test_summarize_says_so_when_no_draw_mode_command_appears() -> None:
    # An empty e1 field is the interesting negative result, not an error:
    # it means the rate is set outside the ordering table.
    records = bp.parse_records("pkts=8 abe=0 ovl=0 ovlabe=0 e1= tp= "
                               "codes=38:8 ovlcodes=")
    text = "\n".join(bp.summarize(records))
    assert "no draw-mode" in text


def test_rate_names_cover_the_two_bit_field() -> None:
    assert sorted(bp.RATE_NAMES) == [0, 1, 2, 3]


def test_default_page_symbols_resolve_for_the_shared_page_overlays() -> None:
    symbols = bp.load_symbols(bp.REPO_ROOT / "config")
    for overlay in ("mabaria", "brizad", "thunder", "barrier"):
        page, page_ptr = bp.default_page_symbols(overlay, symbols)
        assert page is not None
        assert page_ptr is not None
    # lv5deth carries two separate pages, so attribution is opt-in there.
    assert bp.default_page_symbols("lv5deth", symbols) == (None, None)

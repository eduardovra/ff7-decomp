import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import flag_probe as fp


def test_arm_script_embeds_both_values() -> None:
    script = fp.ARM.format(
        value=0xA8,
        alt=0xA9,
        freeze=8,
        head=",".join(str(b) for b in fp.HEAD),
        entry=fp.RENDER_ENTRY,
        after=fp.AFTER_DRAW,
        flags=fp.FLAGS,
        slots=fp.EFFECT_SLOTS,
        slot_index=fp.EFFECT_SLOT_INDEX,
        stride=fp.EFFECT_SLOT_STRIDE,
        anim=fp.ANIMATION_FRAME,
        page_lo=fp.PRIM_PAGE,
        page_hi=fp.PRIM_PAGE + fp.PAGE_SPAN,
        gcdb=fp.G_CDB,
        ot_offset=fp.OT_OFFSET,
        last_bucket=fp.OT_BUCKETS - 1,
        terminator=fp.OT_TERMINATOR,
        cap=fp.WALK_CAP,
    )
    assert "FF7Flags.value = 168" in script
    assert "FF7Flags.alt = 169" in script
    # both breakpoints, and the parity flip that alternates them
    assert "FF7Flags.parity = 1 - FF7Flags.parity" in script
    assert str(fp.RENDER_ENTRY) in script
    assert str(fp.AFTER_DRAW) in script


def test_arm_script_guards_on_residency() -> None:
    script = _default_script()
    # brizad's first bytes, as decimal, since every magic overlay shares
    # the load address
    assert "176,255,189,39" in script
    assert "FF7Flags.rejected = FF7Flags.rejected + 1" in script


def test_freeze_is_omitted_when_negative() -> None:
    script = _default_script(freeze=-1)
    assert "FF7Flags.freeze = -1" in script
    # the write is still emitted but gated, so a negative value runs free
    assert "if FF7Flags.freeze >= 0 then" in script


def test_addresses_are_masked_into_the_two_megabyte_space() -> None:
    script = _default_script()
    assert fp.FLAGS < 0x200000
    assert fp.PRIM_PAGE < 0x200000
    assert "0x801B1008" not in script


def _default_script(freeze: int = 8) -> str:
    return fp.ARM.format(
        value=0xA8,
        alt=0xA0,
        freeze=freeze,
        head=",".join(str(b) for b in fp.HEAD),
        entry=fp.RENDER_ENTRY,
        after=fp.AFTER_DRAW,
        flags=fp.FLAGS,
        slots=fp.EFFECT_SLOTS,
        slot_index=fp.EFFECT_SLOT_INDEX,
        stride=fp.EFFECT_SLOT_STRIDE,
        anim=fp.ANIMATION_FRAME,
        page_lo=fp.PRIM_PAGE,
        page_hi=fp.PRIM_PAGE + fp.PAGE_SPAN,
        gcdb=fp.G_CDB,
        ot_offset=fp.OT_OFFSET,
        last_bucket=fp.OT_BUCKETS - 1,
        terminator=fp.OT_TERMINATOR,
        cap=fp.WALK_CAP,
    )

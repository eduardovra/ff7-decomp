"""Read the GPU semi-transparency rate a magic overlay draws under.

`how-a-spell-is-drawn.md` section 10 shows the depth cue driving a
spell's vertex colour to the far colour, and flag 0x08 setting command
bit 0x02 so the primitive is drawn semi-transparent. What it cannot show
is the *rate* that blend uses: that is a two-bit field in the GPU's draw
mode, and no magic overlay writes it in either C or assembly. Under
B+F a black source vanishes, which is the fade-out the game shows; under
B/2+F/2 it would smear a dark shape instead.

So the rate has to be read from the machine. This walks the ordering
table at a breakpoint on the render function, once per frame, and
reports every draw-mode command in it plus how the overlay's own packets
are coded.

The ordering table is `g_cDb->unk70`: 0x1000 buckets, each the head of a
linked list of packets. A packet's first word is `(len << 24) | next`,
and the top byte of its second word is the GP0 command. Command 0xE1
sets the draw mode; bits 5-6 of that word are the rate.

Reads no bit operations -- Redux's Lua is not guaranteed to have them,
so this divides instead.

One timing caveat. The default breakpoint is the render function's
entry, so the table it walks is the one the previous frame built and the
write pointer has not yet advanced over this frame's packets. That is
the same one-frame lag magic_probe.py documents, and it is harmless for
a rate that holds all animation -- but point --at past the draw if you
need a frame's own packets attributed to it.

See docs/magic-probe.md for the loop this plugs into, and note the
residency guard: all seven magic overlays load at 0x801B0000.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from magic_probe import (  # noqa: E402
    MAGIC_VRAM_START,
    RAM_MASK,
    load_symbols,
    overlay_head,
    parse_records,
    resolve_address,
)
from redux_lua import DEFAULT_HOST, eval_lua  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parent.parent
G_CDB = 0x801517C0
OT_OFFSET = 0x70
OT_BUCKETS = 0x1000
OT_TERMINATOR = 0xFFFFFF
PAGE_SPAN = 0x20000
DEFAULT_CAP = 20000
LUA_TABLE = "FF7Blend"

# GP0(0xE1) bits 5-6, and the same field inside a textured primitive's
# texpage halfword.
RATE_NAMES = {
    0: "B/2+F/2",
    1: "B+F",
    2: "B-F",
    3: "B+F/4",
}


def build_arm_script(
    address: int,
    head: bytes,
    page: int | None,
    page_ptr: int | None,
    cap: int,
) -> str:
    """Lua installing the residency-guarded ordering-table walk.

    With page and page_ptr, packets landing inside the overlay's own
    primitive page are counted separately from the rest of the scene.
    """
    head_list = ",".join(str(b) for b in head)
    if page is None:
        attribute = """
    local function mine(addr) return false end"""
    else:
        # Both pages, not the live one: the overlay flips between them
        # and the write pointer only bounds the page in use this frame.
        attribute = f"""
    local pageLo = {page & RAM_MASK}
    local pageHi = pageLo + {PAGE_SPAN}
    local function mine(addr)
      return addr >= pageLo and addr < pageHi
    end"""
    return f"""
{LUA_TABLE} = {LUA_TABLE} or {{}}
{LUA_TABLE}.log = {{}}
{LUA_TABLE}.rejected = 0
{LUA_TABLE}.head = {{{head_list}}}
local function u32(mem, addr)
  return mem[addr] + mem[addr + 1] * 256 + mem[addr + 2] * 65536
      + mem[addr + 3] * 16777216
end
local function resident(mem)
  for i = 1, #{LUA_TABLE}.head do
    if mem[{MAGIC_VRAM_START & RAM_MASK} + i - 1] ~= {LUA_TABLE}.head[i] then
      return false
    end
  end
  return true
end
local function tally(counts, key)
  counts[key] = (counts[key] or 0) + 1
end
local function render(counts)
  local keys = {{}}
  for key in pairs(counts) do keys[#keys+1] = key end
  table.sort(keys)
  local out = {{}}
  for _, key in ipairs(keys) do
    out[#out+1] = string.format('%d:%d', key, counts[key])
  end
  return table.concat(out, ',')
end
local function renderHex(counts)
  local keys = {{}}
  for key in pairs(counts) do keys[#keys+1] = key end
  table.sort(keys)
  local out = {{}}
  for _, key in ipairs(keys) do
    out[#out+1] = string.format('%02X:%d', key, counts[key])
  end
  return table.concat(out, ',')
end
if {LUA_TABLE}.bp ~= nil then {LUA_TABLE}.bp:remove() end
{LUA_TABLE}.bp = PCSX.addBreakpoint({address}, 'Exec', 4, 'ff7blend',
  function()
    local mem = PCSX.getMemPtr()
    if not resident(mem) then
      {LUA_TABLE}.rejected = {LUA_TABLE}.rejected + 1
      return true
    end
    local ot = u32(mem, {G_CDB & RAM_MASK}) % 2097152 + {OT_OFFSET}{attribute}
    local seen = 0
    local abe = 0
    local ours = 0
    local oursAbe = 0
    local rates = {{}}
    local tpRates = {{}}
    local codes = {{}}
    local ourCodes = {{}}
    -- The table is reverse-ordered: entry i chains to entry i-1, so the
    -- head is the last bucket and one descent covers every packet in
    -- draw order. Walking per bucket re-walks the whole list each time.
    local p = u32(mem, ot + {OT_BUCKETS - 1} * 4) % 16777216
    do
      while p ~= {OT_TERMINATOR} and p ~= 0 and seen < {cap} do
        local at = p % 2097152
        local tag = u32(mem, at)
        local len = math.floor(tag / 16777216)
        if len > 0 then
          local word = u32(mem, at + 4)
          local code = math.floor(word / 16777216)
          tally(codes, code)
          if code == 225 then
            tally(rates, math.floor(word / 32) % 4)
          end
          -- textured four-vertex polygons carry a texpage halfword in
          -- the upper half of the word holding uv1
          if code >= 44 and code <= 47 and len >= 8 then
            local uv1 = u32(mem, at + 20)
            tally(tpRates, math.floor(uv1 / 65536 / 32) % 4)
          end
          if math.floor(code / 2) % 2 == 1 then
            abe = abe + 1
          end
          if mine(at) then
            ours = ours + 1
            tally(ourCodes, code)
            if math.floor(code / 2) % 2 == 1 then
              oursAbe = oursAbe + 1
            end
          end
        end
        seen = seen + 1
        p = tag % 16777216
      end
    end
    local parts = {{}}
    parts[#parts+1] = 'pkts=' .. seen
    parts[#parts+1] = 'abe=' .. abe
    parts[#parts+1] = 'ovl=' .. ours
    parts[#parts+1] = 'ovlabe=' .. oursAbe
    parts[#parts+1] = 'e1=' .. render(rates)
    parts[#parts+1] = 'tp=' .. render(tpRates)
    parts[#parts+1] = 'codes=' .. renderHex(codes)
    parts[#parts+1] = 'ovlcodes=' .. renderHex(ourCodes)
    {LUA_TABLE}.log[#{LUA_TABLE}.log+1] = table.concat(parts, ' ')
    return true
  end, 'ff7blend')
return 'armed at ' .. string.format('0x%08X', {address})
"""


def build_dump_script(
    address: int,
    head: bytes,
    ot_bytes: int,
    page: int,
    page_bytes: int,
    frames: int,
) -> str:
    """Lua dumping the raw ordering table and primitive page, once.

    The chain convention is easier to get right offline against real
    bytes than by guessing inside a breakpoint callback, so this records
    the table verbatim for the first few resident frames.
    """
    head_list = ",".join(str(b) for b in head)
    return f"""
{LUA_TABLE} = {LUA_TABLE} or {{}}
{LUA_TABLE}.log = {{}}
{LUA_TABLE}.rejected = 0
{LUA_TABLE}.head = {{{head_list}}}
local function u32(mem, addr)
  return mem[addr] + mem[addr + 1] * 256 + mem[addr + 2] * 65536
      + mem[addr + 3] * 16777216
end
local function hex(mem, addr, size)
  local out = {{}}
  for i = 0, size - 1 do
    out[#out+1] = string.format('%02X', mem[addr + i])
  end
  return table.concat(out)
end
local function resident(mem)
  for i = 1, #{LUA_TABLE}.head do
    if mem[{MAGIC_VRAM_START & RAM_MASK} + i - 1] ~= {LUA_TABLE}.head[i] then
      return false
    end
  end
  return true
end
if {LUA_TABLE}.bp ~= nil then {LUA_TABLE}.bp:remove() end
{LUA_TABLE}.bp = PCSX.addBreakpoint({address}, 'Exec', 4, 'ff7blend',
  function()
    local mem = PCSX.getMemPtr()
    if not resident(mem) then
      {LUA_TABLE}.rejected = {LUA_TABLE}.rejected + 1
      return true
    end
    if #{LUA_TABLE}.log >= {frames} then return true end
    local ot = u32(mem, {G_CDB & RAM_MASK}) % 2097152 + {OT_OFFSET}
    local parts = {{}}
    parts[#parts+1] = string.format('otbase=%08X', ot)
    parts[#parts+1] = 'ot=' .. hex(mem, ot, {ot_bytes})
    parts[#parts+1] = 'page=' .. hex(mem, {page & RAM_MASK}, {page_bytes})
    {LUA_TABLE}.log[#{LUA_TABLE}.log+1] = table.concat(parts, ' ')
    return true
  end, 'ff7blend')
return 'dumping at ' .. string.format('0x%08X', {address})
"""


def build_drain_script() -> str:
    """Lua returning the collected records and clearing the buffer."""
    return f"""
if {LUA_TABLE} == nil or {LUA_TABLE}.log == nil then return '' end
local text = table.concat({LUA_TABLE}.log, '\\n')
{LUA_TABLE}.log = {{}}
return text
"""


def build_rejected_script() -> str:
    """Lua returning how many hits the residency guard dropped."""
    return f"""
if {LUA_TABLE} == nil then return '0' end
return tostring({LUA_TABLE}.rejected or 0)
"""


def build_disarm_script() -> str:
    return f"""
if {LUA_TABLE} ~= nil and {LUA_TABLE}.bp ~= nil then
  {LUA_TABLE}.bp:remove()
  {LUA_TABLE}.bp = nil
end
return 'disarmed'
"""


def parse_tally(field: str) -> dict[int, int]:
    """Turn a `key:count,key:count` field into a dict."""
    counts: dict[int, int] = {}
    for entry in field.split(","):
        key, _, count = entry.partition(":")
        if key:
            counts[int(key)] = int(count)
    return counts


def summarize(records: list[dict[str, str]]) -> list[str]:
    """One human line per distinct rate seen, plus the overlay's codes."""
    rates: dict[int, int] = {}
    tp_rates: dict[int, int] = {}
    overlay_codes: dict[int, int] = {}
    overlay_abe = 0
    overlay_packets = 0
    for record in records:
        for key, count in parse_tally(record.get("e1", "")).items():
            rates[key] = rates.get(key, 0) + count
        for key, count in parse_tally(record.get("tp", "")).items():
            tp_rates[key] = tp_rates.get(key, 0) + count
        for entry in record.get("ovlcodes", "").split(","):
            key, _, count = entry.partition(":")
            if key:
                code = int(key, 16)
                overlay_codes[code] = overlay_codes.get(code, 0) + int(count)
        overlay_abe += int(record.get("ovlabe", 0))
        overlay_packets += int(record.get("ovl", 0))
    lines = [f"{len(records)} frames"]
    if not rates:
        lines.append("no draw-mode (0xE1) command in the ordering table --")
        lines.append("the rate is set outside it; see --help")
    for key in sorted(rates):
        name = RATE_NAMES[key]
        lines.append(f"draw mode rate {key} ({name}): {rates[key]} commands")
    for key in sorted(tp_rates):
        name = RATE_NAMES[key]
        lines.append(f"texpage rate {key} ({name}): {tp_rates[key]} packets")
    if overlay_packets:
        codes = ", ".join(
            f"0x{code:02X} x{count}"
            for code, count in sorted(overlay_codes.items())
        )
        lines.append(
            f"overlay packets: {overlay_packets}, "
            f"{overlay_abe} semi-transparent ({codes})"
        )
    return lines


def default_page_symbols(
    overlay: str,
    symbols: dict[str, int],
) -> tuple[int | None, int | None]:
    """Guess `<Overlay>PrimBuffer` / `<Overlay>BufferPtr` if they exist."""
    name = overlay.capitalize()
    page = symbols.get(f"{name}PrimBuffer")
    page_ptr = symbols.get(f"{name}BufferPtr")
    if page is None or page_ptr is None:
        return None, None
    return page, page_ptr


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("overlay", help="overlay name from config/us.yaml")
    parser.add_argument(
        "action",
        choices=("arm", "dump", "drain", "disarm"),
    )
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument(
        "--at",
        metavar="SYMBOL",
        help="render function to break on; defaults to vram_start + 0x20",
    )
    parser.add_argument(
        "--page",
        metavar="SYMBOL",
        help="the overlay's primitive page, for attributing packets to it;"
             " defaults to <Overlay>PrimBuffer",
    )
    parser.add_argument(
        "--page-ptr",
        metavar="SYMBOL",
        help="the overlay's write pointer; defaults to <Overlay>BufferPtr",
    )
    parser.add_argument(
        "--cap",
        type=int,
        default=DEFAULT_CAP,
        help="packets to walk per frame before giving up on a chain",
    )
    parser.add_argument(
        "--frames",
        type=int,
        default=2,
        help="frames to capture in dump mode",
    )
    parser.add_argument(
        "--page-bytes",
        type=lambda text: int(text, 0),
        default=0x2000,
        help="bytes of the primitive page to dump",
    )
    parser.add_argument("--out", type=Path)
    parser.add_argument(
        "--raw",
        action="store_true",
        help="print the drained records as JSON instead of a summary",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    symbols = load_symbols(REPO_ROOT / "config")

    if args.action == "disarm":
        print(eval_lua(source=build_disarm_script(), host=args.host))
        return 0

    if args.action == "drain":
        text = eval_lua(source=build_drain_script(), host=args.host)
        if text.startswith("error"):
            print(text, file=sys.stderr)
            return 1
        rejected = eval_lua(source=build_rejected_script(), host=args.host)
        records = parse_records(text)
        if args.raw:
            payload = "\n".join(json.dumps(record) for record in records)
        else:
            payload = "\n".join(summarize(records))
        if args.out is not None:
            args.out.write_text(payload + "\n" if payload else "")
        else:
            print(payload)
        print(
            f"{len(records)} records, {rejected.strip()} hits rejected "
            f"by the residency guard",
            file=sys.stderr,
        )
        return 0

    if args.page is not None:
        page = resolve_address(args.overlay, args.page, symbols)
        page_ptr = resolve_address(args.overlay, args.page_ptr, symbols)
    else:
        page, page_ptr = default_page_symbols(args.overlay, symbols)
    if page is None:
        print(
            f"no primitive page symbol for {args.overlay}; packets will not"
            " be attributed to the overlay",
            file=sys.stderr,
        )
    if args.action == "dump":
        if page is None:
            raise SystemExit("dump needs --page for this overlay")
        script = build_dump_script(
            address=resolve_address(args.overlay, args.at, symbols),
            head=overlay_head(args.overlay, REPO_ROOT / "build" / "us"),
            ot_bytes=OT_BUCKETS * 4,
            page=page,
            page_bytes=args.page_bytes,
            frames=args.frames,
        )
        print(eval_lua(source=script, host=args.host))
        return 0

    script = build_arm_script(
        address=resolve_address(args.overlay, args.at, symbols),
        head=overlay_head(args.overlay, REPO_ROOT / "build" / "us"),
        page=page,
        page_ptr=page_ptr,
        cap=args.cap,
    )
    print(eval_lua(source=script, host=args.host))
    return 0


if __name__ == "__main__":
    sys.exit(main())

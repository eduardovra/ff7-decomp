"""Establish what a bit in the model renderer's flag word does, by forcing
the word to a chosen value and measuring the packets that come out.

Reading func_800D29D4 says which bits it tests; it does not prove what a
bit does to a drawn model. This runs the experiment on brizad, whose
descriptor is static data at a known address:

  * the flag word is rewritten at the top of every render frame, so the
    value survives the overlay reload that begins each cast;
  * two values alternate frame by frame, so both arms of the comparison
    happen inside one cast rather than across two;
  * --freeze pins the effect's animation frame, so consecutive frames are
    identical apart from the flag word.

With all three, a difference between adjacent records is caused by the
bit and nothing else. The GTE rotation matrix is logged beside each
record because the mirror bits act on it directly -- negating a matrix
column is visible there even when the model is symmetric about that
plane and the silhouette does not move.

    python3 tools/flag_probe.py arm 0xA8 0xA9 --freeze 8
    # drive the party into a cast, e.g. tools/pad_input.py CROSS 8
    python3 tools/flag_probe.py drain
    python3 tools/flag_probe.py disarm
"""
import argparse
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

# brizad: entry of BrizadRenderIce, and the point after func_800D29D4 has
# linked this frame's packets. The ordering table is only complete there --
# at the next frame's entry the table has been cleared and rebuilt.
RENDER_ENTRY = 0x801B0000
AFTER_DRAW = 0x801B0174
FLAGS = 0x1B1008  # BrizadRenderDesc + 4
PRIM_PAGE = 0x1B1014
PAGE_SPAN = 0x20000
EFFECT_SLOTS = 0x162978
EFFECT_SLOT_INDEX = 0x15169C
EFFECT_SLOT_STRIDE = 0x20
ANIMATION_FRAME = 2  # offset in BrizadData
G_CDB = 0x1517C0
OT_OFFSET = 0x70
OT_BUCKETS = 0x1000
OT_TERMINATOR = 0xFFFFFF
WALK_CAP = 20000
# first 8 bytes of build/us/brizad.exe, the residency guard: every magic
# overlay loads at 0x801B0000 and enemies cast during the capture
HEAD = (0xB0, 0xFF, 0xBD, 0x27, 0x15, 0x80, 0x02, 0x3C)

ARM = """
FF7Flags = FF7Flags or {{}}
FF7Flags.log = {{}}
FF7Flags.rejected = 0
FF7Flags.value = {value}
FF7Flags.alt = {alt}
FF7Flags.freeze = {freeze}
FF7Flags.parity = 0
local HEAD = {{{head}}}
local function u32(mem, addr)
  return mem[addr] + mem[addr + 1] * 256 + mem[addr + 2] * 65536
      + mem[addr + 3] * 16777216
end
local function s16(mem, addr)
  local v = mem[addr] + mem[addr + 1] * 256
  if v >= 32768 then return v - 65536 end
  return v
end
local function resident(mem)
  for i = 1, #HEAD do
    if mem[0x1B0000 + i - 1] ~= HEAD[i] then return false end
  end
  return true
end

if FF7Flags.entry ~= nil then FF7Flags.entry:remove() end
FF7Flags.entry = PCSX.addBreakpoint({entry}, 'Exec', 4, 'ff7flagsSet',
  function()
    local mem = PCSX.getMemPtr()
    if not resident(mem) then
      FF7Flags.rejected = FF7Flags.rejected + 1
      return true
    end
    if FF7Flags.freeze >= 0 then
      local slot = {slots} + u32(mem, {slot_index}) % 256 * {stride}
      mem[slot + {anim}] = FF7Flags.freeze % 256
      mem[slot + {anim} + 1] = 0
    end
    local v = FF7Flags.value
    if FF7Flags.parity == 1 then v = FF7Flags.alt end
    FF7Flags.parity = 1 - FF7Flags.parity
    for i = 0, 3 do
      mem[{flags} + i] = v % 256
      v = math.floor(v / 256)
    end
    return true
  end, 'ff7flagsSet')

if FF7Flags.walk ~= nil then FF7Flags.walk:remove() end
FF7Flags.walk = PCSX.addBreakpoint({after}, 'Exec', 4, 'ff7flagsWalk',
  function()
    local mem = PCSX.getMemPtr()
    if not resident(mem) then return true end
    -- the composed rotation matrix as the GTE holds it: the mirror bits
    -- negate one of its columns, which is the clearest reading of them
    local cp = PCSX.getRegisters().CP2C.r
    local function half(reg, hi)
      local w = cp[reg]
      local v
      if hi then v = math.floor(w / 65536) % 65536 else v = w % 65536 end
      if v >= 32768 then v = v - 65536 end
      return v
    end
    local mtx = string.format('R=[%d %d %d|%d %d %d|%d %d %d]',
        half(0, false), half(0, true), half(1, false),
        half(1, true), half(2, false), half(2, true),
        half(3, false), half(3, true), half(4, false))
    local ot = u32(mem, {gcdb}) % 2097152 + {ot_offset}
    local p = u32(mem, ot + {last_bucket} * 4) % 16777216
    local seen, ours = 0, 0
    local codes = {{}}
    local first = ''
    local n, sumx, sumy = 0, 0, 0
    local minx, maxx, miny, maxy = 9999, -9999, 9999, -9999
    while p ~= {terminator} and p ~= 0 and seen < {cap} do
      local at = p % 2097152
      local tag = u32(mem, at)
      local len = math.floor(tag / 16777216)
      if len > 0 and at >= {page_lo} and at < {page_hi} then
        ours = ours + 1
        local code = math.floor(u32(mem, at + 4) / 16777216)
        codes[code] = (codes[code] or 0) + 1
        if len == 6 then
          if first == '' then
            first = string.format('rgb=%06X/%06X/%06X v=(%d,%d)(%d,%d)(%d,%d)',
                u32(mem, at + 4) % 16777216,
                u32(mem, at + 12) % 16777216,
                u32(mem, at + 20) % 16777216,
                s16(mem, at + 8), s16(mem, at + 10),
                s16(mem, at + 16), s16(mem, at + 18),
                s16(mem, at + 24), s16(mem, at + 26))
          end
          for _, off in ipairs({{8, 16, 24}}) do
            local x = s16(mem, at + off)
            local y = s16(mem, at + off + 2)
            n = n + 1
            sumx = sumx + x
            sumy = sumy + y
            if x < minx then minx = x end
            if x > maxx then maxx = x end
            if y < miny then miny = y end
            if y > maxy then maxy = y end
          end
        end
      end
      seen = seen + 1
      p = tag % 16777216
    end
    local ck = {{}}
    for k, v in pairs(codes) do ck[#ck+1] = string.format('%02X:%d', k, v) end
    table.sort(ck)
    local geom = 'n=0'
    if n > 0 then
      geom = string.format('n=%d cx=%d cy=%d x=[%d,%d] y=[%d,%d]',
          n, math.floor(sumx / n), math.floor(sumy / n),
          minx, maxx, miny, maxy)
    end
    FF7Flags.log[#FF7Flags.log+1] = string.format(
        'flags=%03X pkts=%d %s %s codes=%s %s',
        u32(mem, {flags}), ours, geom, first, table.concat(ck, ','), mtx)
    return true
  end, 'ff7flagsWalk')
return 'armed, alternating 0x' .. string.format('%X', {value})
    .. ' / 0x' .. string.format('%X', {alt})
"""

DRAIN = """
if FF7Flags == nil then return 'not armed' end
local out = {}
for i = 1, #FF7Flags.log do out[#out+1] = FF7Flags.log[i] end
out[#out+1] = 'rejected=' .. FF7Flags.rejected
FF7Flags.log = {}
return table.concat(out, '\\n')
"""

DISARM = """
if FF7Flags == nil then return 'not armed' end
if FF7Flags.entry ~= nil then FF7Flags.entry:remove() end
if FF7Flags.walk ~= nil then FF7Flags.walk:remove() end
FF7Flags = nil
return 'disarmed'
"""


def lua(source: str) -> str:
    done = subprocess.run(
        [str(REPO_ROOT / ".venv/bin/python3"), str(REPO_ROOT / "tools/redux_lua.py")],
        input=source,
        capture_output=True,
        text=True,
        cwd=REPO_ROOT,
    )
    return (done.stdout or done.stderr).strip()


def arm(
    value: int,
    alt: int,
    freeze: int,
) -> str:
    return lua(ARM.format(
        value=value,
        alt=alt,
        freeze=freeze,
        head=",".join(str(b) for b in HEAD),
        entry=RENDER_ENTRY,
        after=AFTER_DRAW,
        flags=FLAGS,
        slots=EFFECT_SLOTS,
        slot_index=EFFECT_SLOT_INDEX,
        stride=EFFECT_SLOT_STRIDE,
        anim=ANIMATION_FRAME,
        page_lo=PRIM_PAGE,
        page_hi=PRIM_PAGE + PAGE_SPAN,
        gcdb=G_CDB,
        ot_offset=OT_OFFSET,
        last_bucket=OT_BUCKETS - 1,
        terminator=OT_TERMINATOR,
        cap=WALK_CAP,
    ))


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("arm", "drain", "disarm"))
    parser.add_argument(
        "value",
        nargs="?",
        type=lambda text: int(text, 0),
        help="flag word for even frames",
    )
    parser.add_argument(
        "alt",
        nargs="?",
        type=lambda text: int(text, 0),
        help="flag word for odd frames; defaults to value",
    )
    parser.add_argument(
        "--freeze",
        type=int,
        default=-1,
        help="pin the effect's animation frame; -1 lets it run",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.action == "drain":
        print(lua(DRAIN))
        return 0
    if args.action == "disarm":
        print(lua(DISARM))
        return 0
    if args.value is None:
        raise SystemExit("arm needs a flag value")
    alt = args.value
    if args.alt is not None:
        alt = args.alt
    print(arm(value=args.value, alt=alt, freeze=args.freeze))
    return 0


if __name__ == "__main__":
    sys.exit(main())

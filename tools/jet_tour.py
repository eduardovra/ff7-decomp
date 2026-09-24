"""Capture jet objects of chosen types where the ride spawns them.

From a save state inside jet, this reads the level's spawn table, then
replays with no input: it holds g_JetSpeed high until shortly before each
target type's first spawn segment, drops to a normal speed and captures the
frame at a few offsets. Nothing is hidden, so the object pool behaves as in
play. Each capture is written as a PNG with the target type's objects boxed.

    python3 tools/jet_tour.py --state jet-start 0 2 4 14
    python3 tools/jet_tour.py --state jet-start 4 --offsets 5,10,20,40

See docs/jet-probe.md.
"""

from __future__ import annotations

import argparse
import sys
import time
from pathlib import Path
from urllib.request import urlopen

sys.path.insert(0, str(Path(__file__).resolve().parent))

from redux_lua import eval_lua  # noqa: E402
from vram_png import decode_screenshot, write_png  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parent.parent
HOST = "http://localhost:8080"
FRAME_HOOK = 0x800A46E8
GAME_STATE = 0x9C560
GAMESTATE_JET = 11
PAUSED = 0xD16DC
SPEED = 0xA897C
SEGMENT = 0xD16E0
POOL = 0xD1DC0
POOL_STRIDE = 0x13C
SPAWNS = 0x800D1C0C
SPAWN_COUNTS = 0x800D1C10
TRACK_LENGTH = 0x800D1724
FAST_SPEED = 0x40000
NORMAL_SPEED = 0x4000
LEAD_SEGMENTS = 10
WIDTH, HEIGHT = 320, 240
BOX_COLOR = bytes((255, 0, 255))

LUA_HELPERS = """
local m = PCSX.getMemPtr()
local function u32(a)
  a = bit.band(a, 0x1FFFFF)
  return m[a] + m[a+1] * 256 + m[a+2] * 65536 + m[a+3] * 16777216
end
local function s32(a)
  local v = u32(a)
  if v >= 2^31 then v = v - 2^32 end
  return v
end
local function s16(a)
  a = bit.band(a, 0x1FFFFF)
  local v = m[a] + m[a+1] * 256
  if v >= 32768 then v = v - 65536 end
  return v
end
local function w32(a, v)
  for i = 0, 3 do m[a + i] = math.floor(v / 256^i) % 256 end
end
"""

SPAWN_TABLE_SCRIPT = LUA_HELPERS + f"""
local spawns, counts = u32(0x{SPAWNS:08X}), u32(0x{SPAWN_COUNTS:08X})
local out, index = {{}}, 0
for segment = 0, math.floor(u32(0x{TRACK_LENGTH:08X}) / 4) - 1 do
  for i = 1, m[bit.band(counts + segment, 0x1FFFFF)] do
    local entry = spawns + index * 0x60
    out[#out+1] = segment .. ' ' .. s16(entry) .. ' ' .. s16(entry + 4)
    index = index + 1
  end
end
return table.concat(out, '\\n')
"""


def build_tour_script(
    stops: list[int],
    offsets: list[int],
    out_dir: Path,
) -> str:
    stop_list = ",".join(map(str, stops))
    offset_list = ",".join(map(str, offsets))
    return LUA_HELPERS + f"""
if FF7Tour and FF7Tour.bp then FF7Tour.bp:remove() end
FF7Tour = {{done = false, frames = 0, shots = {{}}, phase = 1}}
local stops, offsets = {{{stop_list}}}, {{{offset_list}}}
FF7Tour.bp = PCSX.addBreakpoint(0x{FRAME_HOOK:08X}, 'Exec', 4, 'tour',
function()
  if m[0x{GAME_STATE:X}] ~= {GAMESTATE_JET} then return true end
  local f = FF7Tour.frames + 1
  FF7Tour.frames = f
  local capture = false
  if FF7Tour.arrived == nil then
    if s32(0x{SEGMENT:X}) < stops[FF7Tour.phase] then
      w32(0x{SPEED:X}, {FAST_SPEED})
    else
      FF7Tour.arrived = f
      w32(0x{SPEED:X}, {NORMAL_SPEED})
    end
  else
    local since = f - FF7Tour.arrived
    for _, o in ipairs(offsets) do capture = capture or since == o end
    if since >= offsets[#offsets] then
      FF7Tour.phase = FF7Tour.phase + 1
      FF7Tour.arrived = nil
    end
  end
  if capture then
    local rows = {{}}
    for i = 0, 99 do
      local o = 0x{POOL:X} + i * 0x{POOL_STRIDE:X}
      if s16(o + 0xDA) ~= 0 then
        local pts = {{}}
        for k = 0, 5 do
          pts[#pts+1] = s16(o + 0x11C + k*4) .. ',' .. s16(o + 0x11E + k*4)
        end
        rows[#rows+1] = s32(o + 0x28) .. ' ' .. s32(o + 0x30) .. ' '
          .. table.concat(pts, ' ')
      end
    end
    local ss = PCSX.GPU.takeScreenShot()
    local path = string.format('{out_dir}/f%05d.raw', f)
    local fh = Support.File.open(path, 'TRUNCATE')
    fh:writeMoveSlice(ss.data)
    fh:close()
    FF7Tour.shots[#FF7Tour.shots+1] = f .. '|' .. s32(0x{SEGMENT:X}) .. '|'
      .. table.concat(rows, ';')
  end
  if FF7Tour.phase > #stops then
    m[0x{PAUSED:X}] = 1
    FF7Tour.done = true
    FF7Tour.bp = nil
    return false
  end
  return true
end, 'tour')
m[0x{PAUSED:X}] = 0
return 'armed'
"""


def draw_box(
    rgb: bytearray,
    xs: list[int],
    ys: list[int],
) -> None:
    def put(x: int, y: int) -> None:
        if 0 <= x < WIDTH and 0 <= y < HEIGHT:
            at = (y * WIDTH + x) * 3
            rgb[at:at + 3] = BOX_COLOR

    for x in range(min(xs), max(xs) + 1):
        put(x, min(ys))
        put(x, max(ys))
    for y in range(min(ys), max(ys) + 1):
        put(min(xs), y)
        put(max(xs), y)


def annotate(
    shot: str,
    targets: set[int],
    out_dir: Path,
) -> str | None:
    frame, segment, rows = shot.split("|")
    raw = (out_dir / f"f{int(frame):05d}.raw").read_bytes()
    rgb = decode_screenshot(raw, WIDTH, HEIGHT)
    found = []
    for row in filter(None, rows.split(";")):
        kind, model, *points = row.split()
        if int(kind) not in targets:
            continue
        xy = [tuple(map(int, p.split(","))) for p in points]
        xs = [x for x, _ in xy]
        ys = [y for _, y in xy]
        visible = max(xs) >= 0 and min(xs) < WIDTH
        visible = visible and max(ys) >= 0 and min(ys) < HEIGHT
        if visible and max(xs) - min(xs) < WIDTH:
            draw_box(rgb=rgb, xs=xs, ys=ys)
            found.append(f"t{kind}/m{model}@{min(xs)},{min(ys)}")
    if not found:
        return None
    png = out_dir / f"f{int(frame):05d}.png"
    write_png(png, rgb, WIDTH, HEIGHT)
    return f"frame {frame} seg {segment}: {' '.join(found)} -> {png}"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("types", type=int, nargs="+")
    parser.add_argument("--state", required=True, help="Redux save-state name")
    parser.add_argument("--offsets", default="20,60,100,140")
    parser.add_argument(
        "--out",
        type=Path,
        default=REPO_ROOT / "build" / "jet_tour",
    )
    args = parser.parse_args()
    offsets = [int(o) for o in args.offsets.split(",")]
    args.out.mkdir(parents=True, exist_ok=True)
    out_dir = args.out.resolve()

    urlopen(f"{HOST}/api/v1/state/load?name={args.state}").read()
    table = [line.split() for line in eval_lua(SPAWN_TABLE_SCRIPT).splitlines()]
    now = int(eval_lua(LUA_HELPERS + f"return s32(0x{SEGMENT:X})"))
    first: dict[int, int] = {}
    for segment, kind, _model in table:
        if int(segment) > now and int(kind) not in first:
            first[int(kind)] = int(segment)
    stops = sorted(first[t] - LEAD_SEGMENTS for t in args.types if t in first)
    missing = [t for t in args.types if t not in first]
    if missing:
        print(f"no spawn after segment {now} for types {missing}")
    if not stops:
        return 1

    script = build_tour_script(stops=stops, offsets=offsets, out_dir=out_dir)
    print(eval_lua(script))
    while eval_lua("return tostring(FF7Tour.done)") != "true":
        time.sleep(2)
    shots = eval_lua("return table.concat(FF7Tour.shots, '\\n')")
    for shot in shots.splitlines():
        line = annotate(shot=shot, targets=set(args.types), out_dir=out_dir)
        if line is not None:
            print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main())

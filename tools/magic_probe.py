"""Capture per-frame data from a magic overlay running in PCSX-Redux.

Polling cannot do this. A RAM snapshot is all 2MB and takes ~34ms, which
against a 30fps game is one sample per frame -- too coarse for a 16-frame
animation, and not fixable client-side. So the sampling happens inside the
emulator: a Lua Exec breakpoint on the render function fires exactly once
per frame and records what it sees, and this module installs it, drains it
and parses the result.

The invoker must return true. Returning false deletes the breakpoint, so
it fires exactly once -- which looks like a spell that only rendered one
frame rather than like a bug in the probe.

Every record is residency-guarded. All seven magic overlays load at
0x801B0000 and only one is resident at a time -- MabariaRenderModel and
ThunderRenderModel share the address 0x801B0020 -- so the callback compares
RAM against the built overlay before recording anything.

See docs/magic-probe.md.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from redux_lua import DEFAULT_HOST, eval_lua  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parent.parent
MAGIC_VRAM_START = 0x801B0000
RAM_MASK = 0x1FFFFF
RESIDENCY_BYTES = 16
EFFECT_SLOT_STRIDE = 0x20
SYMBOL_RE = re.compile(r"^\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+)\s*;")
LUA_TABLE = "FF7Probe"


def load_symbols(config_dir: Path) -> dict[str, int]:
    """Map symbol name to address across every config/*.txt symbol file."""
    symbols: dict[str, int] = {}
    for path in sorted(config_dir.glob("*.txt")):
        for line in path.read_text(errors="replace").splitlines():
            match = SYMBOL_RE.match(line)
            if match is not None:
                symbols[match.group(1)] = int(match.group(2), 16)
    return symbols


@dataclass
class Watch:
    """One named region of RAM to record on every breakpoint hit."""

    name: str
    addr: int
    size: int


def build_arm_script(
    address: int,
    head: bytes,
    watches: list[Watch],
    shots_dir: Path | None = None,
) -> str:
    """Lua that installs the residency-guarded recording breakpoint.

    With shots_dir, each hit also writes the framebuffer as raw XBGR1555.
    The capture is the last completed frame, so it carries the same
    one-frame lag as the memory read at the function's entry.
    """
    head_list = ",".join(str(b) for b in head)
    reads = []
    for watch in watches:
        offset = watch.addr & RAM_MASK
        reads.append(
            f"    parts[#parts+1] = '{watch.name}='"
            f" .. hex(mem, {offset}, {watch.size})"
        )
    body = "\n".join(reads)
    shot = ""
    if shots_dir is not None:
        shot = f"""
    local ss = PCSX.GPU.takeScreenShot()
    local name = string.format('{shots_dir}/%04d.raw', #{LUA_TABLE}.log)
    local fh = Support.File.open(name, 'TRUNCATE')
    fh:writeMoveSlice(ss.data)
    fh:close()
    parts[#parts+1] = string.format('shot=%dx%d', ss.width, ss.height)"""
    return f"""
{LUA_TABLE} = {LUA_TABLE} or {{}}
{LUA_TABLE}.log = {{}}
{LUA_TABLE}.rejected = 0
{LUA_TABLE}.head = {{{head_list}}}
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
{LUA_TABLE}.bp = PCSX.addBreakpoint({address}, 'Exec', 4, 'ff7probe',
  function()
    local mem = PCSX.getMemPtr()
    if not resident(mem) then
      {LUA_TABLE}.rejected = {LUA_TABLE}.rejected + 1
      return true
    end
    local parts = {{}}
{body}{shot}
    {LUA_TABLE}.log[#{LUA_TABLE}.log+1] = table.concat(parts, ' ')
    return true
  end, 'ff7probe')
return 'armed at ' .. string.format('0x%08X', {address})
"""


def build_rejected_script() -> str:
    """Lua returning how many hits the residency guard dropped."""
    return f"""
if {LUA_TABLE} == nil then return '0' end
return tostring({LUA_TABLE}.rejected or 0)
"""


def build_drain_script() -> str:
    """Lua that returns the collected records and clears the buffer."""
    return f"""
if {LUA_TABLE} == nil or {LUA_TABLE}.log == nil then return '' end
local text = table.concat({LUA_TABLE}.log, '\\n')
{LUA_TABLE}.log = {{}}
return text
"""


def build_disarm_script() -> str:
    return f"""
if {LUA_TABLE} ~= nil and {LUA_TABLE}.bp ~= nil then
  {LUA_TABLE}.bp:remove()
  {LUA_TABLE}.bp = nil
end
return 'disarmed'
"""


def parse_records(text: str) -> list[dict[str, str]]:
    """Turn drained `name=HEX name=HEX` lines into dicts."""
    records = []
    for line in text.splitlines():
        line = line.strip()
        if not line:
            continue
        record = {}
        for field in line.split(" "):
            name, _, value = field.partition("=")
            if name:
                record[name] = value
        records.append(record)
    return records


def parse_watch(
    spec: str,
    symbols: dict[str, int],
) -> Watch:
    """Parse `NAME`, `NAME:SIZE` or `0xADDR:SIZE` into a Watch."""
    name, _, size_text = spec.partition(":")
    if name.startswith("0x"):
        addr = int(name, 16)
    elif name in symbols:
        addr = symbols[name]
    else:
        raise SystemExit(f"unknown symbol: {name}")
    if size_text:
        return Watch(name=name, addr=addr, size=int(size_text, 0))
    return Watch(name=name, addr=addr, size=4)


def overlay_head(
    overlay: str,
    build_dir: Path,
) -> bytes:
    return (build_dir / f"{overlay}.exe").read_bytes()[:RESIDENCY_BYTES]


def resolve_address(
    overlay: str,
    symbol: str | None,
    symbols: dict[str, int],
) -> int:
    if symbol is None:
        return MAGIC_VRAM_START + 0x20
    if symbol.startswith("0x"):
        return int(symbol, 16)
    if symbol in symbols:
        return symbols[symbol]
    raise SystemExit(f"unknown symbol: {symbol}")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("overlay", help="overlay name from config/us.yaml")
    parser.add_argument(
        "action",
        choices=("arm", "drain", "disarm"),
    )
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument(
        "--at",
        metavar="SYMBOL",
        help="render function to break on; defaults to vram_start + 0x20",
    )
    parser.add_argument(
        "--watch",
        action="append",
        default=[],
        metavar="NAME[:SIZE]",
    )
    parser.add_argument("--out", type=Path)
    parser.add_argument(
        "--shots",
        type=Path,
        metavar="DIR",
        help="capture the framebuffer on every hit, as raw XBGR1555",
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
        lines = [json.dumps(record) for record in records]
        payload = "\n".join(lines)
        if args.out is not None:
            args.out.write_text(payload + "\n" if payload else "")
        else:
            print(payload)
        print(f"{len(records)} records, {rejected.strip()} hits rejected "
              f"by the residency guard", file=sys.stderr)
        return 0

    watches = [parse_watch(spec, symbols) for spec in args.watch]
    if not watches:
        raise SystemExit("arm needs at least one --watch")
    if args.shots is not None:
        args.shots.mkdir(parents=True, exist_ok=True)
    script = build_arm_script(
        address=resolve_address(args.overlay, args.at, symbols),
        head=overlay_head(args.overlay, REPO_ROOT / "build" / "us"),
        watches=watches,
        shots_dir=args.shots,
    )
    print(eval_lua(source=script, host=args.host))
    return 0


if __name__ == "__main__":
    sys.exit(main())

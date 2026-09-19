"""Record which currentActionId values the game actually dispatches.

battle2.c pairs an overlay-id table with an entrypoint table per
currentActionId, but only five of those ids carry a label and two carry
none at all. Static reading has gone as far as it can: the field is read
18 times in C and never written, so the writer is still in assembly.

This installs four Exec breakpoints and one Write watchpoint:

  BattleDispatchModelLoadImage  the switch that picks the id table
  BattleDispatchModelRunScript  dispatcher for actions 4, 7 and 8
  func_800D0C80                 dispatcher for actions 2, 13, 20 and 32
  BattleLoadOverlaySector       the disc read, logging the sector
  g_BattleModels[i] + 0x23      the write watchpoint, logging the PC

The first three take the acting unit in a0, so the pair at 0x22/0x23 is
already committed when they are entered -- the same assumption
magic_probe's force patch relies on. The watchpoint is only there for
the PC: a write breakpoint fires around the store rather than after it,
so the value it would read back is not trustworthy and is not recorded.

Sector numbers map to MAGIC/*.BIN through the disc directory, so a trace
says which overlay each (action, effect) pair really loaded.

See docs/magic-probe.md for the launch settings, which are not optional.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from redux_lua import DEFAULT_HOST, eval_lua  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parent.parent
BATTLE_ELF = REPO_ROOT / "build/us/battle.elf"
NM = "mipsel-linux-gnu-nm"
RAM_MASK = 0x1FFFFF
BATTLE_VRAM_START = 0x800A0000
BATTLE_MODEL_BASE = 0x801518E4
BATTLE_MODEL_STRIDE = 0xB9C
BATTLE_MODEL_COUNT = 10
COMMAND_ID_OFFSET = 0x22
COMMAND_TYPE_OFFSET = 0x23
MAX_RECORDS = 4000
LUA_TABLE = "FF7Action"
DEFAULT_ISO = REPO_ROOT / "disks/Final Fantasy VII (USA) (Disc 1).iso"
EXTENT_RE = re.compile(r"\[\s*(\d+)\s+0\d\]\s+([A-Z0-9_]+\.BIN);1")
LOAD_RE = re.compile(r"^load lba=(\d+)\b")

UNIT_SITES = (
    ("dispatch-load", "BattleDispatchModelLoadImage"),
    ("dispatch-run", "BattleDispatchModelRunScript"),
    ("dispatch-alt", "func_800D0C80"),
)
SECTOR_SITE = "BattleLoadOverlaySector"


def resolve_sites(elf: Path) -> dict[str, int]:
    """Map the probe's function names to addresses via nm.

    Two of the sites are static, so they exist only in the ELF and not
    in any config/*.txt symbol file.
    """
    wanted = {name for _, name in UNIT_SITES}
    wanted.add(SECTOR_SITE)
    result = subprocess.run(
        [NM, str(elf)],
        capture_output=True,
        text=True,
        check=True,
    )
    found: dict[str, int] = {}
    for line in result.stdout.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] in wanted:
            found[parts[2]] = int(parts[0], 16)
    missing = sorted(wanted - found.keys())
    if missing:
        raise SystemExit(f"not found in {elf}: {', '.join(missing)}")
    return found


def build_arm_script(sites: dict[str, int]) -> str:
    installs = []
    for tag, name in UNIT_SITES:
        installs.append(
            f"add(0x{sites[name]:08X}, 'Exec', 4, unit('{tag}'))"
        )
    installs.append(
        f"add(0x{sites[SECTOR_SITE]:08X}, 'Exec', 4, sector)"
    )
    body = "\n".join(installs)
    watch_base = BATTLE_MODEL_BASE + COMMAND_TYPE_OFFSET
    ram_base = BATTLE_MODEL_BASE & RAM_MASK
    stride = BATTLE_MODEL_STRIDE
    return f"""
{LUA_TABLE} = {LUA_TABLE} or {{}}
{LUA_TABLE}.log = {{}}
{LUA_TABLE}.repeats = {{}}
{LUA_TABLE}.dropped = 0
if {LUA_TABLE}.bps ~= nil then
  for _, bp in ipairs({LUA_TABLE}.bps) do bp:remove() end
end
{LUA_TABLE}.bps = {{}}
-- A dispatcher re-entered while one action plays would otherwise fill
-- the cap with one repeated line, so runs are counted, not appended.
local function record(text)
  local log = {LUA_TABLE}.log
  local repeats = {LUA_TABLE}.repeats
  local last = #log
  if last > 0 and log[last] == text then
    repeats[last] = (repeats[last] or 1) + 1
    return
  end
  if last >= {MAX_RECORDS} then
    {LUA_TABLE}.dropped = {LUA_TABLE}.dropped + 1
    return
  end
  log[last + 1] = text
end
local function add(addr, kind, width, fn)
  local bps = {LUA_TABLE}.bps
  bps[#bps + 1] = PCSX.addBreakpoint(addr, kind, width, 'ff7action',
    fn, 'ff7action')
end
local function unit(tag)
  return function()
    local slot = bit.band(PCSX.getRegisters().GPR.n.a0, 0xFF)
    if slot < {BATTLE_MODEL_COUNT} then
      -- getMemPtr is RAM-relative; addBreakpoint takes the full address.
      local mem = PCSX.getMemPtr()
      local at = 0x{ram_base:X} + slot * 0x{stride:X}
      record(string.format('%s slot=%d action=%d effect=%d', tag, slot,
        mem[at + 0x{COMMAND_TYPE_OFFSET:X}], mem[at + 0x{COMMAND_ID_OFFSET:X}]))
    end
    return true
  end
end
local function sector()
  local gpr = PCSX.getRegisters().GPR.n
  record(string.format('load lba=%d len=%d', gpr.a0, gpr.a1))
  return true
end
-- The watchpoint fires on any write to that RAM, including whoever last
-- reused it -- boot-time LZS output lands there long before it is the
-- battle model table. main lives below 0x800A0000, so requiring a PC at
-- or above it keeps battle and the overlays and drops the rest.
local function writer(slot)
  return function()
    local pc = PCSX.getRegisters().pc
    if pc >= 0x{BATTLE_VRAM_START:08X} then
      record(string.format('write slot=%d pc=%08X', slot, pc))
    end
    return true
  end
end
{body}
for i = 0, {BATTLE_MODEL_COUNT} - 1 do
  local at = 0x{watch_base:08X} + i * 0x{BATTLE_MODEL_STRIDE:X}
  add(at, 'Write', 1, writer(i))
end
return 'armed ' .. tostring(#{LUA_TABLE}.bps) .. ' breakpoints'
"""


def build_dump_script(clear: bool) -> str:
    if clear:
        reset = (
            f"{LUA_TABLE}.log = {{}}\n"
            f"{LUA_TABLE}.repeats = {{}}\n"
            f"{LUA_TABLE}.dropped = 0"
        )
    else:
        reset = ""
    return f"""
if {LUA_TABLE} == nil or {LUA_TABLE}.log == nil then return 'not armed' end
local lines = {{}}
for i, text in ipairs({LUA_TABLE}.log) do
  local n = {LUA_TABLE}.repeats[i]
  if n ~= nil and n > 1 then
    lines[i] = text .. ' x' .. tostring(n)
  else
    lines[i] = text
  end
end
local out = table.concat(lines, '\\n')
local dropped = {LUA_TABLE}.dropped
{reset}
if dropped > 0 then
  out = out .. '\\n-- dropped ' .. tostring(dropped) .. ' records'
end
return out
"""


def build_clear_script() -> str:
    return f"""
if {LUA_TABLE} == nil then return 'not armed' end
if {LUA_TABLE}.bps ~= nil then
  for _, bp in ipairs({LUA_TABLE}.bps) do bp:remove() end
end
{LUA_TABLE}.bps = {{}}
{LUA_TABLE}.log = {{}}
{LUA_TABLE}.dropped = 0
return 'cleared'
"""


def sector_names(iso: Path) -> dict[int, str]:
    """Map each MAGIC/*.BIN extent to its filename, from the disc."""
    result = subprocess.run(
        ["isoinfo", "-i", str(iso), "-l"],
        capture_output=True,
        text=True,
        check=True,
    )
    names: dict[int, str] = {}
    for match in EXTENT_RE.finditer(result.stdout):
        names.setdefault(int(match.group(1)), match.group(2))
    return names


def decode(text: str, iso: Path) -> str:
    """Annotate every `load lba=N` line with the overlay it read."""
    names = sector_names(iso)
    lines = []
    for line in text.splitlines():
        match = LOAD_RE.match(line)
        if match is not None:
            name = names.get(int(match.group(1)), "?")
            line = f"{line}  {name}"
        lines.append(line)
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "command",
        choices=("arm", "dump", "drain", "clear", "decode"),
        help="arm installs the hooks, dump reads the log, drain reads "
        "and empties it, clear removes the hooks, decode annotates a "
        "log read from stdin with overlay filenames",
    )
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--elf", type=Path, default=BATTLE_ELF)
    parser.add_argument("--iso", type=Path, default=DEFAULT_ISO)
    args = parser.parse_args(argv)

    if args.command == "decode":
        print(decode(text=sys.stdin.read(), iso=args.iso))
        return 0

    if args.command == "arm":
        script = build_arm_script(sites=resolve_sites(args.elf))
    elif args.command == "clear":
        script = build_clear_script()
    else:
        script = build_dump_script(clear=args.command == "drain")

    print(eval_lua(source=script, host=args.host))
    return 0


if __name__ == "__main__":
    sys.exit(main())

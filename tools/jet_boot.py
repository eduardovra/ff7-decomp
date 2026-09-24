"""Boot the jet (Speed Square) minigame from any save or a new game.

main's loop seeds g_GameState from Savemap.current_module on entry to
SysInitFieldFromSavemap, and dispatches GAMESTATE_JET to MINI/JET.BIN.
A one-shot breakpoint there writes GAMESTATE_JET into the savemap, so the
next field entry (New Game, or loading a save) loads jet instead.

    python3 tools/jet_boot.py          # arm, then start a game
    python3 tools/jet_boot.py --disarm
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from magic_probe import load_symbols  # noqa: E402
from redux_lua import eval_lua  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parent.parent
RAM_MASK = 0x1FFFFF
SAVEMAP_CURRENT_MODULE = 0xB94
GAMESTATE_JET = 11
LUA_TABLE = "FF7JetBoot"


def build_arm_script(hook: int, current_module: int) -> str:
    offset = current_module & RAM_MASK
    return f"""
if {LUA_TABLE} ~= nil and {LUA_TABLE}.bp ~= nil then {LUA_TABLE}.bp:remove() end
{LUA_TABLE} = {{fired = false}}
{LUA_TABLE}.bp = PCSX.addBreakpoint(0x{hook:08X}, 'Exec', 4, 'jetboot',
  function()
    local mem = PCSX.getMemPtr()
    mem[{offset}] = {GAMESTATE_JET}
    mem[{offset + 1}] = 0
    {LUA_TABLE}.fired = true
    -- returning false deletes the breakpoint, making it one-shot
    {LUA_TABLE}.bp = nil
    return false
  end, 'jetboot')
return 'armed at 0x{hook:08X}'
"""


DISARM_SCRIPT = f"""
if {LUA_TABLE} == nil then return 'not armed' end
if {LUA_TABLE}.bp ~= nil then {LUA_TABLE}.bp:remove() end
local fired = {LUA_TABLE}.fired
{LUA_TABLE} = nil
return 'disarmed, fired=' .. tostring(fired)
"""


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--disarm", action="store_true")
    args = parser.parse_args()
    if args.disarm:
        print(eval_lua(DISARM_SCRIPT))
        return 0
    symbols = load_symbols(REPO_ROOT / "config")
    current_module = symbols["Savemap"] + SAVEMAP_CURRENT_MODULE
    script = build_arm_script(
        hook=symbols["SysInitFieldFromSavemap"],
        current_module=current_module,
    )
    print(eval_lua(script))
    return 0


if __name__ == "__main__":
    sys.exit(main())

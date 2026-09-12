# Porting to PC with PSY-Z

[PSY-Z](https://github.com/Xeeynamo/psyz) is a drop-in replacement for the
PSY-Q runtime that lets PS1 code compile and run natively on Linux, macOS,
Windows, iOS, WebAssembly and PSP. It is by the same author as this repo, so
it is the natural PC target for this decomp.

It is a link-time API replacement, **not** an emulator or a recompiler. It
cannot execute MIPS. Every function still behind `INCLUDE_ASM` on a code path
you want to run has to become C first.

This document scopes the smallest useful target: **boot straight into a battle
and cast a spell**, skipping the title screen, field and world map.

## Why field is not needed

`BATINI_Main(s32 sceneID)` is the battle entry point, and it takes a single
integer. Everything else it reads comes from `Savemap` -- `Savemap.party`,
`Savemap.battle_speed`, `Savemap.config` -- plus kernel text. None of that
comes from field.

`Savemap` has a field-free seeding path that is already decompiled, in
`src/main/btlinit.c`:

```c
void func_80014934(void) {
    func_800148A0();
    func_80014578(INIT_KERNEL, (void*)0x801B0000, 0);
    func_800145BC(0);
    SysGzipPackDecompressById((u8*)0x801B0000, &Savemap.party, KERNEL_INIT);
}
```

The default new-game party is a compressed blob in `KERNEL.BIN`. Field never
computes it. That is the whole reason this shortcut works.

Both entry points are **already decompiled**:

| symbol | address | state |
| --- | --- | --- |
| `BATINI_Main` | `0x801B0050` | C |
| `BATTLE_RunFrame` | `0x800A3354` | C |
| `func_80014934` | -- | C |

Today `main` reaches these indirectly through fixed overlay addresses. In a
native build that indirection disappears and you call them directly, so the
overlay loader becomes dead code rather than a problem to solve.

## The bootstrap

```c
#include <psyz.h>

int main(void) {
    psyz_init();
    func_80014934();          /* seed Savemap from KERNEL.BIN */
    g_BattleMode = 0;         /* normally g_FieldState.battleMode2 */
    BATINI_Main(sceneID);     /* any formation from SCENE.BIN */
    for (;;) {
        BATTLE_RunFrame();
    }
}
```

`g_BattleMode` is the one piece of field state on this path
(`src/main/110B8.c`). Set it to a constant. You need field's *struct
definitions* from `game.h`, never field's *code*.

## What PSY-Z supplies -- do not decompile

`src/main/psxsdk.c` holds **486** of the repo's 1542 `INCLUDE_ASM` stubs, and
they are PSY-Q library functions, not game code: `_SpuInit`, `SpuStart`,
`_spu_*`, plus CD, GPU and GTE entry points. PSY-Z reimplements exactly these.
Decompiling them is wasted effort for a PC port.

Roughly 436 carry recognisable PSY-Q names. The remaining ~50 are unnamed
`func_*` and three `SysMovie*` (FF7's own STR player, which *is* game code).
PSY-Z coverage of each one is unverified -- its README states gaps are
expected.

This matters more than it looks. Of the 14 still-asm functions the fully
decompiled `src/magic/*.c` calls, **10 are PSY-Q** and vanish for free:

    ApplyMatrix     CompMatrix      RotMatrixYXZ    ScaleMatrix    SetFarColor
    SetRotMatrix    SetTransMatrix  rand            rcos           rsin

## Scope

**262 functions**, reached transitively from `BATINI_Main`, `BATTLE_RunFrame`,
`func_80014934` and `src/magic/*.c`, with PSY-Q excluded.

For comparison: 1542 `INCLUDE_ASM` stubs remain repo-wide, 486 of them PSY-Q.
Skipping field, world, menu, mini-games and the ending is what buys the rest.

## Tier 0 -- direct blockers of `BATINI_Main`

Eight functions. Everything else is behind them.

| function | file |
| --- | --- |
| `BattleInitPlayer` | `src/battle/batini.c` |
| `BattleInitEnemyUnits` | `src/battle/batini.c` |
| `BattleRecalcUnitSpeed` | `src/battle/battle.c` |
| `BattleUpdateUnitMasks` | `src/battle/battle.c` |
| `func_800A5BC8` | `src/battle/battle.c` |
| `SysInitPlayerStatFromMateria` | `src/main/17238.c` |
| `SysCalculateTotalLureGilPreemptiveValue` | `src/main/17238.c` |
| `SysInitPlayerStatFromEquip` | `src/main/1F6B4.c` |

## Tier 1 -- what the spell overlays need

All seven magic overlays are already at 100%. Once PSY-Q is discounted they
need just **four** functions, all in `src/battle/battle2.c`:

    BattleGetPartPosition    func_800D29D4    func_800D4368    func_800D4D90

## Running it under gdb

PSY-Z produces an ordinary native ELF, so gdb works normally -- real
breakpoints and watchpoints on game functions, which PCSX-Redux cannot give
you. Note the PC build is a *separate* build from the matching one: modern
clang/gcc with `-g -O0`, not gcc 2.6.3.

```shell
cmake -B build-pc -DCMAKE_BUILD_TYPE=Debug && cmake --build build-pc
gdb ./build-pc/ff7
```

```
(gdb) break BATINI_Main
(gdb) run
(gdb) watch -l Savemap.party          # catch whoever corrupts the party
(gdb) break BATTLE_RunFrame           # then `finish` to step frame by frame
```

`rr record ./ff7` then `rr replay` gives reverse execution, which is the fast
way to find what wrote a bad value in `g_BattleState`.

**You do not have to finish all 262 before debugging.** A function still as
`INCLUDE_ASM` will not link natively at all, so the practical route is to emit
empty stubs for the whole set, link immediately, then replace stubs one at a
time. The build runs (badly) from day one and gdb is available throughout --
far better feedback than waiting for function 262.

## Versus the emulator loop

`./mako.sh redux` and `magic_probe.py` (see `emulator-loop.md` and
`magic-probe.md`) are not replaced by a native build. The two answer
different questions.

### What a native build removes

**The residency guard.** Every magic overlay sits at `0x801B0000` because
PS1 RAM is 2MB, so `MabariaRenderModel` and `ThunderRenderModel` collide at
`0x801B0020` and every sample has to be checked against the built `.exe` to
learn which spell is resident. Linked natively they are ordinary distinct
symbols. No fingerprint compare, no dropped hits, no `drain` count, and no
ambiguity when an enemy casts mid-battle and swaps the overlay underneath a
breakpoint. This is the same root cause as the battle/world `0x800A`
collision in **Traps** below.

**Per-overlay symbol loading.** Redux resets the symbol table and uploads
`main` plus one overlay, because uploading them together collides. gdb holds
battle, field, world and all seven magic overlays at once.

**The whole HTTP and Lua layer.** No ~256-byte URL cap, no chunked append
buffer, no per-response connection teardown, no 34ms full-RAM read to fetch
four bytes. `redux_lua.py` and `redux_probe.lua` stop being needed, along
with the `pcsx.json`-rewritten-on-exit ordering and the rule that the dynarec
and the debugger cannot both be live.

**`--force TYPE:ID`.** Instead of breaking on `func_800D1110` and
`func_800D0C80` to rewrite bytes `0x22`/`0x23` of the acting unit's
`BattleModel` from `a0`, you call the overlay entry point directly.
`sceneID` is already an argument to `BATINI_Main`.

### What a native build adds

| capability | today | native |
| --- | --- | --- |
| data change detection | sample `--watch` at breakpoints | `watch -l`, fires on write with the stack that did it |
| going backwards | re-run with a new breakpoint | `rr replay` |
| state vs. picture | one frame of lag by construction | same instant |
| bad writes | silently clobber a neighbouring global | ASan/UBSan trap |

The watchpoint case is the strongest. Working out what writes a descriptor
field is a stack trace natively, and re-deriving offset `0xA` from assembly
cost an afternoon.

ASan matters more than it looks: it catches exactly the overlapping-symbol
class (`D_800A1230[2]` against `D_800A1234`) that PSY-Z's porting guide warns
silently breaks ports.

### What Redux keeps

- **It is the oracle.** PSY-Z prioritises compatibility over accuracy, does
  not target 1:1 output, and does not reproduce hardware-misuse bugs. The
  spec here is byte-exact codegen, so a native build can never settle a
  matching question. `blend_probe.py` is the clearest case: blending is
  precisely what PSY-Z declines to guarantee.
- **It runs code that is not decompiled yet.** A native build needs the 262
  finished or stubbed. Redux runs the original assembly, so it stays the only
  way to probe a function still behind `INCLUDE_ASM`.
- **Which makes it partly circular.** You probe a function to understand it
  well enough to decompile it, and the native build needs it decompiled
  first. PSY-Z debugging improves *verification* of code already written, not
  *discovery* of behaviour not yet understood.

Keep `mako.sh redux` as the matching oracle. Reach for the native build when
the question is "this matches but behaves wrong", where a watchpoint beats
sampling over HTTP by a wide margin.

## Traps

- **Battle and world share the `0x800A` load address.** Name-based analysis
  conflates them: `func_800A6168` exists in `src/world/world.c` while battle
  has a *different* function at that same address. An earlier version of this
  scope wrongly pulled in 18 world functions for this reason. Resolve overlay
  symbols by overlay, never by name alone.
- **Ordered tables and primitive tags.** PSY-Z requires `u_long ot[]` ->
  `OT_TYPE` and a leading `u_long tag` -> `O_TAG`. `SysMenuSetOtag` in
  `src/main/269C0.c` is where this lands.
- **`u_long` is 64-bit on 64-bit builds.** Pointers must never be stored in
  `s32`/`u32`. Non-pointer 32-bit fields typed `u_long` must become
  `unsigned int`.
- **These changes break the sha1.** They alter codegen, so they cannot land in
  the matching build. A port has to be a downstream fork or sit behind
  `#ifdef __psyz`.
- **No hardcoded addresses.** PSY-Z crashes on `*(s32*)0x800A1234`. The C
  written so far is clean of these -- keep it that way.
- **PS1 file I/O is not POSIX.** Never include `<fcntl.h>`; use `FWRITE`
  and `FCREAT` from `<romio.h>`. `KERNEL.BIN`, `SCENE.BIN` and the battle
  models still have to load off the disc.

## Regenerating the list

The counts below drift as functions get decompiled. Regenerate with the
transitive walk over C callees and `jal`/`%hi`/`%lo` targets in the remaining
assembly, rooted at `BATINI_Main`, `BATTLE_RunFrame`, `func_80014934` and
`src/magic/*.c`, excluding `src/main/psxsdk.c`. A quick upper bound:

```shell
grep -rc INCLUDE_ASM src/battle src/main --include='*.c' | grep -v ':0'
```

## The 262


### `src/battle/battle1.c` — 75

    func_800B4E30                             func_800B54B8                             func_800B5AAC
    func_800B5C1C                             func_800B5E64                             func_800B5FE8
    func_800B60E0                             func_800B64CC                             func_800B677C
    func_800B6D6C                             func_800B7764                             func_800B79F0
    func_800B7DB4                             func_800B8944                             func_800B8A34
    func_800B8B48                             func_800B8EE4                             func_800B8FCC
    func_800B905C                             func_800B91CC                             func_800B9568
    func_800BA11C                             func_800BA2BC                             func_800BA360
    func_800BA598                             func_800BACEC                             func_800BB2A8
    func_800BB538                             func_800BB684                             func_800BBA84
    func_800BC754                             func_800BC8B0                             func_800BCA58
    func_800BCB1C                             func_800BE49C                             func_800BE69C
    func_800BE86C                             func_800BEA38                             func_800BFB88
    func_800BFDA0                             func_800BFF88                             func_800C0088
    func_800C0314                             func_800C0480                             func_800C0630
    func_800C0970                             func_800C0B20                             func_800C0DD8
    func_800C1104                             func_800C1394                             func_800C14C0
    func_800C1D8C                             func_800C2000                             func_800C2150
    func_800C223C                             func_800C2704                             func_800C2864
    func_800C2FD4                             func_800C36B4                             func_800C3950
    func_800C494C                             func_800C5170                             func_800C5694
    func_800C5864                             func_800C59B8                             func_800C5ADC
    func_800C5C18                             func_800C5CC0                             func_800C62F4
    func_800C6628                             func_800C70AC                             func_800C7220
    func_800C7340                             func_800C76C8                             func_800C7924

### `src/main/17238.c` — 75

    SysAddCommandToTemp                       SysAddMagicSummonSkillToUnitStructure     SysAddMagicToTemp
    SysAddMateria00                           SysAddMateria12                           SysAddMateria20
    SysAddMateria21                           SysAddMateria25                           SysAddMateria35
    SysAddMateria40                           SysAddMateria41                           SysAddMateriaCounterAttack
    SysAddMateriaEquipStatBonus               SysAddMateriaLongRange                    SysAddMateriaX1
    SysAddMateriaX2                           SysAddMateriaX3                           SysAddMateriaX4
    SysAddMateriaX5                           SysAddMateriaX6                           SysAddMateriaX7
    SysAddMateriaX8                           SysAddMateriaX9                           SysAddMateriaXa
    SysAddMateriaXb                           SysAddMateriaXc                           SysAddPairCommandWithCounter
    SysAddPairFlagToActiveCommand             SysAddPairFlagToAllActiveCommands         SysAddPairFlagToAllMagics
    SysAddPairFlagToAllSummons                SysAddPairFlagToCommandMagicSummon        SysAddPairFlagToMagic
    SysAddPairFlagToSummon                    SysAddPairMagicWithAll                    SysAddPairMagicWithMagicCounter
    SysAddPairMagicWithMpTurbo                SysAddPairMagicWithQuadraMagic            SysAddPairMasterMagicWithAll
    SysAddPairMasterMagicWithMpTurbo          SysAddPairMasterMagicWithQuadraMagic      SysAddPairMasterMateriaWithCounter
    SysAddPairMasterSummonWithMpTurbo         SysAddPairMasterSummonWithQuadraMagic     SysAddPairMateriaOrdered
    SysAddPairMateriaUnordered                SysAddPairMateriaWithSlotCheck            SysAddPairSummonWithMagicCounter
    SysAddPairSummonWithMpTurbo               SysAddPairSummonWithQuadraMagic           SysAddPairWithAddedEffect
    SysAddPairWithAll                         SysAddPairWithCounter                     SysAddPairWithElemental
    SysAddPairWithMagicCounter                SysAddPairWithMpTurbo                     SysAddPairWithQuadraMagic
    SysAddPairWithSneakFinalAttack            SysCalculateTotalLureGilPreemptiveValue   SysCopyAndSortCommand
    SysCopyBoostedStatToUnitStructure         SysCopyCommandToUnitStructure             SysCopySummonToUnitStructure
    SysCopyTempMagicToUnitStructure           SysGetCommandOrder                        SysGetMateriaActivatedStars
    SysInitPlayerStatFromMateria              SysInitPlayerTempStat                     SysParseMateriaEquip
    SysParseMegaallMateria                    SysRemoveStealIfMug                       SysSearchExistedCommand
    SysSearchExistedMagic                     func_8001726C                             func_8001AE08

### `src/battle/battle2.c` — 52

    BattleEntityGetCenter                     BattleGetPartPosition                     func_800C7C4C
    func_800CD400                             func_800CD5E4                             func_800CD860
    func_800CEB48                             func_800D08B8                             func_800D09D0
    func_800D1530                             func_800D29D4                             func_800D32B4
    func_800D3354                             func_800D3418                             func_800D3474
    func_800D34C8                             func_800D3548                             func_800D3658
    func_800D376C                             func_800D3AF0                             func_800D3F0C
    func_800D415C                             func_800D4284                             func_800D4368
    func_800D4710                             func_800D491C                             func_800D4A64
    func_800D4C08                             func_800D4D90                             func_800D4FF0
    func_800D5B6C                             func_800D5D28                             func_800D61AC
    func_800D6260                             func_800D650C                             func_800D6734
    func_800D6840                             func_800D6998                             func_800D6ACC
    func_800D6C20                             func_800D6D8C                             func_800D6F78
    func_800D70C0                             func_800D7178                             func_800D7368
    func_800D751C                             func_800D7724                             func_800D7888
    func_800D7A88                             func_800D7B1C                             func_800D7BA4
    func_800D85B0

### `src/battle/battle.c` — 23

    BattleInitCharCmdState                    BattleRecalcUnitSpeed                     BattleRunUnitScript
    BattleUpdateUnitMasks                     func_800A3ED0                             func_800A4E80
    func_800A5AC8                             func_800A5BC8                             func_800A5E0C
    func_800A72C8                             func_800AE42C                             func_800AF874
    func_800B0F04                             func_800B141C                             func_800B153C
    func_800B16D0                             func_800B18A8                             func_800B1AA0
    func_800B1D48                             func_800B2A2C                             func_800B2B5C
    func_800B2CFC                             func_800B3030

### `src/main/1F6B4.c` — 11

    SysAddAttackType                          SysAddElementalDefense                    SysAddStats
    SysAddStatusAttackBit                     SysAddStatusProtect                       SysAddStatusProtectBit
    SysGetPlayerBaseAttackDefense             SysInitPlayerStatFromEquip                func_80022DE4
    func_800254E4                             func_80025514

### `src/main/14C70.c` — 8

    SysDecompKernStringWithF9                 SysGetLimitCmdId                          SysKernGetString
    func_80014E0C                             func_80014E74                             func_800155A4
    func_800155B0                             func_80015668

### `src/main/akao.c` — 5

    SystemAkaoExecute                         func_8002FDA0                             func_8002FF4C
    func_80030038                             func_80030148

### `src/battle/battle3.c` — 4

    func_800DBF8C                             func_800DC0CC                             func_800DCFD4
    func_800E16B8

### `src/battle/batres.c` — 3

    func_801B0000                             func_801B0EF8                             func_801B17CC

### `src/battle/batini.c` — 2

    BattleInitEnemyUnits                      BattleInitPlayer

### `src/main/1C0EC.c` — 2

    SysSortMagicInUnitStructure               func_8001C3CC

### `src/main/33B70.c` — 1

    SysCdromSetChainParam

### `src/main/btlinit.c` — 1

    func_800148A0

<!-- total 262 -->

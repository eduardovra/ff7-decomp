# Porting to PC with PSY-Z

[PSY-Z](https://github.com/Xeeynamo/psyz) is a drop-in replacement for the
PSY-Q runtime that lets PS1 code compile and run natively on Linux, macOS,
Windows, iOS, WebAssembly and PSP. It is by the same author as this repo, so
it is the natural PC target for this decomp.

It is a link-time API replacement, **not** an emulator or a recompiler. It
cannot execute MIPS. Every function still behind `INCLUDE_ASM` on a code path
you want to run has to become C first.

This document's target is **boot straight into a battle and cast a spell**,
skipping the title screen, field and world map. An earlier version called that
"the smallest useful target". It is not the smallest -- it is the largest, and
the next section re-sequences the plan around that. Everything below it still
stands; what changed is the order the work happens in.

## The three phases, in short

| phase | work | difficulty |
| --- | --- | --- |
| 0 -- import `.data`/`.bss` | replace 928 guessed 256-byte zero stubs with true types, sizes and contents | large and tedious, but mechanical; shared by every overlay, so none of it is wasted |
| 1 -- boot a cheap overlay | decompile `dschange` (2 functions) or finish `ending` (11 left), wire disc loading, get a screen up | easy; proves out the link, the stub generator and gdb |
| 2 -- battle | the 262 functions, plus loading the true `KERNEL.BIN`, a `SCENE.BIN`, models and textures | hardest by a wide margin, and last |

## Battle boots last -- the revised sequence

Xeeynamo, who wrote both PSY-Z and this repo, was shown this plan and does not
think battle is the right first target:

> I think there's far too much missing yet for that. BATTLE is the most
> complex overlay. Essentially all .data and .bss must be imported first.
> Then let the game load the true KERNEL.BIN, a SCENE.BIN and all models +
> textures. I believe the Battle overlay will be the last one to boot.
>
> In terms of complexity, I think we have ENDING > DSCHANGE > SAVEMENU
> (includes Title screen) > MENUs > MINI > WORLD > FIELD > BATTLE.

Read that ordering easiest-first -- ENDING is the cheapest overlay to bring up
and BATTLE the dearest -- which is what "the last one to boot" pins down; the
bare `>` list on its own is ambiguous about direction.

The repo's own numbers agree. Remaining `INCLUDE_ASM` stubs per `src/`
directory, a rough proxy for how far each overlay is from running:

| directory | stubs left | |
| --- | --- | --- |
| `magic` | 0 | all seven spell overlays complete |
| `dschange` | 2 | the entire overlay is two functions, both still asm |
| `menu` | 5 | across five files |
| `brom` | 5 | |
| `ending` | 11 | of 51 functions; 40 are already C |
| `mini` | 49 | no C file yet |
| `world` | 119 | |
| `field` | 227 | |
| `battle` | 338 | |
| `main` | 787 | 486 of them PSY-Q, see below |

### The evidence that data, not functions, is the blocker

His "essentially all .data and .bss must be imported first" is already
observable in the prototype. `SysCdromSetChainParam` was the named blocker in
**How far it gets**; it has since been decompiled (`src/main/33B70.c`). The
`-battle` run dies in exactly the same place, because what it actually reads is
`D_8004A634`, the CD operation dispatch table, whose real contents live in
`asm/us/main/data/psxsdk.data.s` and which `src/pc/stubs.c` supplies as 256
bytes of zeroes. Decompiling the function did not move the blocker one inch.

That is the whole shape of the problem, and the stub counts say it plainly:
**928 stubbed globals against 320 stubbed functions**, every global's size a
guess from the gap to the next symbol.

### What this changes

**Phase 0 -- data before functions.** Importing `.data` and `.bss` for real,
with true sizes and types, is the prerequisite workstream, not cleanup to do
once the functions land. `src/pc/globals.c` is where it goes; today it holds
one hand-written definition (`Savemap`) and `src/pc/stubs.c` holds the other
927 as guesses. This work is shared by every overlay, so none of it is wasted
whichever one boots first.

**Phase 1 -- first native boot is a cheap overlay, not battle.** `dschange`
(two functions) or `ending` (11 of 51 left) reaches a running screen far
sooner, and proves out the parts that are genuinely common: the PSY-Z link,
the stub generator, disc loading, the hardcoded-address mapping, gdb. Neither
needs `KERNEL.BIN` tables, `SCENE.BIN` formations, models or textures.

**Phase 2 -- battle.** Still the interesting target, because it is the only
one that runs a spell and the only one with all seven magic overlays already
at 100%. It is the end of the queue rather than a shortcut to the front of it.

### What does not change

The technical content below is unaffected and most of it is not
battle-specific: what PSY-Z supplies and must not be decompiled, the
`u_long`/`OT_TYPE` traps, the hardcoded-address mapping, the overlapping-symbol
collisions, disc loading, and the whole **Versus the emulator loop**
comparison. **Why field is not needed** and **What sets up the game state**
remain correct about battle -- `BATINI_Main` really does take one integer.
The 262 stays as battle's function inventory; it is simply no longer the
number that gates the first native boot.

His estimate is one person's judgement, not a measurement. It is load-bearing
because he is the only person holding both toolchains, and because the
`D_8004A634` case above independently confirms its central claim.

## Precedent: sotn-decomp

sotn-decomp already ships this, and not as a side experiment:

- `tools/psyz` is a git submodule.
- `src/pc/` holds a PC backend of roughly 78 KB across 20 files -- a
  simulation layer, a replay harness, I/O, plus per-stage shims under
  `stages/`, `servants/` and `bosses/`.
- Its main `include/game.h` includes `<psyz.h>` directly, so the port lives
  in-tree rather than in a fork.
- `CMakeLists.txt` sets the `__psyz` compile definition and calls
  `psyz_title()`.
- A dedicated workflow, `picci.yaml` ("Build cross-platform codebase"),
  builds Linux x86_64 and i686, macOS and Windows on every pull request
  touching `src/`, `include/` or the cmake files.

PSY-Z is therefore a CI-gated build target on a large, real game, not only the
SDK samples its README advertises.

Two lessons carry over: the in-tree route works (see **Traps**), and globals
cost more than the function count suggests (see **Scope**).

One caveat. sotn-decomp is itself a work in progress and its PC builds still
pass CI, so a port plainly does not demand 100%. How it handles functions that
are still assembly is unverified here -- do not read it as proof that FF7 can
build at roughly half.

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
    func_800148B4();          /* boot init: LBA table + all of KERNEL.BIN */
    func_80014934();          /* reseed Savemap.party from the init blob */
    BATINI_Main(sceneID);     /* any formation from SCENE.BIN */
    for (;;) {
        BATTLE_RunFrame();
    }
}
```

There is no `psyz_init()`. PSY-Z initialises itself from the first
`ResetGraph`/`PutDispEnv`, exactly as PSY-Q did, so the entry point is a plain
`main`.

`func_800148B4` is easy to miss and nothing works without it.
`func_80014934` on its own decompresses **only** the init section, whereas
`func_800148B4` is the real boot path: it clears the battle flags, builds the
LBA table (`func_80014610`), and loads `INIT_KERNEL` with `func_80014750` as
its callback, which walks every section of `KERNEL.BIN` into the fixed tables
in `D_80048DD4` (`src/main/144D8.c`) -- command, attack, growth, item, weapon,
armor, accessory and materia. `BATINI_Main` reads `g_WeaponTable` through
`SysInitPlayerStatFromEquip` and `g_MateriaData` through `BattleGetMateriaValue`,
so skipping it leaves every character with zeroed gear rather than no gear.

`func_800148B4` is itself still `INCLUDE_ASM`, and it is **not** in the 262.

`g_BattleMode` needs no assignment: `func_800148A0`, the first thing
`func_800148B4` and `func_80014934` both call, already zeroes it along with
`D_80062F88`. It is the one piece of field state on this path
(`src/main/110B8.c`). You need field's *struct definitions* from `game.h`,
never field's *code*.

## What sets up the game state

Nothing picks anything. The party comes from a blob, and one integer chooses
both the enemies and the arena.

### Party, materia and equipment

All of it is `Savemap`, and the seed is one compressed `KERNEL.BIN` section:

```c
SysGzipPackDecompressById((u8*)0x801B0000, &Savemap.party, KERNEL_INIT);
```

`&Savemap.party` is `0x8009C738`, the same address `D_80048DD4[3]` names. That
section *is* the new-game party: for each of the nine `SavePartyMember` records
(`include/game.h`) it writes `char_id`, `level`, the six stats, `name`, the
`weapon`/`armor`/`accessory` ids and `materia_weapon[8]`/`materia_armor[8]`. You
get what "New Game" gives you, with no field code involved.

Which three fight is `Savemap.partyID[3]` at `0x8009CBDC`; `BattleInitPlayer`
walks it and `0xFF` means an empty slot. `BATINI_Main` then runs
`SysInitPlayerStatFromEquip` and `SysInitPlayerStatFromMateria` per slot against
those same bytes plus the kernel tables.

To use a different party, write `Savemap` after seeding it:

```c
Savemap.partyID[0] = CLOUD;
Savemap.party[0].weapon = weaponID;
Savemap.party[0].materia_weapon[0] = (ap << 8) | materiaID;  /* 0xFF = empty */
```

`BattleGetMateriaValue` in `src/battle/batini.c` is where that packing is
visible: low byte is the materia id, the upper 24 bits are AP.

### Enemies and arena

`BattleInitLoadSceneData` turns `sceneID` into everything else:

- `sceneID / 4` selects a scene chunk; `BattleGetScenePackId` maps it through
  `D_80083184` to a four-sector block of `SCENE.BIN`, loaded to `0x801C0000`
  and `Unzip`ped into one `SceneContainer`.
- `sceneID % 4` picks one of the four formations in that block.
- Out of it come `enemyModelIDs`, `setup` (**`stageID` is the battle
  background**, plus `cameraID`, `type`, flags and the escape counter), the
  camera placements, the `formation` table of per-enemy positions, the enemy
  stat records, their attacks and names, and the AI script.

So field's only job was to roll a `sceneID` from the map's encounter table.
Supplying it directly is the whole shortcut.

Two battle-overlay globals normally arrive from field and must be zero for a
plain random encounter: `D_8016376A`, the battle flags -- bit `0x10` is Battle
Square, which rewrites `stageID` to 37, forces `SETUP_CANNOT_ESCAPE` and boosts
enemy strength and magic by 25% -- and `g_BattleMultiInfo.isMultiBattle`. Both
are BSS, so zero is what you already have.

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
decompiled `src/magic/*.c` calls, **9 are PSY-Q** and vanish for free:

    ApplyMatrix     CompMatrix      RotMatrixYXZ    ScaleMatrix    SetFarColor
    SetRotMatrix    SetTransMatrix  rcos            rsin

`rand` is not among them -- PSY-Z does not define it. Let libc's `rand` resolve
it, and make sure the stub generator does not shadow it. The same applies to
`printf`, `sprintf`, `memcpy`, `memset`, `strcmp` and `atoi`, which FF7 also
carries as PSY-Q assembly.

PSY-Z does **not** cover PS1 file I/O either. `open`, `read`, `write` and
`close` share their names with POSIX but not their semantics, and nothing in
PSY-Z provides them. Loading `KERNEL.BIN` and `SCENE.BIN` is work you own.

## Scope

The number that gates a battle boot is **globals**, not functions -- see
**Battle boots last** above. PS1 globals live at fixed linker-script addresses,
so natively each one needs a real C definition, and `.data` has to carry its
real contents rather than zeroes. sotn-decomp carries `src/pc/stubs.c` for the
same reason, and it is where overlapping symbols surface: one declaration in
that file is annotated as an overlap its authors found hard to remove.

The function inventory is **262**, reached transitively from `BATINI_Main`,
`BATTLE_RunFrame`, `func_80014934` and `src/magic/*.c`, with PSY-Q excluded.

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

## The prototype

It exists and it runs. `tools/psyz` is a submodule, and the repo's existing
modern-compiler target -- which already builds every decompiled `.c` with
`SKIP_ASM=1` into `libff7.a` -- links against it behind an opt-in CMake flag.

```shell
git submodule update --init --depth 1 tools/psyz
git -C tools/psyz submodule update --init --depth 1 external/SDL

cmake -B build-pc -DFF7_PC=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-pc --target ff7 psyz -j8      # pass 1: the libraries
tools/gen_pc_stubs.py --psyz build-pc/psyz/libpsyz.a \
    build-pc/libff7.a \
    build-pc/CMakeFiles/ff7_pc.dir/src/pc/main.c.o
cmake --build build-pc -j8                        # pass 2: the executable

./build-pc/ff7_pc -headless                       # no window, CI-friendly
./build-pc/ff7_pc -frames 600                     # the PSY-Z render loop
./build-pc/ff7_pc -battle -scene 100              # the boot path, still dies
```

Two passes because the stub set comes from the link itself: build the
libraries, ask the linker what is missing, generate a placeholder for each,
link. `src/pc/stubs.c` is generated and committed; regenerate it whenever a
function lands.

### What it measures

The numbers the linker gives are not the same as the 262, and are worth having
(the stub counts drift as functions land; `src/pc/stubs.c` states its own):

| | |
| --- | --- |
| symbols `libff7.a` leaves undefined | 1592 |
| of those, supplied by PSY-Z | 81 |
| stubbed functions | 320 |
| stubbed globals | 928 |

The globals dominate, as the **Scope** section warns. They are also the
sloppiest part of the prototype: `tools/gen_pc_stubs.py` sizes each one from
the gap to the next address in `config/*.txt` and falls back to 256 bytes, so
every one of them is a guess. `Savemap` is hand-defined in `src/pc/globals.c`
from its real `SaveWork` type precisely because a guess there would be too
small and corrupt the heap. That file is where the others should migrate.

Only **three** symbols actually collide in a single native link.
`D_800A0000` is defined by battle, field and world -- a `u8`, a `u32[]` and
the string `"NEW  "`, three unrelated things sharing a name because the name
is the overlay load address. `func_800ADFC0` and `func_800AF1A8` are battle
against world, genuinely different functions at the same address.

None of that needs a dynamic loader. The PC build renames them per overlay
with `COMPILE_DEFINITIONS` in `CMakeLists.txt`, battle keeping the bare name,
and links everything including world.

**Do not rename them in the sources.** That was tried and it breaks the
matching build: `asm/us/world/data/world.data.s` references `D_800A0000`, and
the world overlay's own still-assembly code calls both functions, so the
rename would have to propagate into checked-in assembly and the symbol config.
The collision only exists in the native link, so the fix belongs there.

### Traps the prototype walked into

Both are the silent kind, and both are now handled in the generator:

- **Stubbing `printf`.** FF7 carries it as PSY-Q assembly, so it lands in the
  undefined set, and a stub for it swallows every diagnostic the program
  prints. `sprintf`, `memcpy`, `memset`, `strcmp` and `atoi` are the same.
- **Stubbing something the archive already defines.** An archive member can
  reference what another member defines, so `nm -u` reports it as undefined.
  A stub then wins the link over the real body, because an object beats an
  archive member. Symptom: fully decompiled functions returning 0.

### How far it gets

`-headless` runs `SysCountActiveBits`, `SysGetLsbNumber` and `SysMemCopy32`
out of `src/main/btlinit.c` and prints correct answers, so decompiled game
code really is executing. `-frames` drives the PSY-Z render loop.

`-battle` maps PS1 RAM and then dies in `SystemCdromReadChain`, on
`D_8004A634[*op]()` -- the CD operation dispatch table, still data in
assembly, so a stub of zeroes and a call to NULL. That is the expected shape:
gdb names what to do next.

Disc loading is the whole blocker, and it is smaller than it looks. FF7 never
does file I/O: `SystemLoadFileBySector` issues an asynchronous sector read
through `SysCdromSetChainParam`, pumped by `while (SystemCdromReadChain())`,
over `CdControl(CdlSetloc, ...)`. PSY-Z already implements `CdControl`,
`CdRead`, `CdReadSync` and `CdSync`, parses `.cue`/`.bin`, and takes an image
path from `Psyz_CdSetDiskPath`. `SystemCdromReadChain` and
`SysCdromSetChainParam` are both C now (`src/main/33B70.c`), and the crash did
not move: what is missing is the handler table `D_8004A634` itself, still data
in `asm/us/main/data/psxsdk.data.s`. See **Battle boots last** -- this is the
data-before-functions problem in its smallest form.

## Running it under gdb

PSY-Z produces an ordinary native ELF, so gdb works normally -- real
breakpoints and watchpoints on game functions, which PCSX-Redux cannot give
you. Note the PC build is a *separate* build from the matching one: modern
clang/gcc with `-g -O0`, not gcc 2.6.3.

```
(gdb) break BATINI_Main
(gdb) run -battle -scene 100
(gdb) watch -l Savemap.party          # catch whoever corrupts the party
(gdb) break BATTLE_RunFrame           # then `finish` to step frame by frame
```

`rr record ./build-pc/ff7_pc` then `rr replay` gives reverse execution, which
is the fast way to find what wrote a bad value in `g_BattleState`.

**You do not have to finish all 262 before debugging**, and the prototype above
already proves it: stubs for the whole set, linked immediately, replaced one at
a time. The build runs (badly) today and gdb is available throughout -- far
better feedback than waiting for function 262.

On a machine whose session is Wayland, SDL may need `-u WAYLAND_DISPLAY` or an
`xvfb-run` wrapper; `-headless` sidesteps graphics entirely.

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
  `#ifdef __psyz`. sotn-decomp takes the second route and keeps everything
  in-tree: `<psyz.h>` from its main `include/game.h`, PC-only code under
  `src/pc/`.
- **Hardcoded addresses are already there.** PSY-Z crashes on
  `*(s32*)0x800A1234`, and the decompiled C contains 88 such casts across 17
  files. The battle bootstrap hits one immediately: `func_80014934` decompresses
  into `(void*)0x801B0000` and `BattleInitLoadSceneData` reads `0x801C0000`.
  Mapping the PS1's 2MB of RAM at `0x80000000` makes them all valid and costs
  nothing in the matching build, which is what the prototype does. Rewriting
  them into real buffers is the clean fix, and it changes codegen.
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

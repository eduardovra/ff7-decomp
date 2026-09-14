# PC stub dependencies

Upstream PR 167 ("Add PsyZ support") boots the title screen natively.
Xeeynamo will not merge it "until most of the stubbed dependencies are
decompiled", so the stub list in its `src/pc/stubs.c` is a ready-made,
externally motivated work queue: every entry we match shrinks the gate on
that PR.

That stub list -- not our own `src/pc/stubs.c` -- is the list to work from.
Ours is a different, much larger one because our CMake links the whole tree
rather than just the boot path; the extra entries are ordinary decomp
backlog with no one waiting on them.

## Reading the queue

Generate it, never hand-maintain it:

```shell
.venv/bin/python3 tools/pc_stub_status.py             # whole queue
.venv/bin/python3 tools/pc_stub_status.py --state todo
```

The script fetches the PR diff with `gh`, extracts every symbol upstream
stubs as a *function*, and classifies each one against this tree by
**address**. Five states:

- `todo` -- still behind an `INCLUDE_ASM` here. This is the work queue, and
  the line carries our local name, the instruction count and the asm
  directory.
- `psyz` -- our name for the address is one PsyZ's headers declare, so the
  PC build links PsyZ's implementation. Nothing to decompile; the job was
  naming the symbol, and it is finished.
- `done` -- splat emitted asm for the address but nothing includes it, so
  the build compiles C for it.
- `hasm` -- hand-written asm in `src/**/*.s`. Present in the ROM build but
  still undefined for PC, and unstubbing it means porting asm to C rather
  than matching a function.
- `unsplit` -- no asm at the address at all, or no address for the name in
  any `config/` symbol file. Not startable; needs splat work first.

Pass `--diff <file>` to reuse a saved `gh pr diff` instead of refetching,
and `--pr`/`--repo` to point at a different PR.

## Why it compares addresses, not names

The obvious implementation -- list the names upstream stubs, list the names
we define, `comm` the two -- is wrong in four ways, and the first is the
one that matters:

1. **Upstream's stubs keep the label the symbol had when it was stubbed.**
   Every function we have since *named* while decompiling looks missing
   forever: upstream's `func_801D2D74` is our `StealAllMateria`. Five of
   the materia functions in `itemmenu.c` were reported as outstanding work
   for this reason.
2. **A grep for definitions in `*.c` misses `hasm`.**
   `SysCdromGetPackPointer` and `func_80034D5C` live in `src/main/lzss.s`.
3. **Absence of a C definition is not one state.** "Behind `INCLUDE_ASM`",
   "hand-written asm" and "never split" need different work, and only the
   first is a decomp task.
4. **A name appearing in `src/` is not a definition.** Matching
   declarations and call sites marks callers as done -- that is what put
   the `ending.c` and `field.c` entries on the wrong side of the list.

Two traps inside the script are worth knowing if it is ever rewritten:

- A function's asm file can open with a `.rodata` jump table, whose
  `.word` entries look exactly like instructions and carry unrelated
  addresses. Measure only between the function's own `glabel` and its
  `.size`, or `SysMenuDrawBattleResult` reads as living at `8001032C`.
- `asm/us/main/nonmatchings/psxsdk/` holds 28 stale files left from when
  `0x80033BE0`--`0x80034E00` moved out to `33B70` and `lzss.s`: 21 are
  byte-identical duplicates of live `33B70` files and 5 are now hand-written
  asm. Nothing includes any of them, so a `find`-based survey of the asm
  tree reports the wrong directory. Keying off the `INCLUDE_ASM` the build
  actually compiles avoids them.

## Where the low-hanging fruit is

Regenerate rather than trusting these; at the time of writing, of 77
function stubs: 56 `todo`, 2 `psyz`, 13 `done`, 2 `hasm`, 4 `unsplit`.

**Read the next section first.** Nine `todo` entries are SDK functions that
must not be decompiled here at all, and the cheapest-looking ones are among
them. What is left, game code only:

| insns | stub | asm |
|---|---|---|
| 12 | `func_80029998` | `akao` |
| 14 | `func_80034444` | `33B70` |
| 18 | `func_800211C4` | `1F6B4` |
| 21 | `func_80036244` | `psxsdk` |
| 23 | `func_800A1EEC` `func_800A1F48` | `ending` |
| 26 | `SysMenuSetMenuListAnimation` `func_8001117C` | `1F6B4` `110B8` |
| 27 | `SysMovieLoadMovieSettings` | `psxsdk` |
| 29 | `func_80029818` | `akao` |

Already off the list: `SysMenuGetMenuListState` (3), `func_80034410` (4) and
`func_80034F3C` (8) are decompiled, and `func_80036298` turned out to be
`SpuInit` -- see below.

## The SDK entries are not work

`src/main/psxsdk.c` is 980 lines and, at the time of writing, contains not
one C definition -- it is entirely `INCLUDE_ASM`, on purpose. Line 38
carries a note asking that these functions not be decompiled here, pointing
at `psyz/decomp` instead, and the note has no closing marker: its region is
line 38 to the end of the file. PsyZ reimplements this code, so matching it
here is work upstream will not take.

**The note's line is not the boundary.** It is tempting to read "above line
38" as "game code", and that is wrong: `func_80036298` sat above it and was
`SpuInit`, a real libspu function -- psyz has exactly
`void SpuInit(void) { _SpuInit(0); }`, and `_SpuInit` is a library internal
that only the library calls. It is now named `SpuInit` here and needs no C.

Two reliable tests instead, both cheap:

- **Does it call a library internal?** An underscore-prefixed callee
  (`_SpuInit`) means the caller is library code, because nothing outside the
  library calls those.
- **Does psyz already have its body?** Grep `psyz/decomp/src/<lib>/`. A hit
  means SDK. `func_80034F3C` wraps `DecDCTReset(0)` and looks like the same
  case, but psyz has no such wrapper and its only caller is FF7's own
  `SysCdromInit`, so it is game code -- and it is decompiled here.

Everything below line 38 is library:

| below the note | what it is |
|---|---|
| `func_8003DDA4` `func_8003DE6C` `func_8003DE84` | libcd callback setters |
| `func_80041D28` `func_80041E30` | libcd |
| `func_800484A8` `func_80048540` | between `unpack_packet` and `_card_*` |
| `MulMatrix2` | libgte |
| `delete` | BIOS B-table syscall `0x45`, with `firstfile`/`nextfile` |

They are in upstream's stub list because they are still labelled
`func_8003*` here, so nothing connects them to what PsyZ provides. That
makes them an identification job, not a decomp one: name the label and the
`psyz` state picks it up. `SpuInit` is the worked example -- one
`./mako.sh symbols add` entry, the `INCLUDE_ASM` and two call sites
renamed, and the stub is gone with no C written.

`func_8003DE2C` is done the same way -- it is `CdSync`, a passthrough to
`CD_sync` with a `nop` delay slot, called as `switch (CdSync(1, NULL))`
against `CdlComplete`/`CdlDiskError`.

The three setters are the next candidates, and the evidence is close to
hand: `psyz/decomp/src/libcd/sys.c` has
`CdSyncCallback(func) { prev = CD_cbsync; CD_cbsync = func; return prev; }`,
exactly their shape, and this tree already names `def_cbsync`,
`def_cbready` and `def_cbread` at `0x8003DC60`--`0x8003DCB0`. Those are the
values `CD_cbsync`/`CD_cbready`/`CD_cbread` are initialised to, so whichever
default each of `D_80051634`, `D_80051628` and `D_8005162C` receives at init
tells you which callback that global is -- and therefore which setter is
which. Establish that before naming any of them.

`MulMatrix2` is the exception: PsyZ does not declare it, so naming alone
will not resolve that one.

The densest cluster in the queue is menu drawing (`SysMenuDraw*` in
`26B70`, `1F6B4` and `1CDA4`), where this fork already has momentum from
upstream PR 166. It is also where the largest functions are:
`SysMenuDrawMainMenu` at 972 instructions and `SysMenuDrawWindow` at 699.
`SysMenuDrawAddWindow` depends on `SysMenuDrawWindow` landing first.

Counting the SDK block out, the real queue is 47 entries, not 56.

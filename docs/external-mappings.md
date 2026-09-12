# External mappings from other projects

Community reverse-engineering resources with names, addresses, and
struct layouts that can be ported into this project's symbol files and
headers. FF7's modding community documented most major structures long
ago; this is external information no tool can derive from the binary.

## Sources

### Akari's q-gears_reverse (richest PSX-specific source)

<https://github.com/Akari1982/q-gears_reverse> -- `ffvii/` directory.

Per-module RAM maps, ~400-500 documented locations each, with variable
names, struct sizes, and field offsets (not formal C structs -- offset
lists with descriptions):

| file | covers | notes |
|---|---|---|
| `address_battle.txt` | battle overlay | unit structs, action queues |
| `address_field.txt` | field overlay | |
| `address_wm.txt` | world map overlay | |
| `address_system.txt` | kernel / main | source of PR #135's labels |

Also per-module RAM snapshots (`ffvii_*_ram.bin`) useful for verifying
offsets, and `documents/`.

Direct overlap examples with this repo:

- Its "queue at `80163798`, 0x40 items, 0xC size" is our unnamed
  `D_80163798 = 0x80163798; // size:0x300` in `config/symbols.battle.txt`.
- Its battle unit struct (`800f5bb8`, size 0x44, `+00` speed,
  `+3c` current hp) covers a family of `D_800F5*` unknowns.

PR #135 (<https://github.com/xeeynamo/ff7-decomp/pull/135>) already
ports the kernel *function labels* from here. The *data structures* in
the address files are the unclaimed half.

### Final Fantasy Inside / Qhimm wiki

The `qhimm-modding.fandom.com` mirror now returns HTTP 402 to scripted
fetches; use the `wiki.ffrtt.ru` URLs below, which serve the same
pages.

- Savemap, field-by-field: <https://wiki.ffrtt.ru/index.php/FF7/Savemap>
  (mirror: <https://qhimm-modding.fandom.com/wiki/FF7/Savemap>) --
  directly relevant to savemenu.
- Kernel memory management:
  <https://qhimm-modding.fandom.com/wiki/FF7/Kernel/Memory_management>
- Battle scenes / kernel.bin formats:
  <https://qhimm-modding.fandom.com/wiki/FF7/Battle/Battle_Scenes>
- PSX sound code map (akao struct offsets):
  <https://wiki.ffrtt.ru/index.php?title=FF7%2FPSX%2FSound%2FCode_Map>

### The "Gears" document

<https://q-gears.sourceforge.net/gears.pdf> -- classic engine
architecture writeup (module structure, memory layout, formats). Best
for orienting naming decisions rather than mechanical import.

### Qhimm forums (scattered, deep)

- Useful memory addresses: <https://forums.qhimm.com/index.php?topic=17081.0>
- FF7 1.02 addresses (PC, structures largely mirror PSX):
  <https://forums.qhimm.com/index.php?topic=12914.0>

### q-gears engine code and reversing data

<https://github.com/q-gears/q-gears> -- the engine itself; file-format
structs (kernel.bin, scene.bin, field formats) in C++.

<https://github.com/q-gears/q-gears-reversing-data> -- richer than
"file formats" suggests. `reversing/ffvii/ffvii_battle/effect/magic/`
documents the battle *effect* functions the magic overlays call:

| ours | q-gears note (`functions.txt`) |
|---|---|
| `func_800D4D90` | "create number of quads according to data in effect" -- walks effect frame data, emits textured quads, `gte_RTPS`, colour blending |
| `func_800D4368` | "applies scale matrix, rotation, and translation transformations" |
| `func_800D5444` | "initializes impact effect data structure with sound and damage information" |
| `func_800D574C` | "calculates middle point from target mask unit positions for sound positioning" |
| `func_800D56A8` | "sets rotation/translation matrices via GTE, performs perspective transform" |
| `func_800D55F4` | "executes AKAO sound system with effect parameters" |

`fire_desc.txt` sketches an overlay's skeleton -- the same shape as
`thunder.c`:

> load texture from 0x801b039c. add 0x801b00b8 callback for all units
> in target mask. add 0x801b0294 callback. calculate sound params and
> play sound.
>
> 0x801b0000 effect: work 0xe frames. each frame read data from effect
> billboard data and create number of quads with texture.

That is `func_800D2980` -> `MagicAnimationRegister` ->
`BattleEffectRegister` -> `BattleCommandSend`, then a fixed-frame-count
render callback -- exactly `func_801B06CC` and `func_801B0020`.

Applied so far: the q-gears note on `func_800D4D90` named offset `0x8`,
corroborated by the countdown loop (`addiu -1` / `bltz`) in that
function's asm -- though the note's "number of quads according to data in
effect" describes the count held in the *data*, not in the field, which is
an index. It is now `u08.frameIndex`. The same note supports the
`CVECTOR color` arm of the union at offset 0x4.

The per-spell `.asm` files there (`thunder.asm`, `brizad.asm`, ...) are
raw disassembly with no annotations -- the value is in `functions.txt`
and the `*_desc.txt` files.

#### Two addressing schemes, don't mix them

q-gears is a cross-platform engine reimplementation, so the natural
worry is that its addresses are PC. They are not: this subtree is PSX
MIPS, and their `thunder.asm` is our thunder overlay instruction for
instruction (same `0x8015169C` / `0x80162978` references, and their
`L0068`/`L0090`/`L016c` are our `.L801B0068`/`.L801B0090`/`.L801B016C`).

The repo does use two different bases, though:

| where | base | convert |
|---|---|---|
| per-spell `.asm` dumps | overlay-relative, `0x80000000` | add `0x1B0000` |
| `functions.txt` names (`funcd4d90`) | absolute PSX | none |

`functions.txt` is absolute because those functions live in the
resident battle module rather than the overlay. Absolute references
*inside* a dump keep their real addresses regardless -- the entry point
at their `0x80000000` does `jal $801b06cc`, which is `func_801B06CC`.

### Zaarbs' PC-port decomp (C++ source, not addresses)

<https://github.com/Zaarbs/ff7> -- a partial decompilation of the 1998
PC port, GPL-3.0, work in progress (it builds a playable `FF7.exe`
against DirectX/dgvoodoo2). The PC port was ported *from* the PSX code,
so the module and function decomposition largely survives: `src/battle/`
(`b3ddata`, `battle3d/`, `yama/`), `src/field/` (`ad_*`, `fkawai`,
`frender`), `src/wm/`, `src/menu/`, `src/movie/`, `src/sound/`,
`src/polygon/`, plus kernel-ish pieces we also have -- `heap.cpp`,
`list.cpp`, `sort.cpp`, `stack.cpp`, `cd.cpp`, `playtimeCount.cpp`.

Value here is *structure*, not offsets: field names and struct shapes,
control flow of a routine we are staring at in asm, and above all
plausible **names** for things (the PC port kept many original
identifiers). Useful when a PSX function has no q-gears note but its PC
sibling is decompiled and readable.

Hard limits:

- **No addresses transfer.** This is x86 with a different build; nothing
  in it maps onto a PSX RAM address. Do not import numbers from it.
- **The renderer is not ours.** `src/graphics/` is the DirectX backend
  the port replaced the GPU/GTE path with -- irrelevant to our packet
  building, and actively misleading for the battle render path.
- **Structs may have been widened** for 32-bit x86 (pointers, padding,
  `int` vs `short`). A layout has to be re-checked against our asm.
- **GPL-3.0 and decompiled third-party code.** Read it for
  understanding; do not paste from it into `src/`.

## Porting workflow

A struct import touches two places (see docs/tooling-ideas.md,
resolver section, for why):

1. `./mako.sh symbols add <symbols file> <name> 0x<addr> <size>` --
   feeds splat (asm labels, data boundaries).
2. Typedef + `extern` declaration in a header -- feeds the compiler
   and m2c.

Score every import with the taxonomy harness
(`tools/taxonomy_poc.py --batch` before/after): the PR description can
then say "unlocks N functions / fixes M compile-fails" with data.
Suggested first import: `address_battle.txt` -- battle is the largest
unmatched overlay (376 functions) and the file has both names and
layouts.

Attribution: credit the source (Akari / q-gears / wiki page) in the
commit message when porting.

## Caveat

All of the above is community reverse engineering, not ground truth.
Treat a name or offset from it as a hypothesis to check against the
asm, exactly as with any other guess -- the value is that it is an
*independent* derivation, so agreement is real evidence.

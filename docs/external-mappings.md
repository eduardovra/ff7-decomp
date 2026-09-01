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

### q-gears engine code

<https://github.com/q-gears/q-gears> and
<https://github.com/q-gears/q-gears-reversing-data> -- file-format
structs (kernel.bin, scene.bin, field formats) in C++; more about disc
formats than RAM layout.

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

# Decomp workflow notes

Practical things that cost real time to rediscover. Complements the terse
Tooling section in the README.

## Build and verify

```shell
ninja build/us/src/magic/brizad.c.o     # compile one file, fast
make build                              # everything, ends in a per-overlay sha1
```

`<overlay>.exe: OK` in the build output *is* the match test. A per-function
diff score of 0 is not the same thing -- see "Data layout" below.

```shell
.venv/bin/python3 tools/asm-differ/diff.py -mows <function>
```

Score is in the header; lower is better, 0 means the instructions agree.

**`-m` hides compile failures.** When the build fails asm-differ silently
redraws the last object that *did* compile, so the diff looks frozen while
you edit. If the diff stops responding to your changes, run the `ninja` line
above in another terminal and check for errors there first.

## Symbols

Use the supported command rather than hand-editing the symbol files:

```shell
./mako.sh symbols add <path> <name> <offset> [size]
```

Naming a symbol is usually what turns unreadable output into obvious code.
m2c can only name what splat named. If an expression looks structurally
insane -- pointer arithmetic against a field address, a cast to a type that
makes no sense -- **suspect a missing symbol before you suspect exotic C.**

Worked example: brizad's render call decompiled as
`func_800D29D4((Unk801B0C98*)(&D_801B100E - 0xA), ...)`. splat had labelled
only `0x801B100E`, because that is the address the code references; the
16-byte struct starting at `0x801B1004` had no name and was swallowed into a
neighbouring data blob. Adding `BrizadRenderDesc = 0x801B1004;` let the
source be written the way it originally was, and it matched immediately.

## splat

**splat only writes `asm/.../nonmatchings/*.s` when the `.c` file does not
exist.** Adding names to a symbol file and re-splitting will not rename
existing assembly. To force it:

```shell
mv src/magic/brizad.c /tmp/keep.c
.venv/bin/splat split build/us/brizad.yaml
mv /tmp/keep.c src/magic/brizad.c
```

The `.s` filenames come from the symbol names, so `INCLUDE_ASM` entries must
be updated to match after a rename.

**splat drops a data label once nothing in the assembly references it.**
Moving a descriptor into C removes the only reference to the blob it points
at, so a symbol in the middle of a data range silently merges into its
neighbour and the link fails with `undefined reference`. Give it an entry of
its own first: `./mako.sh symbols add <symbols_path> <name> 0x<addr>`. A blob
at the start of its range keeps its label either way.

## Function order is address layout

The linker emits functions in source order, so a function's position in the
`.c` file determines its address. Addresses must ascend down the file. A
forward declaration does not help -- it emits no code, and the address comes
from where the *definition* sits.

## `static` goes last

Convert to `static` only after every function in the file matches.

- Static functions do not appear in the `.map`, and asm-differ locates
  functions through the `.map`. Converting early gives you
  `Not able to find .o file for function.`
- You cannot mark an `INCLUDE_ASM` function static; the assembly emits a
  global label.
- An unreferenced static *may* be dropped, which would shift every later
  address. gcc 2.6.3 kept brizad's unused twin, but verify with `nm` rather
  than assuming.

The payoff is real: lv5deth's block in the generated
`config/sym_ovl_export.us.txt` went from 16 lines to 10, and brizad exports
4 symbols instead of 9. Only the entry point needs to stay global.

## Codegen gotchas (gcc 2.6.3, `-O2 -G0`)

**`>>` and `/` are not interchangeable on signed values.** Division rounds
toward zero, an arithmetic shift rounds toward negative infinity, so `/ 4096`
emits a branch plus a `+4095` bias that `>> 12` does not. If the target has a
bare `sra` with no surrounding branch, the source used a shift. Use a named
constant (`>> FIXED_SHIFT`) to keep it readable.

**Parentheses do not constrain reassociation.** Integer multiply is
associative, so gcc flattens `A * (B * const)` into one tree and pairs the
constant with whichever factor it likes. If the target's shift/add
strength-reduction chain runs on a different variable than yours, the
original had an **intermediate variable**; a statement boundary is the only
thing that will move the chain.

```c
t = effect->Scale * 0x36D;                    /* forces the chain onto Scale */
scale = (s32)(effect->AnimationFrame * t) >> 12;
```

Once the tree is split, source order then controls `mult` operand order.

**Register allocation follows pseudo-creation order.** A diff whose only
fault is the same two registers swapped, repeated everywhere they appear,
means the instructions are already right. gcc hands out hard registers in
the order the pseudos are created, so a subexpression written inline can
claim a register ahead of the value it was computed from. Naming it puts
the creation order back:

```c
/* target: lw v0,0(a2); srl v1,v0,8  -- the loaded word is in $v0 */
ap = materia >> 8;                 /* materia's pseudo is created first */
if (ap == 0xFFFFFF && (materia & 0xFF) == materiaId)
```

Written as `if ((materia >> 8) == 0xFFFFFF && ...)` the shift takes $v0 and
pushes the load to $v1. Reach for the named temp before
`register u32 materia asm("$2")` -- a pin makes the score 0 without
explaining anything, and it stays in the committed source forever. Applies
to 2.7.2 as well as 2.6.3.

**m2c invents a name per assignment.** It works close to SSA form, so every
write becomes a fresh `temp_*`/`var_*` and it cannot tell one variable
reassigned from two variables. When a value the target keeps in a single
register is spread across two in your build, suspect that split first: the
tell is a two-address update in the target, `addu s1,s1,v0`, against a
three-address one in yours, `addu s0,a2,s0`. Collapsing them back into one
local restores the allocation, and usually fixes a register swap further
down as a side effect, since the extra long-lived pseudo was shifting
everything after it.

**Where you assign decides how long a value lives.** An initialiser at the
top of a function computes the value there. If it is not consumed until after
several calls, gcc must park it in a callee-saved register, adding a
save/restore and growing the stack frame. Assign at the point of use instead.

**A global's address held in a pointer is not the same program as naming
the global.** The address costs two instructions either way, but gcc spends
them differently: name the global and it folds `%lo` into each memory
operand, rebuilding the address at every access; take its address into a
local and it materialises the address once and reuses the register.

```
D_800EE42C--;                    temp = &D_800EE42C; *temp -= 1;
--------------------------       ------------------------------
lui   $v0, %hi(sym)              lui   $v0, %hi(sym)
lhu   $v0, %lo(sym)($v0)         addiu $v0, $v0, %lo(sym)
addiu $v0, $v0, -1               lhu   $v1, 0($v0)
lui   $at, %hi(sym)              addiu $v1, $v1, -1
sh    $v0, %lo(sym)($at)         sh    $v1, 0($v0)
```

A read-modify-write on a named global therefore pays for the address twice.
The pointer local is the only way to ask for it once, so one in the target
is evidence the original source had one -- it is not a decomp hack. The
shape is toolchain-invariant: in `jet.c` `func_800A4400` the direct form
scores 615 or more under all five cc1/aspsx pairings the build offers, so a
diff of this shape is a source problem, never an annotation problem.
`func_800A442C`, `func_800A8238` and `func_800A8264` are the same idiom.

**Declarations must start a block.** gcc 2.6.3 is C89: a declaration after a
statement is a `parse error`. Any nested `{ }` opens a new block, which is a
legitimate way to keep a declaration next to its use.

## Data layout failures look like nothing is wrong

If every function scores 0 but the overlay still fails its sha1, the problem
is data, not code. Check the object:

```shell
mipsel-linux-gnu-nm -n build/us/src/magic/brizad.c.o
```

gcc 2.6.3 accepts an **undeclared identifier as an array size** without an
error, silently producing a near-zero-size array. A `#define` placed below
the declaration that uses it will do exactly this, and every function will
still diff clean.

**An explicit `= {0}` keeps a static in `.data`; leaving it uninitialised
moves it to `.bss`.** gcc 2.6.3 only sends tentative definitions to `.bss`,
so `static MATRIX m = {0};` emits 32 zero bytes into `.data` where
`static MATRIX m;` emits none. Zero-filled globals inside a `.data` range
therefore have to carry the initialiser, or the section comes up short and
the sha1 breaks while every function still scores 0.

## Toolchain annotation

A `//! PSYQ=3.3 CC1=2.6.3` comment at the top of a file selects the
compiler, parsed by `tools/ninja/gen.py`.

| annotation | cc1 | aspsx |
|---|---|---|
| PSYQ=3.3 | cc1-psx-26 | 2.21 |
| PSYQ=3.5 | cc1-psx-26 | 2.34 |
| PSYQ=3.6 | cc1-psx-272 | 2.34 |
| PSYQ=4.0 | cc1-psx-272 | 2.56 |

No annotation defaults to cc1-psx-272 / 2.34.

`CC1=2.6.3` / `CC1=2.7.2` overrides the cc1 the `PSYQ=` row selected and
leaves the aspsx version alone, so `PSYQ=3.3 CC1=2.7.2` means cc1-psx-272
with aspsx 2.21 -- a pairing no real SDK release shipped, but the most
common one in this tree.

**Before blaming the annotation, flip it and score the whole file.** A
wrong compiler is not subtle: bginmenu.c built as 2.6.3 breaks four
untouched functions by 15 to 340 points. A single function that is off by
nothing but register names scores the same under both, and is telling you
about the source, not the toolchain.

**Separate the two axes before concluding anything.** A `PSYQ=` row moves
the cc1 and the aspsx version together, so one flip cannot say which of
them broke. `PSYQ=3.3 CC1=2.7.2` changes the compiler alone, holding aspsx
at 2.21; `PSYQ=3.5` changes the assembler alone, holding cc1 at 2.6.3.
Scored across that grid, jet.c fails through a different function on each
axis:

| held | changed | breaks |
|---|---|---|
| aspsx 2.21 | cc1 2.6.3 -> 2.7.2 | `func_800A80F8`, 0 -> 497 |
| cc1 2.6.3 | aspsx 2.21 -> 2.34 | `func_800A4400` and the 3 after, 0 -> 105 |

`func_800A4400` scores 0 under either compiler, so it says nothing about
the cc1 -- but under `PSYQ=3.6`, which moves both axes at once, it looks
like it does. Pick a probe that isolates the axis you are testing.

**Re-run `.venv/bin/python3 tools/ninja/gen.py` after changing the
annotation.** `ninja` happily rebuilds the object with the old `as_flags`,
so the score does not move and the annotation looks innocent. Confirm with
`ninja -t commands <obj> | grep aspsx-version`. Bare `python3` is PyPy and
dies inside gen.py on a nested f-string quote; use the venv.

Most files do not constrain the cc1 at all, so a `CC1=` override is often
inherited noise. Drop it and score the file before keeping it -- jet.c
carried `PSYQ=3.3 CC1=2.7.2` where plain `PSYQ=3.3` matches, and plain
`PSYQ=3.3` is a real SDK pairing.

## Which aspsx version a file wants

The aspsx version leaves a visible mark, so a wrong one is diagnosable
without guessing. The tell is how the assembler expands a store or load
indexed off a symbol. Below 2.30 it materialises the whole address first:

```
lui   $at, %hi(sym)      # aspsx <= 2.21        lui  $at, %hi(sym)
addiu $at, $at, %lo(sym)                        addu $at, $at, $idx
addu  $at, $at, $idx                            sh   $v1, %lo(sym)($at)
sh    $v1, 0($at)
```

`maspsx` calls this `addiu_at` and enables it for `--aspsx-version` below
2.30 (`tools/maspsx/maspsx.py`). **A diff whose only defect is one missing
`addiu $at, $at, %lo(...)` is an annotation problem, not a source
problem** -- no amount of rewriting the C will produce it. The full
behaviour matrix is in `tools/maspsx/README.md`.

## What the ROM says about the SDK version

The `PSYQ=x.y` labels are this repo's naming for an aspsx version, not
something the ROM records. What the disc actually attests:

- **The assembler is aspsx 2.21 (June 1995).** `addiu_at` is required, which
  bounds it at 2.21 or older. Division expands to `break`, never `tge`
  (553 to 0 across `asm/us`), which excludes 2.05/2.08 -- the only versions
  using `tge`. The remaining 1.05/1.07 are pre-1995 plain-DOS builds that
  predate the commercial SDK line.
- **The linked libapi is newer than the assembler.** `disks/us/SCUS_941.63`
  still carries its RCS stamps: `intr.c 1.73` (1995/11/10), `sys.c 1.115`
  (1995/11/29), `bios.c 1.76` (1996/04/03). The overlays carry none; only
  the main executable links libapi.

So the shipped toolchain was mixed -- a mid-1995 assembler against
libraries built in April 1996 -- which is why a single "PSYQ version" never
quite fits the tree.

Retail PSYQ 4.00 and 4.70 are not it: their `.LIB` files carry no `$Id`
strings at all (rebuilt, dated 1997 and 2000), and comparing FF7 against
them by 24-byte code windows gives a longest LIBAPI run of 65 bytes. Pinning
the release rather than bounding it needs a PSYQ 3.x dump -- check that its
`ASPSX.EXE` is 2.21 and its LIBAPI `$Id` set matches the three above.
Reference binaries, if you want to redo this: PSYQ 4.00/4.70 from the psyz
release referenced by `tools/psyz/decomp/sdk/Makefile`, and the real
`ASPSX.EXE` set from the maspsx `aspsx` release. The 16-bit ones need
dosemu2; wine will not run them.

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

**Where you assign decides how long a value lives.** An initialiser at the
top of a function computes the value there. If it is not consumed until after
several calls, gcc must park it in a callee-saved register, adding a
save/restore and growing the stack frame. Assign at the point of use instead.

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

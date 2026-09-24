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

**A store at `reg + offset` beyond 16 bits, where the register holds a
known global address, means the source held that byte's address in a
pointer.** CSE (`find_best_addr` in cse.c) folds every `reg + const` store
address whose register it knows to be a constant, so `db[1].draw.isbg = 0`
compiles to the absolute `sb $0, sym+0x12674` whether `db` is the array or a
pointer to it. A plain register address is never folded, and combine later
merges the pointer arithmetic back into the store, giving the target's
`sb $0, 0x12674($s2)` that the assembler expands with `$at`. Only this shape
reproduces it, and the pointer must be computed with no call in between:

```c
Unk800D1964* db = D_800AB898;
u_char* isbg;
isbg = &db[1].draw.isbg;      /* jet.c func_800A7C88 */
*isbg = 0;
```

Neither a chained assignment, a volatile pointer, an index variable nor a
`DRAWENV*` to the second buffer works: each leaves a `reg + 0x18` address
that CSE folds. The pattern is unique in the tree, so treat it as a
last-resort explanation, not a first guess.

**A commutative operator's operand order follows what the operands were at
expansion time.** `global + local` puts the local first (`addu v0,local,base`):
gcc swaps a memory first operand behind a register second one before it loads
the memory into a register. Two memory operands, or two registers, keep the
source order. When the target has the loaded global first and the local
second, the file was built with `-fforce-mem`, which loads memory operands
before that swap test: `jet.c` carries `FORCE_MEM=true` for exactly this
reason, and the flag took `func_800A372C`, `func_800A3B58` and
`func_800A35DC` from 20 to 60 down to 0 while every matched function stayed
at 0. The one casualty, `func_800A7B48`, was a pointer local
`info = &D_800A89D8[arg0]`: under the flag the base is a register before the
add, so the pointer sum keeps base-first order, where the target's index-first
order is the memory-address path. Indexing the global directly at each use
restored it. Applying the flag tree-wide breaks six other overlays, so it is
per file, not a default.

**A value stored to an array and reused was read back from the array, not kept
in a local.** cse forwards the store, so `xs[i] = word; if (minX > xs[i]) minX
= xs[i];` and a version holding `word` in a local produce the same
instructions, but the local is a user variable whose pseudo exists from the top
of the function, while the forwarded value is a temporary created at the load.
The two get different registers, and with the scheduler that moved a load past
a sign extension in `func_800A0874`. When the register order is the only fault
and a local is only read once after its store, drop the local. Likewise `minX >
xs[i]` and `xs[i] < minX` compile to the same compare but extend their operands
in source order, which decides what fills the load delay slot.

**A pointer assigned from an array element keeps the base first.** `spawn =
&spawns[idx]` expands as a plain add of two registers, base then index, while
the same address formed inside a memory reference goes through the address
path, which puts the scaled index first (`addu v0,v0,a3`). When the target has
index-first for a pointer that is then dereferenced several times, the original
did not build that pointer with `&array[idx]`. Every C pointer sum (`p + i`,
`i + p`, `&p[i]`) expands base-first; only integer arithmetic keeps source
order, so `(T*)(idx * sizeof(T) + (u32)base)` produces index-first. That took
`func_800A4458` in `jet_object.c` from 60 to 50.

**The loop pass orders hoisted setups, and it will not hoist a user variable
set after a conditional jump.** The last 50 points of `func_800A4458` were two
address setups in the preheader: the target materialises the copy destination
(`g_JetSpawnTemplate+0x78`) before `&g_JetSpawnIndex`. The `-dL`/`-dS`/`-dR`
dumps show the loop pass emits them in scan order and both schedulers keep it.
The destination is a compiler temporary made inside the copy loop, hoisted out
of all three loops. A pointer local to the counter is a user variable, and
gcc 2.6.3 `loop.c` (`scan_loop`) moves a set only if (1) the reg is used only in
the set's basic block, (2) it is not a user variable and not used in the exit
test, or (3) `maybe_never` is clear and the reg is not used before the set.
`maybe_never` is set by any label or jump and cleared only at a depth-0
`NOTE_INSN_LOOP_VTOP`, so the inner loop's copied `blez` blocks case 3. Loops
are scanned inner first and `move_movables` emits in scan order before
`loop_start`, so a pointer local set anywhere in the segment body lands first,
and set inside the inner loop it is not hoisted at all.

The fix is to read the counter through an integer cast at every use,
`*(s32*)(u32)&g_JetSpawnIndex` (a `(u8*)` byte-offset cast works too). The cast
keeps expand from folding the address into a `mem (symbol_ref)`, so it lives in
a temporary created at each use -- after the destination -- and case 2 hoists
it. The same cast through a pointer local does not work: the local is still a
user variable. All three counter accesses need it. With the counter fixed, the fields read as
`spawns[index].field` from a value local match too, so the integer-sum pointer
above is not needed in the final code. Dead ends: an inline helper taking `&g_JetSpawnIndex` (integrate
substitutes the constant and marks the parameter copy as a user variable), the
PC port's direct-global form (gcc proves the counter cannot alias the stores
and stops reloading it), and 70k permuter iterations.

**Declarations must start a block.** gcc 2.6.3 is C89: a declaration after a
statement is a `parse error`. Any nested `{ }` opens a new block, which is a
legitimate way to keep a declaration next to its use.

**A constant array index whose element splat named separately scores
nonzero and is still correct.** `D_800A8990[1]` assembles to
`%hi(D_800A8990)` + `%lo(D_800A8990+0x2)`, while the target names the element
splat labelled, `%hi(D_800A8992)` + `%lo(D_800A8992)`. The `%hi` is the same
word either way, so the linked bytes are identical and only the relocation's
symbol differs -- which asm-differ cannot see through. `func_800A2518` in
`jet.c` sits at 220 for 24 such stores and the overlay still passes its sha1.
Let `make build` settle it rather than inventing per-element externs.

**A small helper that is "almost" matched everywhere it is pasted was
inlined.** gcc 2.6.3 inlines an `inline` function into every later caller in
the file and still emits the standalone copy, so the ROM has both.
Hand-expanding the body at each call site never matches: the inlined copy
sign-extends its `s16` parameters late, after the callee's other stores, and
the extension and the store addressing differ from what direct assignments
produce. Mark the helper `inline`, write the call, and shape the arguments: a
struct field passed directly is narrowed to `lhu` at the call, so load it into
an `s32` local first (`lw` + late `sll/sra`); make those locals block-scoped,
since gcc only ties a dying input to the output register (`sll s0,s0`) for
pseudos that live in one basic block; and write the helper's own stores as
plain member assignments, not `setVector`, or a loop that hoists the struct
address keeps `lui at` forms for the vector fields. `func_800A4650` in `jet.c`,
matched into `func_800A6B08` and `func_800A6BD8`, is the worked example.

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

`FORCE_MEM=true` adds `-fforce-mem` to cc1 for that file (see the operand
order gotcha above). `G=`, `O=`, `COMM=`, `g=` and `gcoff=` are the other
keys `parse_compiler_params` accepts.

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

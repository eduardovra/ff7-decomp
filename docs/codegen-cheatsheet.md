# Codegen cheat sheet

Keyed by what the **target** looks like. The answer is the C shape that
produces it, because that is the direction the work actually runs: you have
the assembly, you need the source.

Companion to the "Codegen gotchas" section of `docs/decomp-workflow.md` --
that one collects traps found while matching, this one collects patterns
recognisable on sight.

Four groups, ordered the way you meet them, and **most common first**
inside each:

1. **Reading a function** -- the frame, and calls.
2. **Data access** -- fields, pointers, arrays.
3. **Control flow** -- branches, loops, `switch`.
4. **Arithmetic and types** -- compares, casts, shifts, division.

If you are new to this, read the next section, then group 1, and come back
for the rest when you hit them.

Every snippet below is copied from this tree. To regenerate one, and to get
the source-line markers that prove which C produced which
instructions, use `-dl`:

```shell
mipsel-linux-gnu-objdump -dl --no-show-raw-insn build/us/src/<path>.c.o
```

These are unlinked objects, so relocated fields read as zero: `lui at,0x0`
and `jal 0 <FirstSymbolInFile>` are unresolved relocations, not addresses.
`LM<n>` labels are line-number markers from the assembler, not real symbols.

## Reading the assembly at all

If MIPS is new to you, three facts explain most of what looks strange.

**Registers have conventional roles.** The disassembly uses the names, not
the numbers.

| register | role |
|---|---|
| `zero` | always 0; writes are discarded |
| `at` | scratch, reserved for the assembler's macro expansions |
| `v0`, `v1` | return values, and general scratch |
| `a0`-`a3` | first four arguments |
| `t0`-`t9` | scratch, not preserved across a call |
| `s0`-`s7` | preserved across a call, so the callee saves them |
| `gp` | small-data base; see `-G0` below |
| `sp`, `fp`, `ra` | stack pointer, frame pointer, return address |

A value in `s0`-`s7` is alive across a `jal`. A value in `t0`-`t9` is not.
That alone often tells you where a variable's scope begins and ends.

**The instruction after a branch or jump always executes.** That is the
branch delay slot, a hardware rule, not an optimisation. So this:

```
1cc:    beqz    v0,33c <LM100>
1d0:    sll     v0,v1,0x2
```

runs the `sll` whether or not the branch is taken. When reading, mentally
lift the delay-slot instruction above the branch. Nothing in your C
controls which instruction lands there.

**Nothing is in source order.** This is `-O2`. Loads get hoisted, stores
sink, and unrelated statements interleave. Match the *set* of instructions
first and worry about ordering second.

**`gp` shows up in `src/main`, and nowhere else.** Thirteen files there
carry `G=8` in their `//! PSYQ=` annotation, which turns on the small-data
section, so their globals load as `lw v0,8(gp)`. Everything else in the
tree defaults to `-G0` and reaches a global through a `lui`/`addiu`
relocation pair instead. If you are matching an overlay and your build
emits `gp` loads, you have the annotation wrong.

One last note: `LM<n>` labels in the disassembly are line-number markers
emitted by the assembler, not functions.

## Reading a function

### The prologue and epilogue -- what the frame tells you

```
  0:    addiu   sp,sp,-80
 14:    sw      ra,72(sp)
 18:    sw      s1,68(sp)
 1c:    sw      s0,64(sp)
...
174:    lw      ra,72(sp)
178:    lw      s1,68(sp)
17c:    lw      s0,64(sp)
180:    addiu   sp,sp,80
184:    jr      ra
```

Against a leaf, which has no frame at all:

```
  0:    sw      a0,0(gp)
  4:    jr      ra
  8:    nop
```

```c
void SysGzipSetDataBlock(u8* arg0) { s_Bin = arg0; }
```

**Tell:** **no `sw ra` means the function makes no calls**, full stop. That
is the fastest thing you can learn about an unknown function, and it costs
one glance.

Two more things you get for free before reading any of the body:

- **Count the saved `s` registers.** They hold values that must survive a
  call, so roughly that many locals are live across one. A non-leaf saving
  no `s` register at all had nothing worth keeping.
- **The bottom of the frame is reserved** for outgoing arguments, so
  locals and register saves start above it. The block is 16 bytes at
  minimum, but it **grows with the widest call the function makes** -- see
  the frame map entry below. This is why the smallest non-leaf frame is
  `addiu sp,sp,-24`.

The `sp` restore lands in the `jr ra` delay slot when there is nothing else
to put there, and above the `jr` when the delay slot was already spoken
for. Both forms are normal; neither means anything about the C.

Sources: `src/magic/brizad.c`, `BrizadRenderIce`; `src/main/15B44.c`,
`SysGzipSetDataBlock`.

### `jal`, the argument registers, and why you cannot count arguments

```
868:    addiu   a0,sp,56        # arg 1
86c:    li      a1,0xc8         # arg 2
870:    li      a2,0x5          # arg 3
874:    li      a3,0x4e         # arg 4
878:    li      v0,0x18         # arg 5, staged
87c:    jal     0 <func_801D0408>
880:    sw      v0,16(sp)       # arg 5 stored, in the DELAY SLOT
```

```c
SysMenuSetWindowRect(&sp38, 200, 5, 78, 24);
```

**Tell:** the first four arguments go in `a0` through `a3` in order, and
the fifth onward are stored to `16(sp)`, `20(sp)`, `24(sp)`, that is
`4 * (n - 1)`. The last argument almost always sits in the `jal` delay
slot, so read that instruction as part of the call, never as part of what
follows. A null pointer appears as `move a3,zero`, not `li a3,0`.

The return value arrives in `v0`, readable only from the instruction
*after* the delay slot.

**Three traps here, all of which bite beginners.**

**The `jal` target name is usually a lie.** These are unlinked objects, so
a cross-file call is an unresolved relocation and objdump prints whatever
symbol happens to sit at offset 0. A name is trustworthy only when the
target offset is non-zero and resolves inside the same object, which in
practice means a `static` callee:

```
bc8:    jal     50 <huft_build>       # real: a local static at 0x50
88:     jal     0 <func_800144D8>     # not real, just whatever is at 0
```

**`sw <something>,16(sp)` is ambiguous.** It is a fifth outgoing argument
when a `jal` follows within an instruction or two, and a saved register
otherwise. Same address, two meanings.

**You cannot tell a callee's arity from the call site.** A live incoming
parameter left sitting in `a0` is indistinguishable from a deliberate
argument. Here `cb` is still in `a0` at the `jal`, and the callee takes
nothing at all:

```
 ec:    move    s0,a0           # cb parked in a callee-saved register
 f4:    jal     0 <...>         # SystemCdromReadChain(void)
 f8:    nop
```

```c
u32 SystemCdromReadChain(void);   /* include/game.h */
...
while (SystemCdromReadChain()) {
```

Writing `SystemCdromReadChain(cb)` from that call site would be entirely
reasonable and entirely wrong. The only reliable method is to disassemble
the **callee** and see which argument registers it reads before writing.
Argument registers are caller-saved, so anything you did not set yourself
is formally undefined, not zero.

**Calls through a pointer use `jalr`:**

```
10c:    jalr    s0
```

The pointer lives in a callee-saved register because it has to survive the
other calls around it. The dispatch-table flavour, which is the shape most
FF7 opcode handlers take, is unmistakable once seen:

```
bb0:    sll     v0,v0,0x2       # opcode * 4
bb4:    addu    v0,v0,s3        # + table base
bb8:    lw      v0,0(v0)        # load the handler
bc0:    jalr    v0
```

Sources: `src/menu/savemenu.c`; `src/main/unzip.c`, `huft_build`;
`src/main/144D8.c`, `func_800145BC`; `src/field/field_event.c`.

### The frame map -- four zones, and where your locals start

Every frame has the same four zones, bottom to top:

| zone | contents |
|---|---|
| outgoing arguments | home slots for the four register args, then stack args |
| locals | aggregates and spills |
| saved registers | `s0`..`s8`, then `ra`, at the top |
| above `sp+N` | the caller's frame, where your own stack args live |

**The outgoing block is not fixed at 16 bytes.** It is 16 plus four per
argument beyond the fourth, rounded up to 8, sized by the **widest call the
function makes**. That single fact decides where local 1 sits, so getting
it wrong misreads every offset above it.

A function whose widest call takes five arguments pushes its first local to
24:

```
4bc:    li      v0,0xf0
4c0:    sw      v0,16(sp)       # the 5th argument
...
570:    addiu   a0,sp,24        # &rect, the first local
```

A function in the same tree whose calls all fit in registers puts its first
local at 16:

```
1e8c:   sh      zero,16(sp)     # rect.x
1eb0:   addiu   a3,sp,16        # &rect
```

```c
RECT rect;
...
rect.x = 0;
...
SysMenuSetDrawMode(0, 1, (u16)GetTPage(0, 2, 0x3C0, 0x100), &rect);
```

**Reading a frame, in order:** take the size from `addiu sp,sp,-N`, read
the saved registers off the prologue, then find the highest offset stored
to just before a `jal`. That marks the top of the outgoing block. Locals
are what lies between.

**A struct local is a contiguous run**, and `addiu aN,sp,<offset>` taking
its address is how it reaches a callee. A whole-struct assignment between
two stack slots shows up as unaligned moves when the type is only
halfword-aligned, which looks alarming and is ordinary:

```
5ac:    lwl     v0,27(sp)
5b0:    lwr     v0,24(sp)
5bc:    swl     v0,35(sp)
5c0:    swr     v0,32(sp)
```

```c
prevRect = rect;
```

Sources: `src/main/swirl.c`, `SysBattleSwirlInit`; `src/main/1CDA4.c`,
`SysMenuDrawCursor`.

### Does declaration order decide the stack slot? Partly

This one matters because it is a lever: when a local is in the wrong slot,
you want to know what moves it.

**Aggregates: yes.** Structs and arrays are laid out in ascending offsets
in declaration order, consistently across every function checked, in three
files and under both compilers. One of them has five arrays whose
declaration order is deliberately not their first-use order, and the slots
still follow the declarations.

**Address-taken scalars: no.** Here the declaration order is one thing and
the slot order is another:

```
c54:    lw      a0,1184(sp)     # tl
c58:    lw      a1,1192(sp)     # td
c5c:    lw      a2,1188(sp)     # bl
c60:    lw      a3,1196(sp)     # bd
```

```c
int i;
struct huft* tl;
struct huft* td;
int bl;
int bd;
unsigned l[288];
```

Declared `tl, td, bl, bd`; laid out `tl, bl, td, bd`. The array declared
last sits lowest of all. The order that fits is grouping by the call that
first takes each address, but that is a reading of two examples in one
file, not a rule I can stand behind.

**So:** to move an aggregate, reorder the declarations. To move an
address-taken scalar, reordering declarations is unlikely to help; the
lever is the order of the calls that take its address. Neither has been
confirmed by rebuilding.

### A spill is not a local you forgot

```
720:    sw      t0,40(sp)       # written once
...
744:    lw      t0,40(sp)       # reloaded every inner iteration
748:    nop
```

```c
buf = &SWIRL_DATA->buffers[0];
for (j = 0; j < SWIRL_QUAD_ROWS; j++) {
    for (i = 0; i < SWIRL_QUAD_COLS; i++) {
        SetPolyFT4(&buf->quads[i][j]);
```

**Tell:** a stack slot written once and read many times, with **no
`addiu aN,sp,K` anywhere** taking its address, is a spill rather than a
user aggregate. Here the value is loop-invariant and computable in two
instructions, yet it is reloaded on every one of seventy inner iterations,
and the load-delay slot cannot even be filled.

The reason is visible in the same window: every one of `s0` through `s8` is
live. There was no register left.

**What to conclude, and what not to do.** A spill is direct evidence that
the original had more simultaneously live locals than there are
callee-saved registers. Do not delete the reload, and do not hoist the
value. If you reduce register pressure elsewhere the spill disappears and
the function stops matching.

Source: `src/main/swirl.c`, `SysBattleSwirlInit`.

## Data access

### A load at a constant offset -- a struct field

```
28:     lh      v1,24(s0)
2c:     lh      a0,2(s0)
```

```c
/* 0x02 */ s16 AnimationFrame;
/* 0x18 */ s16 Scale;
...
growth = (effect->Scale * GROWTH_PER_FRAME);
scale = (effect->AnimationFrame * growth) >> 12;
```

**Tell:** the immediate in the load *is* the field's offset, printed in
decimal where the struct comment uses hex -- 24 is `0x18`, 2 is `0x02`. The
base register holds the struct pointer. This is the most common instruction
in the tree, and reading offsets back out of it is how the structs in
`include/` got written in the first place.

Source: `src/magic/brizad.c`, `BrizadRenderIce`.

### The load mnemonic is the field's type

| C type | load | store |
|---|---|---|
| `s8` | `lb` | `sb` |
| `u8` | `lbu` | `sb` |
| `s16` | `lh` | `sh` |
| `u16` | `lhu` | `sh` |
| `s32`, `u32`, any pointer | `lw` | `sw` |

Four fields copied between two structs, one line of C each:

```
1008:   lbu     v0,0(a1)        # u8  char_id   @ 0x00
1010:   sb      v0,4(a0)        # u8  charId    @ 0x04
1020:   lbu     v0,2(a1)        # u8  strength  @ 0x02
1028:   sh      v0,8(a0)        # u16 baseStat[0]
1068:   lhu     v0,46(a1)       # u16 hp_base   @ 0x2E
1070:   sh      v0,20(a0)       # u16 baseStat[6]
1080:   lw      v0,60(a1)       # u32 exp       @ 0x3C
1088:   sw      v0,44(a0)       # u32 expStart  @ 0x2C
```

```c
p->charId = c->char_id;
p->baseStat[0] = c->strength;
p->baseStat[6] = c->hp_base;
p->expStart = c->exp;
```

**Tell:** the load is chosen by the *source* field's type and the store by
the *destination's*, independently. A `u8` widened into a `u16` is just
`lbu` plus `sh`; nothing else marks the conversion. Stores carry no
signedness at all -- `sb`/`sh`/`sw` only say how many bytes.

So when your build emits `lbu` where the target has `lb`, the mistake is
the field's declared signedness in the header, not the expression you wrote
around it. Two loads one byte apart off the same base, differing only in
mnemonic, pin down two adjacent fields of different signedness:

```
146c:   lbu     v1,0(at)        # u8 mul @ 0x0
1490:   lb      a0,1(at)        # s8 add @ 0x1
```

Sources: `src/battle/batres.c`, `LoadResultsRow` and `CalcStatGrowth`.

### A second load whose base was just loaded -- a pointer field

```
1448:   lw      v0,40(a0)       # p->curve @ 0x28 -- loads an ADDRESS
144c:   lw      a0,0(a0)        # p->tier  @ 0x00 -- loads a VALUE, done
1450:   addu    v0,v0,a2
1454:   lbu     v0,0(v0)        # *(curve + gauge) -- the second load
```

```c
/* 0x00 */ s32 tier;
/* 0x28 */ u8* curve;
```

**Tell:** a plain field read finishes in one load. A pointer field takes a
load to fetch the address, then another load *whose base register is the
first load's destination*. That chaining is the signature, and it is what
distinguishes `p->tier` from `p->curve[gauge]` at a glance.

Source: `src/battle/batres.c`, `CalcStatGrowth`.

### `sll`, `addu`, load -- array indexing

The shift amount is `log2` of the element size.

```
2e4:    sll     v0,s0,0x2       # kind * 4
2e8:    addu    v0,v0,s1        # + base
2ec:    lw      a0,0(v0)        # D_80048DD4[kind], a 4-byte array
```

```c
} else if (D_80048DD4[kind]) {
```

Two bytes shifts by 1, and a 32-byte struct by 5:

```
41fc:   sll     v0,v0,0x1       # u16 g_FieldScriptPC[]
4204:   lhu     v1,0(v0)
```
```
20:     sll     v0,v0,0x5       # BrizadData is size:0x20
24:     addu    s0,v0,v1
```

**Tell:** shift, add, load. A **one-byte** element has no shift at all,
just a bare `addu` before the load -- a missing shift means stride 1, not a
missing index. The `lui`/`addiu` pair that usually sets up the base is an
unresolved relocation, so ignore its zero.

Sources: `src/main/144D8.c`, `func_80014750`; `src/field/field_opcodes.c`,
`OpcodeFuncWait1`; `src/magic/brizad.c`, `BrizadRenderIce`.

### A shift/add chain before the base -- and the chain is the `sizeof`

gcc never multiplies to index an array here. It strength-reduces the size
into shifts, adds and subtracts, which means **you can read the element
size straight out of the chain** -- often the fact you were missing.

```
c94:    sll     v0,v1,0x2       # id*4
c98:    addu    v0,v0,v1        # id*5
c9c:    sll     v0,v0,0x2       # id*20
ca0:    lui     at,0x0
ca4:    addu    at,at,v0
ca8:    lbu     v0,13(at)       # .materiaType @ 0x0D
```

```c
} MateriaData; // size: 0x14
...
if (id != 0xFF && (g_MateriaData[id].materiaType & 0xF) == 7) {
```

`x*20 = ((x<<2) + x) << 2`, and `0x14` is 20. Other sizes in this tree:

| size | chain |
|---|---|
| 3 | `sll 0x1`, `addu` |
| 14 | `sll 0x3`, `subu`, `sll 0x1` |
| 20 | `sll 0x2`, `addu`, `sll 0x2` |
| 56 | `sll 0x3`, `subu`, `sll 0x3` |

Note the `subu`: gcc subtracts as readily as it adds.

Two things that mislead here. The field offset gets **folded into the load
immediate** (`13(at)` above), so the element's own address is never
materialised. And in the snippet the `sll` sits in the delay slot of the
`beq` above it, so it runs on both paths.

For a **2D** array you get two shifts feeding one `addu` -- the larger is
the outer stride, the smaller the element:

```
1458:   sll     a0,a0,0x1       # tier * 2   (element, StatGrowth is 2)
145c:   sll     v0,v0,0x4       # row * 16   (8 elements per row)
1460:   addu    a0,a0,v0
```

**Caveat:** inside a loop gcc usually drops the index entirely and walks a
pointer, so the size appears once as a loop-bottom bump and no chain exists
at all. Here 132 is `sizeof(SavePartyMember)`, `0x84`:

```
de8:    addiu   s4,s4,1
dec:    slti    v0,s4,9
df0:    bnez    v0,d40 <LM74>
df4:    addiu   s3,s3,132
```

Sources: `src/battle/batres.c`, `GiveMateriaAp`, `CalcStatGrowth`,
`InitResultsRow`, `GiveSharedExp`; `src/battle/battle.c`, `func_800A311C`;
`src/menu/title.c`.

### Batched `lw`/`sw` quads with no call -- whole-struct assignment

```
3a8:    lw      v0,0(s1)
3ac:    lw      v1,4(s1)
3b0:    lw      a0,8(s1)
3b4:    lw      a1,12(s1)
3b8:    sw      v0,0(s4)
3bc:    sw      v1,4(s4)
3c0:    sw      a0,8(s4)
3c4:    sw      a1,12(s4)
```

```c
*matrix2 = *matrix1;
```

**Tell:** four `lw` into `v0`/`v1`/`a0`/`a1`, four matching `sw`, repeating
until the struct is consumed -- two rounds here for a 32-byte `MATRIX`. No
`jal`, so it is neither a loop nor a `memcpy`. The total byte count gives
you the struct's size, which is often the fact you were missing.

Source: `src/magic/barrier.c`, `BarrierRenderShield`.

### Unions -- free, invisible, and usually not recoverable

A union emits nothing of its own. No tag, no branch, no conversion. The
member you name simply picks the width, the signedness and the sub-offset
at one address. The same field read three ways:

```
690c:   lw      a3,0(at)        # .ptr, a word
c2dc:   lhu     v0,16(s1)       # .unk.unk0, u16 at +0
c4a0:   lh      a2,18(s1)       # .unk.unk2, s16 at +2
```

```c
typedef union {
    u8* ptr;
    Unk80162200 unk;
} Union80162200;
```

**Tell:** one base and displacement reached by loads of different widths.

**Now the honest part: you cannot recover the union.** What the object
tells you is that an address is accessed at more than one width. At least
four different C spellings produce that, and this repo contains three of
them for the same situation: a union, a pointer cast, and a cast to a
different struct. Codegen does not distinguish them.

So treat a union as a documentation decision, not a reverse-engineering
finding. Because it costs nothing, it also cannot break a match. If the
union spelling and the cast spelling both score zero, both are correct, and
neither is evidence about the original source.

**One exception, where the union is load-bearing.** Reading the top half of
a fixed-point word through a member is a narrow load at an offset. Writing
it as a shift is a word load plus an arithmetic shift. Those do not match
each other:

```
5064:   lui     v1,0x0
5068:   lhu     v1,2(v1)        # .i.hi
```

```c
// 16.16 fixed point volume
typedef union {
    s32 val;
    struct {
        s16 lo;
        s16 hi;
    } i;
} AkaoCdVol; /* size = 0x4 */
...
D_8009C578.unk12 = g_AkaoCdVol.i.hi;
```

The high half sits at `+2` because the target is little-endian. A narrow
load at a non-zero offset into a value the rest of the function treats as
one word is the signature.

**Two layout consequences worth remembering.** A union's alignment is the
widest member's, so a union of a `u16` and a pointer pads like a pointer.
And on a union of pointers, where every load looks identical, the only
trace of which member was used is the increment: a cursor bumped by 16 is
one primitive type, bumped by 40 is another. **The stride is the `sizeof`.**

Sources: `src/battle/battle2.c`; `src/main/akao.c`; `src/main/unzip.c`;
`src/menu/title.c`.

### Bitfields -- rare here, and one of them is invisible

Worth knowing mainly so you recognise it in the display-list code, which is
the only place it occurs. There are exactly four bitfield declarations in
this tree, both structs live in the vendored SDK headers, and **none are
declared in `src/` at all**. One of the two structs has no users.

```c
typedef struct {
#ifndef __psyz
    unsigned addr : 24;
    unsigned len : 8;
#else
    u_long addr;
    u_long len;
#endif
    u_char r0, g0, b0, code;
} P_TAG;
```

(The alternate arm is for the PC port, which does not use bitfields.)

This is the head of every display-list packet: a 24-bit link to the next
packet, then a packet length. Reaching it is itself a type pun, since no
primitive struct declares a `P_TAG`; the SDK macros cast to it.

**A write is a read-modify-write, and it is unmistakable:**

```
11fc8:  lui     a0,0xff
11fcc:  ori     a0,a0,0xffff    # 0x00FFFFFF, the addr mask
...
12010:  lw      v0,0(s2)
12014:  and     v1,v1,a1        # clear the field in the target
12018:  and     v0,v0,a0        # mask the incoming value
1201c:  or      v1,v1,v0        # merge
12020:  sw      v1,288(s0)
```

```c
addPrim(ot, poly);
```

**Tell:** load, two `and`s, an `or`, store, on one address, with two
constants that are complements of each other. The complementary pair is the
giveaway. A 24-bit mask cannot fit an immediate, so it gets built with
`lui`/`ori` and hoisted; a field of 16 bits or fewer would be a single
`andi` instead.

A read of `addr` is just the load and one `and`, with no shift, because the
field starts at bit 0. **A field that does not start at bit 0 would add a
shift pair**, but no such field exists in this tree, so treat that as
theory rather than something you will meet here.

**The invisible case, which matters more.** A bitfield that is exactly
eight bits wide *and* byte aligned degenerates completely. No read, no
mask, no merge, just a store:

```
14de8:  li      v0,0x9
14dec:  sb      v0,-26(s0)
```

```c
setlen(prims, 9);
setcode(prims, 0x2E);
```

That is byte-identical to writing through a plain `u8`. So in this one
struct, the 24-bit link is unmistakable and its 8-bit sibling is
indistinguishable from an ordinary byte field.

One caveat on provenance: the world overlay is built without debug
sections, so that last window is anchored by its symbol and its constants
rather than by line markers. Every other snippet in this file is anchored
by `-dl`.

Sources: `include/psxsdk/libgpu.h`; `src/battle/battle1.c`;
`src/world/world.c`.

### A redundant-looking load is never noise

The project rule says repeated loads and redundant re-tests are
load-bearing and must not be simplified away. Here is what they actually
look like, and the three different things they mean. You can tell them
apart from the instruction window alone.

**Cause 1, `volatile`: repeated loads with nothing in between.** Three
loads of the same address inside twenty-one instructions, no call, no
store, and the sign-extension repeated each time:

```
7ac:    lui     v0,0x0
7b0:    lhu     v0,0(v0)
7c0:    beq     v0,v1,848 <LM83>
7c8:    lui     v0,0x0
7cc:    lhu     v0,0(v0)
7dc:    beq     v0,v1,848 <LM83>
7e4:    lui     v0,0x0
7e8:    lhu     v0,0(v0)
```

```c
extern volatile s16 D_800965EC;
...
if (D_800965EC != 5 && D_800965EC != 13) {
    if (D_800965EC != 2) {
```

The sharpest form is a store followed one instruction later by a reload of
the same address into the same register. Nothing but `volatile` does that:

```
558:    sw      v0,0(v1)
55c:    lw      v0,0(v1)
```

```c
/* 0xB88 */ volatile u32 game_timer_fraction;
...
Savemap.game_timer_fraction += 1092; // 65536 / 1092 = ~60
if (Savemap.game_timer_fraction >> 16) {
```

Note what is **not** repeated: the address in `v1` is computed once and
reused. `volatile` forbids caching the value, not the address. If the
address is recomputed too, `volatile` is not your explanation.

**Cause 2, a call in between.** A plain non-volatile global must be
re-read after any call, because the compiler cannot prove the callee did
not write it. Same repeated-load shape, distinguished **only** by the
`jal`s:

```
4164:   lw      v0,0(v0)
4178:   jal     0 <...>
4188:   jal     0 <...>
4190:   jal     0 <...>
419c:   lw      v0,0(v0)        # reload, after three calls
```

**Cause 3, a store that might alias.** No call and no `volatile`, but each
store through a pointer could, as far as the compiler knows, overwrite the
pointer itself, so every store kills the cached load:

```
1e04:   lw      v1,0(v1)
1e0c:   sb      v0,12(v1)
1e14:   lw      v1,0(v1)        # reloaded, no call between
1e1c:   sb      v0,13(v1)
```

Here the **address is recomputed too**, which is the tell that separates
this from `volatile`.

**Use it as a checklist.** When you have a stray load you cannot reproduce,
ask in order: is the object `volatile`, is there a call you inlined away,
or did you turn a store-through-pointer into something the compiler can now
disambiguate.

### `volatile` also pads a frame

An unused local cannot survive optimisation unless it is `volatile`, which
makes it the standard trick for getting a frame size to match. All four
local `volatile` declarations in this tree are exactly this, unused dummies
that exist to occupy stack.

```
65c:    addiu   sp,sp,-80
```

```c
void func_801D069C(void) {
    volatile s32 dummy;
    s32 i;
    SysMenuSetCursorMovement(...);   /* 14 arguments */
```

The fourteen-argument call fills the outgoing block through offset 55, and
the saved registers begin at 64. **Offsets 56 through 63 are never touched
by any instruction in the function.** That hole is `dummy` plus alignment.
Without the declaration the frame would be 72, not 80.

**Tell:** an untouched gap between the top of the outgoing block and the
first saved register means the original declared something there. If your
frame is short by a slot and nothing else differs, this is why.

One thing that is **not** this feature: `__asm__ volatile`, which is most
of the occurrences in this tree and all of them in the graphics coprocessor
header. That keeps an instruction block from being deleted or moved. It is
not an object qualifier and has none of the behaviour above.

Sources: `src/main/110B8.c`, `SysFieldRun` and `VSyncCallbackFunc`;
`src/main/akao.c`; `src/main/1CDA4.c`; `src/menu/cnfgmenu.c`.

## Control flow

### `if` -- the branch is the escape hatch, not the entrance

```
 74:    bnez    v1,a4 <LM18>    # target a4 is line 36, the closing brace
 7c:    lhu     v0,2(s0)        # the body
```

```c
if (D_80062D98 == 0) {
    effect->AnimationFrame++;
```

**Tell:** the C tests equal to zero and the assembly tests *not* zero. The
emitted branch is the negation of the C condition, and it jumps **past**
the body. The body is the fall-through.

**But do not memorise that as a table.** It describes the usual layout, not
a fixed mapping. When the then-block ends in a `return`, gcc puts the else
path in the fall-through and branches *into* the body instead, so the test
comes out uninverted:

```
26a0:   bgez    a0,26b8 <LM315> # target is line 351, the body itself
26a8:   lui     v0,0x0          # line 355, the fall-through return
```

```c
if (arg0 >= 0) {
    D_800F39E0 = arg0;
    ...
    return 0;
}
return D_800F39E0;
```

The reliable method is one step longer and never wrong: **read the source
line of the branch target.** Past the body means inverted. The body itself
means not.

Sources: `src/magic/fire.c`, `FireRenderSprite`; `src/battle/battle.c`,
`func_800A37F8`.

### Which branch instruction, and why there is no `blt`

Comparisons **against literal zero** are a single instruction. The machine
encodes six of them:

| C test | branch emitted to skip the body |
|---|---|
| `x == 0` | `bnez` |
| `x != 0` | `beqz` |
| `x > 0` | `blez` |
| `x <= 0` | `bgtz` |
| `x >= 0` | `bltz` |
| `x < 0` | `bgez` |

Equality against **anything else** is also one branch, `beq` or `bne`, but
the other operand has to reach a register first.

Every **ordered** comparison against a non-zero value needs two
instructions, because **there is no `blt` on this machine**. A
`slt`/`sltu`/`slti`/`sltiu` produces a 0 or 1 in a temporary, then a
`beqz`/`bnez` branches on that:

```
 94:    slti    v0,v0,14
 98:    bnez    v0,a4 <LM18>    # x >= 14, so skip when x < 14
```
```
2d4:    slti    v0,v0,2
2d8:    beqz    v0,2e4 <LM63>   # x < 2, so skip when x >= 2
```

```c
if (effect->AnimationFrame >= 14) {
if (g_BattleEffectCount < 2) {
```

**Tell:** the same `slti v0,v0,N` serves both directions. Only the
`bnez`/`beqz` that follows says which way the C read.

A constant folds into the `slti` immediate, so you can read it straight
off. Equality has no immediate form, so the constant costs a separate load
into a scratch register first:

```
11dc:   li      v0,0x40
11e0:   bne     a0,v0,1248 <LM19>
```

```c
if (i == 64) {
```

**One warning.** A `li at,-1` followed by `bne`, then `lui at,0x8000` and
`break 0x6`, is **not** an `if`. It is the assembler's divide-overflow
check. Nothing in this tree puts a source-level comparison constant in
`at`; real comparisons use an ordinary temporary.

Sources: `src/magic/fire.c`; `src/battle/battle.c`,
`BATTLE_SetFocusedActor`.

### `if`/`else`, `&&`, `||` and the ternary

**`if`/`else` has three landmarks.** The inverted test jumps to the else
label, and an unconditional `j` at the end of the then-block hops over the
else to the join:

```
 84:    bgez    v1,94 <LM14>    # to the else, line 64
 8c:    j       a4 <LM15>       # end of then-block, to the join at line 66
 90:    move    s1,zero         # the entire then-block, in the delay slot
 94:    addu    v0,v0,v1        # the else body
```

```c
if (frame < 0) {
    fade = 0;
} else {
    fade = frame * FADE_PER_FRAME;
}
```

A `j` sitting immediately before a label that an earlier branch targets is
the signature of an `else`. A bare `if` has no such `j`.

**`&&` is two branches to the same target:**

```
d54:    beqz    v0,de8 <LM85>
d60:    beqz    v0,de8 <LM85>   # same label
```

```c
if (id < 9 && ((mask >> id) & 1)) {
```

**`||` sends all but the last branch to the body instead:**

```
2104:   beqz    v0,2134 <LM380> # into the body, line 554
212c:   beqz    v0,2284 <LM400> # past the body
```

```c
if (!(D_80071E24 & 4) || (D_80114498[g_CurrentEntity] != 0)) {
```

**Tell:** count the targets. Every conditional branch aiming at one shared
skip label is `&&`. Branches aiming at the body, with only the last one
aiming past it, is `||`.

**A ternary is a branch**, since the r3000 has no conditional move. The
shape is compact and distinctive: the else value is loaded in the delay
slot so it happens unconditionally, the fall-through overwrites it, and
there is a **single store** at the join:

```
3240:   li      v1,0x14
3244:   bne     v0,v1,3250
3248:   li      v0,0x6          # else value, delay slot, always runs
324c:   li      v0,0x5          # then value, overwrites it
3250:   sb      v0,0(a0)        # one store
```

```c
entry->priority = (cmdIndex == CMD_LIMIT) ? 5 : 6;
```

A register written twice in a row with only a branch between the writes is
the fingerprint.

Sources: `src/magic/brizad.c`, `BrizadRenderIce`; `src/battle/batres.c`,
`GiveSharedExp`; `src/field/field_opcodes.c`, `DebugPrintOpcode`;
`src/battle/battle.c`, `func_800A4350`.

### The delay slot around branches, which misleads three different ways

The orientation section says the instruction after a branch always runs.
Here is what that does to real code.

**It can hold work from before the branch**, so it belongs to neither path
in particular:

```
2104:   beqz    v0,2134 <LM380>
2108:   sw      s0,16(sp)       # a prologue register save
```

Read top to bottom, that looks like a callee-saved register being spilled
only on one path. It is unconditional. gcc just needed the slot.

**It can hold work from the not-taken path.** Here the shift belongs to the
second operand of an `&&`, and it runs even when the first test fails:

```
d54:    beqz    v0,de8 <LM85>
d58:    srav    v0,s5,v1        # mask >> id, from the second operand
```

That does not violate short-circuit evaluation, because a register shift
has no observable effect. It also means gcc could **not** have filled the
slot this way if the second operand dereferenced a pointer the first
operand was guarding.

**It can hold an entire block after an unconditional jump:**

```
 8c:    j       a4 <LM15>
 90:    move    s1,zero         # this is the whole then-block
```

So: the instruction's source line, not its position on the page, tells you
which C statement it came from. And a `nop` in a delay slot is information
too. It means gcc found nothing safe to move there.

### Loops -- the test is at the bottom, and the loop keyword is gone

```
 f0:    move    v1,zero         # i = 0, falls straight into the body
 f4:    lui     at,0x0
 fc:    sb      a0,0(at)
100:    addiu   v1,v1,1
104:    slti    v0,v1,8
108:    bnez    v0,f4 <LM25>
10c:    sra     a0,a0,0x1       # body work, scavenged into the delay slot
```

```c
for (i = 0; i < 8; i++) {
    g_KernRndTablePos[i] = seed;
    seed >>= 1;
}
```

**Tell:** there is **no test at the top**. gcc rotates loops so the only
copy of the condition sits at the bottom as a backward branch. The delay
slot of that branch is filled from the body, not necessarily with the
increment.

When the trip count is not provably non-zero, a guard appears at the top to
skip the whole thing. A constant bound needs none; a variable bound does:

```
198:    beqz    s2,1b4 <LM63>   # guard: bound could be 0
1a0:    sw      zero,0(s1)
1a4:    addiu   v1,v1,1
1a8:    slt     v0,v1,s2
1ac:    bnez    v0,1a0 <LM61>
1b0:    addiu   s1,s1,4         # pointer bump in the delay slot
```

Note `slt` against a register where the constant-bound loop used `slti`.
Note too that the C indexes an array here, yet there is no multiply in the
body: gcc kept a running pointer and bumped it by the element size. **The
bump is the `sizeof`.**

#### You cannot recover `for` versus `while` versus `do`/`while`

This is the part worth internalising. A variable-bound `for` and a
hand-guarded `do`/`while` compile to the *same* skeleton, a guard branch
over a bottom-tested body. Both of these produced it:

```c
for (i = 0; i < g_FieldLineCount; i++) {
```
```c
if (total_pixels > 0) {
    ...
    do { ... } while (i < limit);
}
```

What you *can* read off reliably:

| shape | meaning |
|---|---|
| body, bottom test, no guard | runs at least once, or a `do`/`while` |
| guard over the body, bottom test | may run zero times, cheap test |
| entry `j` to the bottom test | may run zero times, costly test |
| test at the **top**, `j` back at the bottom | not rotated at all |

That last row is the exception worth knowing: when the condition contains a
**call**, gcc cannot duplicate it, so the test stays at the top and the
loop ends in an unconditional backward jump.

```
 f4:    jal     0 <...>
 fc:    beqz    v0,11c <LM25>   # test at the top
114:    j       f4 <LM21>       # unconditional jump back
```

```c
while (SystemCdromReadChain()) {
```

#### `break` and `continue`

`break` jumps **past** the bottom test, to the first statement after the
loop. `continue` jumps to the **increment**, not to the top:

```
aa8:    j       fc4 <LM334>     # continue
aac:    addiu   s0,s0,1         # txt++ from the line above, in the delay slot
fc4:    lhu     t0,32(sp)       # the for-header increment block
```

#### One compiler-specific warning

gcc 2.6.3 sometimes **reverses** a counted loop, walking the array
backwards with `bgez` even though the C counts up from zero. A descending
loop in the target does not mean the original counted down. I have seen
this once, under 2.6.3 only, so treat it as a possibility to check rather
than a rule:

```c
for (i = 0; i < 256; i++) {
    g_FieldMapVars[i] = 0;
}
```
```
430:    addiu   a0,a0,255
438:    sb      zero,0(a0)
43c:    addiu   a3,a3,-1
440:    bgez    a3,438 <LM110>
444:    addiu   a0,a0,-1
```

Sources: `src/main/btlrand.c`, `SysInitRndTablePos`; `src/main/15B44.c`,
`SysGzipBinDecompress` and `SysGzipPackDecompressById`; `src/brom/brom.c`,
`func_800A0534`; `src/main/144D8.c`, `func_800145BC`; `src/main/1CDA4.c`;
`src/field/field_event.c`, `FieldEventInit`.

### `sltiu` + `sll ...,0x2` + `jr` -- a `switch` jump table

```
1b0:    lw      v0,8(gp)
1bc:    sw      v1,8(gp)
1c0:    lbu     v1,0(v0)
1c8:    sltiu   v0,v1,19
1cc:    beqz    v0,33c <LM100>
1d0:    sll     v0,v1,0x2
1d4:    lui     at,0x0
1dc:    addu    at,at,v0
1e0:    lw      v0,0(at)
1e8:    jr      v0
```

```c
switch (*s_Tutorial++) {
case 0:
    ...
case 2:
```

**Tell:** `sltiu` against a bound guarding a `beqz` to the default label,
`sll ...,0x2` scaling the selector to a word index, a `lw` from a relocated
`.rdata` base, then `jr`. The bound is the entry count, here 19, with holes
filled by default entries. It equals `max_case + 1` only when the switch is
not biased; a switch whose cases start higher gets an `addiu` subtracting
the bias first, and still gets a table. The next two entries cover the
other forms and how to read the case values back out.

Source: `src/main/input.c`, `TutorialDoNextEvent`.

### When a `switch` is not a jump table

The table is only one of three forms. Small switches become compares, and
the driver is the **span of the case values**, not how many there are.

**Two or so sparse cases give a linear chain** of equality tests, with no
bound check, no table load and no `jr`:

```
200:    beq     v0,s2,230 <LM58>    # == 2
208:    bne     v0,s1,250 <LM61>    # != 4, so default
```

```c
switch (D_8009C560) {
case 4:
...
case 2:
```

Note the case constants live in callee-saved registers, hoisted out of the
enclosing loop, so the `li` that created them can be far from the compare.

**Three or four cases give a binary search**, which is the form that
surprises people:

```
5dc:    beq     a0,v0,64c <LM158>   # case 1
5e0:    slti    v0,a0,2             # PIVOT, not a bound check
5e4:    beqz    v0,5fc              # upper half
5ec:    beqz    a0,618 <LM155>      # case 0
5fc:    li      v0,0x2
600:    beq     a0,v0,684 <LM162>   # case 2
604:    li      v0,0x3
608:    beq     a0,v0,6bc <LM166>   # case 3
```

**Tell:** a `slti` whose **both** successors are more compares on the same
register is a bisection pivot. A bound check is followed by a table load
and a `jr`. That is the whole difference, and it is easy to misread.

Measured across this tree, the table appears once the case values span five
or more:

| span | form |
|---|---|
| 2 values, sparse | linear compare chain |
| 3 | binary search |
| 4 | binary search |
| 5 and up | jump table |

**A ladder and a chain-compiled switch are the same thing.** An
`if`/`else if` ladder testing equality on one variable produces output
byte-for-byte like the two-case switch above, so **the C construct is not
recoverable**. Two weak hints, neither decisive: relational tests against a
different constant each time can only come from a ladder, and a bisection
pivot's two arms are both tests whereas a ladder's true arm is a body.
When matching, pick whichever reads better.

A long ladder also tail-merges. Six arms here share one call, and the only
per-arm work is loading a different argument:

```
1bc8:   sltu    v0,v0,v1        # operands swapped: const < x means x > const
1bcc:   beqz    v0,1be4         # false, next rung
1bdc:   j       1c9c            # taken, to the ONE shared tail
1be0:   li      a2,0x5          # the only thing that differs
```

Sources: `src/main/144D8.c`, `func_800146A4`; `src/menu/bginmenu.c`,
`func_801D05C4`; `src/field/field_opcodes.c`; `src/main/unzip.c`.

### Reading the case values back out of a jump table

The table is not opaque, even in an unlinked object, and this is the
fastest way to recover a `switch` you are about to write.

The relocations are section-relative, not symbol-relative, so the word
stored in the table **is** the target's byte offset into `.text`:

```shell
mipsel-linux-gnu-objdump -r -j .rodata build/us/src/field/field2.c.o
mipsel-linux-gnu-objdump -s -j .rodata build/us/src/field/field2.c.o
```

```
RELOCATION RECORDS FOR [.rodata]:
OFFSET   TYPE              VALUE
00000010 R_MIPS_32         .text
...
 0010 4c030000 54030000 7c030000 54030000
 0020 7c030000 54030000 7c030000 54030000
 0030 7c030000 7c030000 54030000 7c030000
 0040 54030000
```

Read little-endian, that is thirteen entries: `0x34C`, then `0x354` and
`0x37C` alternating. Three distinct targets. Cross-referencing the
disassembly, `0x34C` is one body, `0x354` is another, and `0x37C` is the
default, which is what every hole in the case set points at.

**The recipe.** The case value is the table index plus whatever bias the
code applied before indexing. Here the dispatch subtracts one first, so
index 0 is case 1. Indices 1, 3, 5, 7, 10 and 12 all reach `0x354`, giving
case values 2, 4, 6, 8, 11 and 13 for that one body:

```
318:    addiu   v0,v0,-1        # bias: case values start at 1
324:    sltiu   v0,v1,13        # 13 entries
32c:    sll     v0,v1,0x2
334:    addiu   at,at,16        # table at .rodata + 0x10
33c:    lw      v0,0(at)
344:    jr      v0
```

Those six values matched the six enum labels grouped in the source exactly.
**Tell:** the bound in the `sltiu` is the entry count, and the `addiu`
before it is the bias. Both are needed to turn an index back into a case.

Source: `src/field/field2.c`, `FieldWindowSetStateToClose`.

## Arithmetic and types

### `slt` vs `sltu` -- signed or unsigned, with one trap

```
db0:    slt     v0,s1,v1        # s32 hp < u16 hp_base -- both promote to int
12a0:   sltu    v0,v0,s2        # u32 exp < u32 next
132c:   slti    v0,a1,61        # s32 level < 61
```

```c
if (hp < c->hp_base) {
if (p->exp < next) {
if (level < 61) {
```

**The trap:** `sltu` proves the operands are unsigned **only when they are
32 bits wide**. On a narrow field gcc emits the unsigned form anyway,
because it knows a `u8` cannot be negative, even though C promotes it to
signed `int`:

```
1170:   sltiu   v0,v0,99        # u8 level >= 99
d50:    sltiu   v0,v1,9         # u8 id < 9
```

Infer nothing about a field's signedness from `sltu` on a byte or a
halfword. This is the one place where the obvious rule is wrong.

Source: `src/battle/batres.c`.

### `andi ...,0xff` and `sll 0x18` / `sra 0x18` -- narrowing and widening

```
c84:    lw      a2,64(a0)
c8c:    andi    v1,a2,0xff      # id = m, narrowed to u8
c90:    beq     v1,t0,cc0 <LM62>
```

```c
m = c->materia_weapon[i];
id = m;
```

**Tell:** a bare `andi rX,rY,0xff` with no load nearby is a value already
in a register being narrowed, so it comes from an assignment or a cast, not
from memory. `0xffff` is the `u16` form.

Widening the other way, from 8 bits to 32, is a shift pair:

```
1fc4:   lbu     a1,0(a0)
1fd0:   sll     a1,a1,0x18      # 0x18 = 24 = 32 - 8
1fd4:   sra     a1,a1,0x18      # arithmetic shift back, so sign-extended
2000:   sh      a1,0(v0)
```

```c
ptr->unk0 = arg0->unk0;         /* s8 into s16 */
```

`sll 0x10` / `sra 0x10` is the same thing from 16 bits. Recognise the pair;
do not read it as the normal way an `s8` loads, since gcc emits a plain
`lb` for that elsewhere. This particular shape came out as `lbu` plus the
pair, which is a quirk of the surrounding code, not a rule.

Sources: `src/battle/batres.c`, `GiveMateriaAp`; `src/battle/battle.c`,
`func_800A311C`.

### Casts, most of which emit nothing

This is the entry where absence is the lesson. A cast usually costs no
instruction at all, so **you cannot recover it by reading the assembly** --
yet the C may still need it, because it decides which instruction gets
selected somewhere else in the expression.

Free, in every case below:

- signed to unsigned at the same width, and back
- any pointer to any other pointer, including through an integer
- widening a `u8` or `u16` that its `lbu`/`lhu` already zero-extended

Here a whole line of C vanishes. The cast on line 229 emits nothing; its
only trace is that the loop below loads with `lbu` and strides by 4 by
hand, instead of loading a word:

```
58c:    move    v1,zero
598:    lbu     v0,0(a1)
5ac:    slti    v0,v1,200
5b4:    addiu   a1,a1,4
```

```c
extern s32 D_8009CE60[];
...
inventory = (u8*)D_8009CE60;
for (j = 0; j < 200; j++) {
    if (inventory[j * 4] == materiaId) {
```

And here a `u8` global reaches a `u32` local with no extension step,
because the load already did it:

```
7184:   lbu     a1,0(a1)
```

```c
extern u8 g_CurrentEntity;
u32 entity;
...
entity = g_CurrentEntity;
```

**Tell:** derive casts from the declared types of the operands and from the
instruction that was *selected*, never from an instruction that was
*added*. Absence of an instruction is not evidence of absence of a cast.

### Casts you can see, because they change the instruction chosen

These are the recoverable ones. None of them adds an instruction; each
swaps one for another.

| cast | effect |
|---|---|
| `(u32)` on a 32-bit value | `slt` becomes `sltu`, `slti` becomes `sltiu` |
| `(u32)` before a `>>` | `sra` becomes `srl` |
| `(s16)` on a `u16` field | `lhu` becomes `lh` |
| pointer cast before a deref | picks the load and store width |

```
284:    sltu    v0,v0,v1
```
```c
s32 i;
...
if (((u32)i) < ((u32)j)) {
```

Two signed locals, one unsigned compare. Without the casts you get `slt`.
This sharpens the earlier compare entry: when you do see the unsigned form
on 32-bit operands, the unsignedness may come from a cast that is
otherwise invisible.

The same cast steers a shift:

```
 f8:    srl     a3,a3,0x1
 fc:    sll     a3,a3,0x4
```
```c
s32 iy = (((u32)icon >> 1) << 4) + 0x70;
```

**Tell:** the shift mnemonic follows the signedness of the **left** operand
after promotion. A logical shift on a value you believe is signed means
there is a cast you have not written down. The arithmetic form is the
default for anything that promotes to `int`, a `u8` or `u16` field
included.

**Narrowing on a store is free.** The earlier entry showed a narrowing that
materialises as a mask; that happens only when the value must stay live in
a register. Sent straight to memory, the store swallows it, and a cast and
no cast produce the identical instruction:

```
3af8:   sb      s0,0(at)
```
```c
Savemap.memory_bank_1[indx] = (u8)value;
```

Sources: `src/menu/itemmenu.c`, `Quicksort` and `func_801D0228`;
`src/menu/bginmenu.c`; `src/field/field_opcodes.c`.

### Short arguments and short returns get extended twice

```
100:    sll     a0,a0,0x10
104:    sll     a1,a1,0x10
108:    sra     a0,a0,0x10
10c:    sra     a1,a1,0x10
11c:    jal     0 <...>
```

```c
void SysMenuDrawTexturedRect(s16, s16, s32, s32, s32, s32, s32, s32);
...
void func_801D0228(s16 arg0, s16 arg1, s32 arg2) {
...
SysMenuDrawTexturedRect(arg0, arg1, ix, iy, 0x10, 0x10, 1, 0);
```

**Tell:** the **caller** extends short arguments, right before the call.
The callee does not extend them in its prologue. Note that this function is
passing on its *own* incoming parameters and still extends them, so a
short value threaded through three calls is extended at every hop.

Returns work the other way and then both ways. The **callee** extends
before returning, classically with the arithmetic shift in the `jr ra`
delay slot:

```
ca20:   sll     v0,v0,0x10
ca24:   jr      ra
ca28:   sra     v0,v0,0x10
```
```c
static s16 func_800BFA98(u8* arg0, s32 arg1) {
```

and then the caller extends the returned value again anyway:

```
9d50:   jal     c9b4 <func_800BFA98>
9d58:   sll     a0,v0,0x10
9d5c:   sra     a0,a0,0x10
```

This is worth real money when typing a function you have not decompiled:
**a shift pair on `v0` right after a `jal` tells you the callee returns
`s8` or `s16`.**

Sources: `src/menu/itemmenu.c`; `src/battle/battle1.c`, `func_800BFA98`.

### There is no floating point in this tree

Worth one entry purely so you stop looking for it. The main CPU has no
floating point unit, and nothing here emulates one.

Checked two ways. No `float` or `double` appears as a declaration,
parameter, return type or literal anywhere in the sources or headers; the
handful of textual matches are all comments about double buffering. And
across every built object there is not one reference to a soft-float
helper.

**So if an expression looks like it needs a fraction, you are reading
fixed point.** The scale is 4096, and the entries on the arithmetic shift
and the biased divide tell you which operation you are looking at.

### Multiplying by a constant, and why a `mult` rules one out

The array entry showed shift/add chains standing in for an element size.
The same machinery handles ordinary multiplication by a literal, and it
gets long. Here are eight instructions computing one multiply, sitting two
instructions away from a real hardware multiply:

```
 30:    sll     v0,v1,0x3       # 8x
 34:    subu    v0,v0,v1        # 7x
 38:    sll     v0,v0,0x3       # 56x
 3c:    subu    v0,v0,v1        # 55x
 40:    sll     v0,v0,0x2       # 220x
 44:    subu    v0,v0,v1        # 219x
 48:    sll     v0,v0,0x2       # 876x
 4c:    addu    v0,v0,v1        # 877x
 50:    mult    a0,v0
 54:    mflo    v0
 58:    sra     v1,v0,0xc
```

```c
#define GROWTH_PER_FRAME (3 * 4096 / (BRIZAD_LIFETIME - 1)) // 0x36D
...
growth = (effect->Scale * GROWTH_PER_FRAME);
scale = (effect->AnimationFrame * growth) >> 12;
```

**Tell:** work the chain forward from 1, one instruction at a time. Each
`sll` multiplies, each `addu` of the original adds one, each `subu`
subtracts one. That runs 8, 7, 56, 55, 220, 219, 876, 877, and `0x36D` is
877. **The chain hands you the constant**, which is usually a tuning value
you could not otherwise guess.

The two lines above are the whole lesson side by side. The literal multiply
became eight instructions; the very next line multiplies two variables and
becomes `mult` plus `mflo`. **The distinguisher is whether one operand is a
literal, not whether the constant is awkward.**

**So a `mult` is strong negative evidence.** Every literal-constant multiply
in compiled code here is strength-reduced, with no observed cost cutoff; the
longest runs eight instructions. If you wrote `x * K` for a literal `K` and
the target has a `mult`, your C is wrong. The value came from a variable, or
from a runtime choice.

That last case is its own trap: gcc will not strength-reduce a multiplier
picked at runtime, **even when every possible value is a power of two**. A
conditional selecting 512 or 128 into a register, then one shared `mult`,
stays a `mult`.

Watch the delay slot while reading a chain. In the sibling multiply in this
same function, the leading `sll` is scheduled into the delay slot of the
branch that selects the arm, so the chain visually begins one line above
the source line it belongs to.

Sources: `src/magic/brizad.c`, `BrizadRenderIce`; `src/battle/battle.c`.

### `bgez` skipping an `addiu ...,4095` -- `/ 4096`, not `>> 12`

```
33bc:   bgez    a0,33c8 <LM678>
33c0:   sw      v0,112(s0)
33c4:   addiu   a0,a0,4095
33c8:   lw      a1,108(s0)
33cc:   sra     v0,a0,0xc
```

```c
spr->unk5C = spr->unk68 / 4096;
```

Against the bare shift, same shift amount:

```
4c:     lh      v1,20(s1)
5c:     mult    v1,v0
60:     mflo    v0
64:     sra     v0,v0,0xc
```

```c
growth = (effect->Scale * g_RefrecBaseScale) >> 12;
```

**Tell:** a lone `sra ...,0xc` is `>> 12`. A `bgez` hopping over
`addiu ...,4095` before the `sra` is `/ 4096` -- C division truncates
toward zero, so a negative value needs `divisor - 1` added first. The two
agree only for non-negative operands, so picking the wrong one is an
instant mismatch even though the arithmetic "looks" the same.

Sources: `src/ending/ending.c`, `func_800A3368`; `src/magic/refrec.c`,
`RefrecRenderModel`.

### `mult` with a magic constant, then `mfhi` -- divide by a constant

```
1334:   lui     v0,0x6666
1338:   ori     v0,v0,0x6667
133c:   addiu   v1,a1,-1
1340:   mult    v1,v0
1344:   sra     v1,v1,0x1f
1348:   mfhi    t2
134c:   sra     v0,t2,0x2
1354:   subu    v0,v0,v1
```

```c
r->tier = (level - 1) / 10;
```

**Tell:** a `lui`/`ori` pair building a reciprocal (`0x66666667` is
2^34/10) feeding `mult`, then `mfhi`, a small `sra`, and a `subu` of the
dividend's sign bit extracted by `sra ...,0x1f`. The shift after `mfhi`
encodes the divisor: `0x2` here is `/ 10`, `0x3` with the same constant is
`/ 20`. No `div` instruction appears. `0x51eb851f` with `sra ...,0x5` is
`/ 100`.

Source: `src/battle/batres.c`, `CalcTotalExp`.

### `div` followed by `break 0x7` -- the divisor is a variable

```
830:    lui     v0,0x1
834:    div     zero,v0,v1
838:    bnez    v1,844 <BattleInitSetSpeed+0x38>
83c:    nop
840:    break   0x7
844:    li      at,-1
848:    bne     v1,at,85c <BattleInitSetSpeed+0x50>
84c:    lui     at,0x8000
850:    bne     v0,at,85c <BattleInitSetSpeed+0x50>
858:    break   0x6
```

```c
D_800F5F44.battleSpeed = 0x10000 / ((speed * 480 / 256 + 0x78) * 2);
```

**Tell:** a real `div` means the divisor was **not** a compile-time
constant, and it always drags along both traps -- `break 0x7` for divide by
zero and the `-1`/`0x80000000` pair guarding `break 0x6` for signed
overflow. If the target has `break 0x7`, no constant divisor will ever
match it.

Source: `src/battle/batini.c`, `BattleInitSetSpeed`.

## Candidates not yet written

Bitfield extraction with a non-zero shift, which does not occur here;
`mflo` ordering as a tell for multiply operand order; `memcpy` versus an
inlined copy loop; the `break 0x6` overflow check as distinct from
`break 0x7`; whether reordering declarations really does move a stack
slot, which is untested.

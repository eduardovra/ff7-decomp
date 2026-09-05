# Magic overlays

How the spell overlays under `src/magic/` are put together, drawn from
`barrier.c`, `lv5deth.c` and `brizad.c`.

For a ground-up walkthrough of one overlay, down to the GTE instructions and
the GPU packets, see `how-a-spell-is-drawn.md`. This doc is the reference;
that one is the explanation.

Each spell is its own overlay, loaded into RAM at `0x801B0000` while the
`battle` overlay stays resident. The overlay's only job is to register
callbacks with the battle engine and render; all scheduling belongs to
`battle`.

## The effect slot system

Everything an overlay does happens through a fixed pool of effect slots
owned by the battle overlay:

```c
extern Unk80162978 D_80162978[100];   // battle_private.h
```

100 entries, `0x20` bytes each. One entry is one running effect instance.

- `BattleEffectRegister(cb)` takes a free entry, attaches `cb` to it and
  returns the **index**.
- Once per frame the engine walks the active entries. For each it sets
  `D_8015169C` to that entry's index, then calls the entry's callback.
- A callback finds its own state with `&D_80162978[D_8015169C]`. There is
  no `this` parameter -- the index arrives through a global.
- Writing `StartFrame = -1` retires the slot.

A slot gets exactly one callback, so anything needing a second independent
lifetime needs a second slot. This is why effects spawn effects.

One cast of Ice, as an example:

```
MAGIC_Brizad
  |- register BrizadDoubleBufferFlip        -> slot A (lives all battle)
  `- MagicAnimationRegister(mask, 4, BrizadAttachToTarget)
       `- engine calls BrizadAttachToTarget(target) per target
            `- register BrizadSpawnIce      -> slot B
                 B.TargetIndex = target
                 `- next frame, BrizadSpawnIce runs and registers
                    BrizadRenderIce         -> slot C, then retires B
                      `- C renders 15 frames, then retires itself
```

Note: the dispatch loop itself is still in assembly. The contract above is
inferred from every call site, not read from the scheduler. It is
unambiguous -- barrier's frame-by-frame timeline only works if callbacks run
exactly once per frame -- but the loop's own details are unverified.

## Entry and dispatch

`func_800D0C80` in `battle2.c` turns a magic id into an overlay call. Most
ids go through a function pointer table:

```c
extern void (*D_800EFAF0[])(s16, u8);   // battle_private.h
```

The table holds raw `0x801B____` addresses, since every overlay loads at the
same base. Ice is index 30, holding `0x801B037C`. A handful of ids are
special-cased directly in `func_800D0C80` instead.

Those addresses are pinned for the linker in
`config/sym_battle_import.us.txt`, which is why an overlay entry point links
even when nothing in the overlay's own C file is global.

**The entry point's position in its `.c` file is dictated by its ROM
address.** Function order in the source is the address layout. When the
entry sits at a low address the original author wrote a thin trampoline
there and put the real body at the end of the file:

```c
static void BarrierMainSetup(int arg0, int arg1);
void MAGIC_Barrier(int arg0, int arg1) { BarrierMainSetup(arg0, arg1); }
```

`MAGIC_Brizad` has no trampoline -- it is the last function in the overlay,
so it must be last in the file.

## The five roles

| role | barrier | lv5deth | brizad |
|---|---|---|---|
| entry | `MAGIC_Barrier` -> `MainSetup` | `MAGIC_Lv5Death` -> `MainSetup` | `MAGIC_Brizad` |
| buffer flip | `BarrierDoubleBufferFlip` | `Lv5DeathBufferFlip` | `BrizadDoubleBufferFlip` |
| per-target | `BarrierAttachToTarget` | `Lv5DeathAttachToTarget` | `BrizadAttachToTarget` |
| timeline | `BarrierAnimationUpdate` | -- | `BrizadSpawnIce` |
| render | Border, Shield | Ring, Sprite, ScreenFade | `BrizadRenderIce` |

Setup is always the same shape: register the flip slot, call
`MagicAnimationRegister(mask, arg1, frameStep, attachCb)`, fire the SFX.

The flip function is effectively copy-pasted across all three: same `<< 16`
page stride, same `^= 1`, same `D_80162080 < 2` retire.

Timeline complexity is where spells differ. `BarrierAnimationUpdate` is a
literal frame script -- frames 0/2/4/6 spawn border faces, 1/3/5/7 spawn
shield faces, 17 fires the result popup. `BrizadSpawnIce` is the degenerate
case: one frame, spawn one thing, retire.

## The slot struct is per-overlay

Only the first four bytes are shared:

```c
/* 0x00 */ s16 StartFrame;
/* 0x02 */ s16 AnimationFrame;
```

Everything after that is each overlay's private business. `Pos` is at `0x04`
in lv5deth but `0x10` in brizad. Offset `0x18` is `FaceIndex` in barrier and
`Scale` in brizad.

**Never port a slot struct between overlays.** Derive it from the loads and
stores in the target assembly every time.

Fields are also reused when a slot does not animate. The flip slot uses
`AnimationFrame` as a 0/1 page index in all three overlays. lv5deth's
screen-fade slot uses offset `0x0E` as a fade-start timestamp, unioned with
the `TargetIndex` meaning it has in the other slots.

## Shared engine calls

| call | purpose |
|---|---|
| `BattleEffectRegister(cb)` | allocate a slot, returns its index |
| `MagicAnimationRegister(mask, arg1, step, cb)` | fan out over targets, `cb(target)` every `step` frames |
| `BattleGetPartPosition(target, part, out)` | where on the model to place the effect |
| `func_800D29D4(desc, ot, otLen, buf)` | render a model, threads the primitive write pointer |
| `func_800D5774(target)` | fire the damage / result popup |
| `BattleCommandSend(32, pan, sfxId)` | play a sound effect |
| `func_800D55A4(target)` | scale derived from the target's model size |

Two ways to compute stereo pan, and it tracks where the SFX fires:
`BattleEntityGetStereoPan(entity)` when the sound plays once at setup
(barrier, brizad), `BattlePositionToStereoPan(&pos)` when it plays per
target (lv5deth).

Globals worth knowing: `D_80062D98` is a global pause flag -- every
animating slot gates its frame advance on it. `D_80162080` counts down as
the battle ends; the flip slot retires when it drops below 2.

## Conventions

**Fixed point is 4096.** `>> 12` follows every scale multiply. Angles use
the same unit, 4096 to a full turn. brizad's constants are both derived from
its 15-frame lifetime: `0x36D` is `3 * 4096 / 14` (grow to 3x) and `0x124`
is `4096 / 14`, which ramps the depth-cue factor to a full fade. It was
called `SPIN_PER_FRAME` until the evidence came in; see
`how-a-spell-is-drawn.md` section 13.

**Double buffering** is always two `0x10000` pages, with the buffer and its
write pointer adjacent in `.bss`, pointer immediately after the buffer.

**Render descriptors** are `Unk801B0C98`: model pointer at `0x0`, flags at
`0x4`. Offset `0xA` reaches the GTE as `IR0`, the depth-cue factor blended
toward `SetFarColor`, so it is a fade level and not a rotation as this doc
previously claimed; `func_800D4D90` instead reads it as a CLUT addend.
`how-a-spell-is-drawn.md` section 10 has the evidence. barrier and lv5deth
also build one in scratchpad RAM at `0x1F800000`; brizad uses stack locals.
Both appear in matching code, so it is an authoring choice rather than a rule.

## Verified against a running game

`magic-probe.md` describes the setup. A Lua breakpoint on an overlay's
render function records one entry per frame from inside the emulator, which
is how these were checked rather than argued.

**brizad, 15 frames.** `Rot` stayed `(0,0,0)` on every frame, and the
descriptor's depth cue stepped by exactly `0x124` fourteen times --
`FADE_PER_FRAME`, `FIXED_ONE / (BRIZAD_LIFETIME - 1)`. The values lag one
frame behind the source because the breakpoint sits at the function's entry
and samples before that frame's write, which is itself a check that the
capture is aligned.

**thunder, 17 frames.** Depth cue held `0x80` and then stepped down by
`0x10` to `0x10` -- the same field as brizad's, over a different range, on
a different scale. That is what justifies one name for it across the
struct. The 17th entry takes the `frame >= 16` branch, sets `StartFrame` to
`-1` and returns without drawing.

**The depth cue is visible, not just plumbed.** Capturing the framebuffer
alongside the memory on a brizad cast: the model grows every frame, yet
bright pixels peak at 1809 on frame 9 and fall to 288 by frame 14. A larger
object with six times fewer bright pixels means per-pixel luminance
dropped, which is the cue blending toward a black `SetFarColor`. Tracing
the field to `IR0` shows where it goes; only the picture shows it works.

**Neither spell rotates, and both were once said to.** brizad's `spin`
naming was caught by reading the code (`how-a-spell-is-drawn.md` section
13). thunder's comment claimed the model "spins up over the first 8
frames"; the trace shows `ThunderModelMatrix` holding `m[0][0]` and
`m[2][1]` at `Scale`, `m[1][2]` at `-Scale`, and the other six entries at
zero on all 17 frames, while `Scale` grows a flat `0x200` per frame from
`0x1000` to `0x3000`. A rotation would drive those six sinusoidally. It is
a fixed orientation being scaled, and it reads as a rotation in source
because a scaled basis and a rotation matrix look alike.

That is the same wrong guess made twice, independently, in two overlays.
When a magic overlay appears to spin, check whether the off-diagonal terms
ever move before naming anything -- and check it in memory. brizad's ice
block is an eight-pointed symmetric star, so a 45-degree rotation looks
exactly like none. No screenshot could have settled it.

## Where overlay data lives

barrier brought its whole data segment into C as initialised arrays.
lv5deth and brizad left theirs in assembly and reach it with `extern`.

The deciding factor is not size -- lv5deth leaves a 12-byte descriptor in
assembly while barrier brings in 3.2 KB. Addresses must stay in ascending
order, so an object can only move into C if everything before it in the
segment moves too. lv5deth's descriptor sits behind a 64 KB texture and is
trapped there.

Suggested rule:

1. Always name and type the symbols, whichever side the bytes live on. This
   is what buys readability and it is cheap.
2. Migrate a segment into C only wholesale, and only when its largest member
   is structured data rather than an opaque blob.
3. Never migrate a segment partially.

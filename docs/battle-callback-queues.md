# Battle callback queues

Four per-frame callback queues live in the resident battle module. They
share one shape, and ten functions in `battle1.c` implement them: four
register, four dispatch, two reset.

## The shape

Each queue is four globals:

| queue | fn pointers | data slots | live count | iterator |
|---|---|---|---|---|
| 0x64 | `D_80161EF0` | `D_80162978` | `D_80162080` | `D_8015169C` |
| 0xA | `D_80163B48` | `D_801620AC` | `D_80163B7C` | `D_801590D0` |
| 0x3C | `D_80163B84` | `D_801621F0` | `D_80163C78` | `D_801590D4` |
| 0x10 | `D_800FA978` | `D_800F7ED8` | `D_800FA9BC` | `D_800F8360` |

Data slots are 0x20 bytes each, except the 0x10 queue's, which are 0x28.

The register function scans for a free function-pointer entry, stores the
callback there, writes the current iterator value into field 0 of the
matching data slot, bumps the count, and **returns the slot index** --
callers do their own `index * 0x20`. Three of the four also refuse a slot
below the current iterator, so a callback registered during dispatch does
not run in the same frame. If no slot is free the function does not
return: it runs `PadStop` / `ResetGraph` / `StopCallback` and then
`SystemError(0x61, n)`, with a different `n` per queue.

The dispatch function resets the iterator to 0, walks every entry calling
the non-null ones, and after each call re-reads the iterator (the callback
may have moved it). A callback that has written **-1 over field 0** of its
slot is retired: fields 0 and 2 are zeroed, the function pointer is
cleared, and the count is decremented. `MagicAnimationData.TargetCursor`
in `battle_private.h` is the same field, seen from one consumer.

## Order within a frame

From `func_800B8438`:

1. `BattleMovementUpdate` (0xA) -- early, inside the state switch, beside
   the model and animation updates.
2. `BattleCameraUpdate` (0x10) -- via `func_800BC81C`, then
   `func_800BC8B0` copies the result into the camera vectors.
3. `func_800BC538` (0x3C), then `func_800BC348` (0x64) -- back to back
   after `SetFarColor(0, 0, 0)`, immediately before the camera matrix
   build.

`BattleCallbacksReset` clears the 0x64, 0xA and 0x3C queues in that order
and tail-calls `func_800BC2F0`, which clears the 0x10 one.

## Naming, and why two are still unnamed

Akari's q-gears_reverse (`ffvii/address_battle.txt`) names all four after
the traffic it traced: damage (0x64), unit movement (0xA), effect (0x3C)
and camera (0x10). Its note on `funcbc1e0` -- "init damage, unit movement,
effect and camera callback arrays" -- lists them in exactly the order
`BattleCallbacksReset` clears them, which is what pins the mapping.

Movement and camera carry only their own traffic, so those names were
taken: `BattleMovementRegister` / `BattleMovementUpdate`,
`BattleCameraRegister` / `BattleCameraUpdate`.

The 0x64 and 0x3C queues are generic and their q-gears names were **not**
taken. Both run in the same draw phase, and what we register into the 0x64
one includes `BrizadRenderIce`, `ThunderRenderModel`, `BarrierRenderShield`
and `BrizadDoubleBufferFlip` -- a double-buffer flip is not damage. The
same document undercuts its own label: it lists **one** field layout for
the movement slots and **six** for the 0x64 slots, each headed by a
different owning callback (`800ce7e0`, `800ce970`, `800d5350`, `800d57c0`,
`fire 801b0000`). A block whose 0x20 bytes mean six different things is a
generic slot, not a damage record.

So `BattleEffectRegister` keeps its existing name and `func_800BC348`,
`func_800BC04C` and `func_800BC538` stay unnamed until we know why there
are two draw-phase queues. The q-gears wording is recorded in one-line
comments beside each so nobody re-derives it.

## Corrections to the source

- `D_800F7ED8` slots are 0x28 = 40 bytes. Reading that "0x28" as decimal
  28 has caused confusion; `Unk800F7ED8` in `battle_private.h` is right.
- The register functions return an index, not a pointer to the block.
  Akari's own pseudocode for `funcb88cc` agrees: `funcbbeac; S0 = V0;`
  then `[0x80162978 + S0 * 0x20 + ...]`.

## Field layouts

Do not flatten the per-callback layouts onto the shared struct -- the same
0x20 bytes mean different things per owner. They belong in per-consumer
structs, like the existing `MagicAnimationData`, derived from each
callback's own asm. `address_battle.txt` documents them under `80162978`,
`801621f0`, `801620ac` and `800f7ed8`, keyed by owning function.

All of the above beyond the asm-derived parts is community reverse
engineering; treat a name or offset from it as a hypothesis to check.

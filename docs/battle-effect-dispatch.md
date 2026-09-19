# Battle effect dispatch

How `battle` decides which `MAGIC/*.BIN` overlay to load and where to call
into it, drawn from `battle2.c` and confirmed against a running game.

`magic-overlays.md` covers what an overlay does once it is resident. This
doc covers how it got there.

## One loader, one address

Every battle effect goes through the same loader, to a hardcoded
destination:

```c
void BattleLoadOverlaySector(s32 loc, s32 len) {
    SystemLoadFileBySector(loc, len, (u_long*)0x801B0000, NULL);
    BattleCdromReadChain();
}
```

`MAGIC/` is the directory for *every* battle effect overlay, not just
spells. `BLAVER.BIN` (Braver), `MRAIN.BIN` (Meteorain) and `WPYU.BIN` sit
next to `FIRE.BIN` and `BARRIER.BIN`, 318 files sharing one flat id space.
They never collide because only one is ever resident.

That is why several different `MAGIC_*` entrypoints resolve to the same
address, and why the linker accepts it: the names are mutually exclusive in
time, not in space.

## The tables

`BattleDispatchModelLoadImage` switches on `g_BattleModels[i].currentActionId`
to pick an id table, indexes it by `attackEffectId`, and feeds the result
through `D_800EEBB8[id]` to the loader. A parallel entrypoint table gives
the call to make once the overlay is resident.

| action | id table | entrypoint table | slots |
| --- | --- | --- | --- |
| 2 magic | `D_800EF63C` | `D_800EFAF0` | 54 |
| 3 summon | `D_800EF6D8` | -- | 18 |
| 4 item | `D_800EF5B0` | `D_800EF9D8` | 70 |
| 7 coin | -- | -- | fixed `D_800EEBB8[221]` |
| 8 throw | `D_800EF8D8` | `D_800EFFE0` | 128 / 127 |
| 13 enemy skill | `D_800EF6A8` | `D_800EFBC8` | 24 |
| 20 limit | `D_800EF838` | `D_800EFEA0` | 80 |
| 32 enemy attack | `D_800EF6FC` | `D_800EFC28` | 158 |

Two asymmetries are real, not transcription slips:

- **Summons have no entrypoint table.** Action 3 loads an overlay and is
  dispatched some other way.
- **Action 8's tables are different lengths.** `0x800EF9D8 - 0x800EF8D8` is
  `0x100` = 128 `s16`; `0x800F01DC - 0x800EFFE0` is `0x1FC` = 127 pointers.
  Effect id 127 has an overlay id but indexing the entrypoint table with it
  would read into `D_800F01DC`.

A plain physical attack writes `currentActionId` but never reaches the
loader, which is why there is no table for it.

## Where the action ids come from

`currentActionId` is read 18 times in C and written nowhere; the writer is
`BattleQueue1InitPlayAnim` (`battle1.c`, still `INCLUDE_ASM`), which stores
the byte at `0x800B7910`. The values match q-gears' `800f692c` +03 "action
type" list, and every one was then confirmed in a running game:

| action | command | seen as |
| --- | --- | --- |
| 2 | magic | Fire -> `FIRE.BIN`, slot 27 |
| 3 | summon | Ifrit -> `IFLEET.BIN`, slot 2 |
| 4 | item | Potion -> `CAREL.BIN`, slot 0 |
| 7 | coin | Gil Toss -> `WPYU.BIN` |
| 8 | throw | Throw -> `WPSI.BIN`, slot 82 |
| 13 | enemy skill | Lv5 Death -> `LV5DETH.BIN`, slot 19 |
| 20 | limit | Big Shot -> `HVSHOT.BIN`, slot 7 |
| 32 | enemy attack | q-gears only; plain physicals never dispatch |

Action 7 is the odd one: the loader hardcodes `D_800EEBB8[221]`, because
Coin always plays the same overlay.

## Verifying the `.BIN` annotations

Each table slot is commented with the overlay its id resolves to. Those are
derived, not asserted: `D_800EEBB8[id].loc` is a disc sector, and the ISO
directory maps sectors to filenames.

```shell
isoinfo -i "disks/Final Fantasy VII (USA) (Disc 1).iso" -l
```

Joining the two confirmed all 531 annotated id-table rows with no
mismatches. `tools/action_probe.py decode` does the same join on a live
trace.

## Reproducing a trace

See `magic-probe.md` for the launch settings, which are not optional.

```shell
.venv/bin/python3 tools/redux_launch.py
.venv/bin/python3 tools/action_probe.py arm
# play a few turns
.venv/bin/python3 tools/action_probe.py drain \
  | .venv/bin/python3 tools/action_probe.py decode
```

The write watchpoint fires on any write to that RAM regardless of what the
memory currently means -- boot-time `SystemLzsDecompress` output lands
there long before it is the model table -- so it filters on a PC at or
above `0x800A0000`. The four Exec hooks only fire in genuine dispatch; when
the two disagree, trust those.

## Open

- **`D_800EFEA0[43..48]`.** Six limit slots whose id is `FIRE.BIN` but whose
  entry is `0x801B0000`, which is `FireRenderSprite`, a static render
  callback. The annotation is disc-verified, so the data really says that.
  They sit right after `DISE.BIN`, but Cait Sith's Slots was traced to the
  55-59 cluster (`TSOL.BIN` at 58), so they are *not* Slots outcomes.
- **`D_800EF9D8[20]`.** Id `FIRE.BIN`, entry `BattleQueueImpactEffect`,
  which lives in `battle`. So "load an overlay, dispatch to battle-resident
  code" happens at least once, which is why the six above cannot simply be
  called dead.
- **Action 8 effect id 127.** Whether it is reachable at all.
- **`BattleInitMagicCastEffect`.** Calls `func_801B0040` with no id table
  and no hardcoded load, so it uses whatever is resident.

A probe can confirm a slot *is* dispatched; it cannot prove one never is.
The honest result for the three above is "not observed", which earns a
comment, not a rename.

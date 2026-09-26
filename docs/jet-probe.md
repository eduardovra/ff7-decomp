# Probing the jet minigame at runtime

Naming the rest of `src/mini/jet/` has to be settled by evidence. The PC
reverse-engineering in `ff7-coaster` is a source of hypotheses, not of
names. This file records how to run jet in PCSX-Redux and what each run
established. Launching and the web API are covered in `magic-probe.md`.

## Where this stands (2026-09-25)

Every jet function is decompiled; this work names what is left from
runtime evidence. Named so far, each with its evidence below: `g_JetPaused`,
`g_JetExit`, `g_JetPadDir`, `g_JetAimMode`, `g_JetSpeed`, `g_JetObjects`,
`g_JetObjectCount`, `JetObjectDamage`, `JetObjectAwardPoints`, `JetPlaySfx`,
13 of the 27 object types in `enum JetObjectType` (`jet_object.c`), and
the generic `JetObject` and `JetObjectState` fields.

To resume:

1. `tools/redux_launch.py`, then `tools/jet_boot.py` and New Game.
2. Pause in the first seconds of the ride and save a state
   (`/api/v1/state/save?name=jet-start`). Redux keeps named states in its
   own config directory, so an old `jet-start` may already exist; the tour
   reads the segment it starts from, so any early state works.
3. `tools/jet_tour.py --state jet-start <types>` captures boxed frames of
   those types where they spawn, into `build/jet_tour/`. It needs no input.

Open, in rough order of payoff:

- Types 0, 2, 4, 14 (and 14's children 15, 16): seen or scheduled, not yet
  captured well. Re-run the tour with shorter `--offsets` (e.g. `5,15,30`).
- 13 shares a case with 7; 3 may be the starfield (not confirmed).
- 100, 201, 203, 230, 250: no spawns seen in a full ride (250 was confirmed
  by injecting one).
- `unk50` slots: only `[0]` points, `[0xD]` hit points and `[18]` death
  sound are established, and only for shootable types.

Two traps already hit:

- Do not "hide" an object by clearing `active` (was `unkDA`). The slot is never freed,
  `g_JetObjectCount` reaches the cap and spawning stops, which reads as
  "this type never appears".
- `g_JetTrackSegment` is the spawn timeline and advances with `g_JetSpeed`,
  so forcing the speed down freezes spawning too, and forcing it up
  fast-forwards.

## Probing the native build

`./build-pc/ff7_pc -jet` runs the ride on the host (see `psyz-port.md`),
so gdb reads jet's globals and structs by name, with types. A gdb Python
`Breakpoint` subclass on `JetObjectsUpdate` whose `stop()` returns `False`
is a per-frame hook. `SDL_VIDEO_DRIVER=offscreen` runs it without a window
and uncapped, about 200 frames a second under the hook. With no input the
ride is the same one PCSX-Redux plays.

`tools/jet_native_tour.py <types>` is the native counterpart of
`jet_tour.py`: one no-input ride under gdb, saving the displayed frame with
each target object's box whenever it reaches one of `--ages` frames old.
Frames come from PSY-Z's `Psyz_VideoAllocCapturedFrame`, called from gdb.
Only five box points count: `JetProject6Points` never writes `unk11C[5]`
(in the original assembly too), so it stays at the spawn template's value.

Resolve symbols inside `stop()`, not when the script loads: the binary is
PIE, so an address taken before `run` is unrelocated and faults.

Only jet's game logic is original there. Its GTE renderers, the CD chain
and the libgpu/libgte stubs are port code, so settle anything about drawing,
loading or sound in PCSX-Redux instead. Struct offsets differ on 64-bit;
use field names, never offsets.

## Booting straight into jet

Speed Square sits behind the Gold Saucer, and no save is needed to reach it.
`main`'s loop copies `Savemap.current_module` (`0x8009D278`) into
`g_GameState` inside `InitFieldFromSavemap` (`0x80011AEC`), then dispatches
`GAMESTATE_JET` (11) by loading `MINI/JET.BIN` and calling `MINI_Jet`.

```shell
.venv/bin/python3 tools/redux_launch.py
.venv/bin/python3 tools/jet_boot.py      # one-shot breakpoint
# in Redux: title screen -> New Game; the ride loads instead of Midgar
curl 'http://localhost:8080/api/v1/state/save?name=jet-ride'
```

The breakpoint writes 11 into `current_module` when that function is entered
and removes itself. It works from New Game on Disc 1 (verified 2026-09-24).
After that, load the `jet-ride` state rather than booting again:

```shell
curl 'http://localhost:8080/api/v1/state/load?name=jet-ride'
```

When the ride ends, `MINI_Jet`'s result goes to
`Savemap.memory_bank_1[354..355]` and the loop returns to field with
`EVTCMD_FIELD_MAP_CHANGE`.

## Findings

Each entry gives the identifier, the hypothesis, the observation that tested
it and the verdict. An identifier is renamed in `src/` only when the
observation confirms it.

### `g_JetPaused` (was `D_800D16DC`), confirmed 2026-09-24

- Hypothesis: a pause flag. `func_800A2E38` toggles it on the first frame
  pad bit `0x800` is held, and `MINI_Jet` skips its whole update and draw
  block while it is set.
- Observation: a Write watchpoint on it during play logged three writes,
  all from the toggle (`sb zero` at `0x800A3320`, `sb v0` at `0x800A3324`),
  matching pause, unpause, pause on START. Read back as 1 while the game
  showed paused.
- Verdict: confirmed.

### `g_JetExit` (was `D_800E2600`), confirmed 2026-09-24

- Hypothesis: ends the ride. Its only reader is the exit test of
  `MINI_Jet`'s loop.
- Observation: over one full ride played to its end with no input, a Write
  watchpoint logged a single jet write, `= 1` at `0x800A6904` in
  `func_800A46E8` object case 253, once its fade reached `0x80` frames. The
  loop then returned to main. A later write from `0x80015CE4` was main
  reusing the memory after jet unloaded.
- Verdict: confirmed as "leave the ride". Whether a win and a loss both reach
  case 253 is not yet tested.

### `g_JetPadDir` (was `D_800A8A6C`), confirmed 2026-09-24

- Hypothesis: the d-pad direction in numeric-keypad layout, as its old
  comment claimed.
- Observation: a per-frame recorder at `func_800A2E38` entry logged 8, 2, 4
  and 6 while Up, Down, Left and Right were held in turn, and 0 between.
  The cursor moved the same way. No other code reads it.
- Verdict: confirmed.

### `g_JetAimMode` (was `D_800D1960`), confirmed 2026-09-24

- Hypothesis: selects what the pad controls. At 1 the d-pad moves the cursor
  and `0x20` fires; at 0 the d-pad edits `D_800A89D0`/`D_800A89D4` (fog near
  and far), and face and shoulder buttons edit `D_800A83D8` and the speed.
- Observation: it read 1 on every frame of normal play. Forced to 0 with the
  d-pad held, fog far went from `0x37DC` to 15200 and `D_800A83D8` from 0 to
  (11300, 0, 4400), with no change on screen. `SetFogNearFar` runs once,
  before the ride loop, and nothing else reads `D_800A83D8`. So in the
  retail game mode 0 is unreachable (only the init writes it, with 1) and
  inert.
- Verdict: confirmed as the aim/other-controls switch. That mode 0 was a
  developer tuning mode is inference, so it is not in the name.

### `g_JetSpeed` (was `D_800A897C`), confirmed 2026-09-24

- Hypothesis: the ride's forward speed. It starts at `0x2710`, `MINI_Jet`
  passes it to the per-frame track and object updates, and the end-of-ride
  objects zero it or set it to `0x4000`.
- Observation: with `g_JetAimMode` forced to 0, holding the key mapped to pad
  bit `0x1` (which subtracts `0x400` per frame) visibly stopped the train,
  and the value read `0x64`. The same poke also took d-pad control of the
  cursor away, confirming `g_JetAimMode`'s gate on screen.
- Verdict: confirmed.

### Objects and hits, confirmed 2026-09-24

- `g_JetObjects` (was `D_800D1DC0`): the 100-slot `Unk800A4390` pool with a
  free list; `active != 0` (was `unkDA`) marks a live slot. A per-frame scan at
  `func_800A46E8` logged 570 spawn/free events over one ride.
- `g_JetObjectCount` (was `D_800EE42C`): read 27 with exactly 27 live slots.
- `JetObjectDamage` (was `func_800A6B08`): a hit logger showed each call
  subtract `g_JetShotPower >> 5` from `unk50[0xD]` -- 3 per shot at power
  ~124, 1 per shot at 8 once the bar drained -- and call the next function
  once it went negative.
- `JetObjectAwardPoints` (was `func_800A6BD8`): kills of objects with
  `unk50[0]` = 60, 40, 40, 40 moved `g_JetScore` 50 -> 110 -> 150 -> 190 ->
  230, and the player saw a "50" popup for a 50-point target. It does not free
  the object, and mode (`unk50[10]`) 3 only adds points and turns it, so it is
  not named "kill".
- Not renamed: `unk50[0]` and `unk50[0xD]` as fields. `unk50` is copied
  wholesale from the spawn table, and other types use `[0]` differently (type
  250 compares the score against it). `unk50[18]` goes to `func_800A29AC`,
  which the pause toggle also calls with `0x3B`, but that it plays a sound is
  not yet observed.

### `JetPlaySfx` (was `func_800A29AC`), confirmed 2026-09-24

- Hypothesis: plays sound effect `arg0`. It alternates `D_800A8928` between
  two AKAO voices, stopping each (`0xB0`/`0xB1`) before playing on it
  (`0x28`/`0x29`), the same commands main's unnamed `func_8001FA68` and
  `func_8001FAAC` send.
- Observation: the START toggle calls it with `0x3B`, and the player heard a
  chime on every pause and unpause. A breakpoint rewrote `a0` from `0x3B` to
  `0x2D`, the `unk50[18]` of type 1 model 32; after 4 swaps the player
  reported that pausing now made a target's death sound instead of the
  chime. Several target kinds were on screen, so that it was exactly `0x2D`'s
  sound is not established -- only that the argument selects the effect.
- Verdict: confirmed, including `unk50[18]` as the death sound id.

### Object and state fields, confirmed 2026-09-25 (native build)

Hypotheses from the handlers in `JetObjectsUpdate`; each checked every frame
of a 6000-frame no-input ride under gdb (296 objects of 12 types).

- `JetObject.active` (was `unkDA`): live objects counted every frame equalled
  `g_JetObjectCount` on all 6000 frames.
- `JetObject.index` (was `unkD8`): equal to the object's own slot in 67138
  live checks, `-1` in 532862 free ones.
- `JetObject.node` (was `unkD4`) and `JetObjectState.modelId` (was `unk8`):
  the node was non-null and `node->modelId == modelId` in all 67138 live
  checks.
- `JetObject.state` (was `unk28`): the `JetObjectState` itself.
- `needsInit` (was `unk10`): 1 on all 115 objects caught right after a
  scheduled spawn; every handler tests it, runs its setup and clears it.
  Type 3 has no setup and never clears it.
- `age` (was `unk14`): stepped by exactly 1 per frame for every type that
  touches it (0, 1, 9, 10, 17, 252, 255); types 4, 5, 8, 13 never change it.
- `life` (was `unkC`): 1 on every live frame of types 0, 1, 5, 10, 13, 17,
  then 0 on the frame the slot was freed; type 9 counted it down from 99.
  Types 3, 252, 255 leave it 0 and free themselves by other tests.
- `pathIndex` and `speed` (were `unk18`, `unk1C`): copied from the spawn
  record's fields of those names. For path types 0, 1, 5 and 17, `unk28` rose
  by exactly `speed` per frame (17809 of 17821 frames; the 12 others are type
  17 before its start segment, which the code gates).
- Left unnamed: `unk28`..`unk34` hold a different quantity per type (path
  position, velocity, a timer).

### Control types 250, 252, 255, confirmed 2026-09-26 (native build)

One full no-input ride (12012 frames) logged every spawn and free of a type
of 200 or more, with `g_JetSpeed`, `g_JetTrackSegment` and
`g_JetDrawEnabled`.

- 252 (`JET_OBJ_RIDE_START`): the first control object, at frame 46.
  Drawing was off on frames 0-45 and on from its spawn; it held
  `g_JetSpeed` at 0 until it freed itself 126 frames later, leaving 16384.
  It also spawns the type 3 object. Its fade is 253's with the shade
  reversed.
- 255 (`JET_OBJ_SPEED_CHANGE`): 15 spawns, each changing the speed by
  `-unk50[0]` per frame for `unk50[1]` frames while above `unk50[2]`. For
  example `[1200, 16, 16384]` took it from 80447 to 59020 (17 x 1200), and
  `[-1000, 120, 30000]` from 32905 to 166106. The last one, `[760, 800, 0]`,
  drove it below 0; the handler clamped it and spawned the 253 that ended
  the ride. So the retail ride ends through a 255, not a scheduled 253.
- 250 (`JET_OBJ_SCORE_CHECK`): the schedule has none, so one was injected at
  frame 300 (`call JetObjectCreate(0, 0, 0, 250, 29)` from the gdb script;
  an inferior call from inside a Python `stop()` hangs). With `unk50[0]` =
  100 and score 0 it spawned a 255 `[300, 400, 0]`, the speed reached 0 in
  54 frames, a 253 followed and `MINI_Jet` returned. With `unk50[0]` = 0
  nothing happened.
- `g_JetDrawEnabled` (was `g_JetTransitionDrawEnabled`): it gates both
  `DrawOTag` calls, so it is all drawing, not a transition. It read 0 on
  frames 0-45, 1 from 252's spawn, and 253 clears it at the exit.
- Left unnamed: type 3 sits 0x9C4 above the camera every frame; what it
  looks like still needs frames from Redux.

## What each object type looks like

Method: from the `jet-start` state with no input, the replay is
deterministic. A per-frame hook at `func_800A46E8` clears `active` on every
object except the types being kept. Each capture is pixel-diffed against the
same frame with every object hidden, so what differs is exactly the kept
type. (Hiding a parent stops its children spawning, so keep 8 with 9.)

The type selects a behaviour in `func_800A46E8`'s switch, and the model
selects the look. It is the only place the type is tested.

| type | models | on screen | notes |
| --- | --- | --- | --- |
| 1 | 31, 32 | small flying craft, 50/60 points | faces along its path |
| 3 | 59 | nothing visible (3 px) | alive the whole ride |
| 5 | 56 | sweeping light beams | path plus constant spin |
| 5 | 57, 71 | 30-point targets | same handler as the beams |
| 8, 9 | 67; 68-70 | a speck, then a burst of 20-36 specks | 8 spawns 9 |
| 10 | 78 | the player's cart (12k px) | placed by `JetTrackSample` |
| 17 | 89, 71 | the pumpkin with the cactus | |

Types 0, 2, 4 and 13 were not on screen in the first 1500 frames. 202 and
203 are spawned only by `JetObjectDamage`/`JetObjectAwardPoints`, so a
no-input replay never shows them.

`ff7-coaster`'s switch labels agree with every row above; its type 3
guess, "starfield", is not confirmed (hiding it removes only 10 px of stars,
most of the starfield survives with every object hidden). It also calls
`unkC` HP; it is `life` (a 0/1 alive flag, or a countdown for short-lived
types), and the hit points are `unk50[0xD]`.

These became `enum JetObjectType` in `jet_object.c`: 1, 5, 8, 9 and 10 from
the frames above plus their handlers, 202 from its spawn sites in
`JetObjectDamage`/`JetObjectAwardPoints`, 253 from the `g_JetExit` write.

### The tour: types later in the ride, 2026-09-24

The earlier isolation runs hid objects by clearing `active` without freeing
the slot, so `g_JetObjectCount` climbed to the cap and spawning stopped. That
is why types 0, 2, 4 and 13 never appeared; those "not seen" results do not
count. The tour hides nothing.

The spawn table (`g_JetSpawns`, `g_JetSpawnCounts` per segment) gives each
type's segments. `g_JetTrackSegment` advances with `g_JetSpeed`, so the tour
holds the speed at `0x40000` until 10 segments before a target, sets
`0x4000`, and captures 20/60/100/140 frames later with every object's box.

- 254 (`JET_OBJ_STOP`): spawned at segment 1867; `g_JetSpeed` read 0 for
  ~600 frames even though the tour kept writing `0x40000`, then resumed.
  Matches its handler: speed 0 until `unk50[0]` vsyncs pass, then
  `+= unk50[1]` for `unk50[2]` frames.
- 11 (`JET_OBJ_EXPLOSION`): 9 type-12 objects spawned in one frame as the
  cart restarted; its handler plays sfx `0x8E`, spawns `unk50[3]` of them
  and frees itself.
- 12 (`JET_OBJ_DEBRIS`): the captures show them scattered across the lava
  scene, moving outward.
- Seen but not named: 14 (m40, fiery streaks in the lava section; spawns
  15 and 16), 13 (m45, four thin beams; shares a case with 7, which tilts
  the object by `unk50[6]` per frame for `unk50[7]` frames), 4 (m43, only
  at the screen edge; its handler drops it with growing speed until
  `vy > 0`). 0, 2 were not in view at the capture offsets.

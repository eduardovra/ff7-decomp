# Probing the jet minigame at runtime

Naming the rest of `src/mini/jet/` has to be settled by evidence. The PC
reverse-engineering in `ff7-coaster` is a source of hypotheses, not of
names. This file records how to run jet in PCSX-Redux and what each run
established. Launching and the web API are covered in `magic-probe.md`.

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
  free list; `unkDA != 0` marks a live slot. A per-frame scan at
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

## What each object type looks like

Method: from the `jet-start` state with no input, the replay is
deterministic. A per-frame hook at `func_800A46E8` clears `unkDA` on every
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
| 10 | 78 | the player's cart (12k px) | same `case` as 5 |
| 17 | 89, 71 | the pumpkin with the cactus | |

Types 0, 2, 4 and 13 were not on screen in the first 1500 frames. 202 and
203 are spawned only by `JetObjectDamage`/`JetObjectAwardPoints`, so a
no-input replay never shows them.

Since 5 and 10 share one `case` and the type is tested nowhere else, the
cart and the beams differ only in their spawn data. Naming 10 "cart" would
name data, not code.

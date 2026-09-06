# Probing a magic overlay at runtime

`make build` proves an overlay is byte-identical, and `./mako.sh redux`
(see `emulator-loop.md`) gets a rebuilt one running. Neither answers what a
decompiled function *does* -- whether a constant really is a scale factor,
whether a ramp reaches the value the arithmetic predicts, whether what
appears on screen matches the comment above the code.

Companion docs: `magic-overlays.md` and `how-a-spell-is-drawn.md` for what
the descriptor fields already mean. Check those first. Both halves of
descriptor offset `0xA` were worked out there, and re-deriving them from
assembly wasted an afternoon.

## Launching

```shell
.venv/bin/python3 tools/redux_launch.py
```

That enforces the settings below and does not return until the Lua handler
answers. Do not launch Redux by hand for this work -- the settings are not
optional and two of them fail silently.

| setting | value | why |
| --- | --- | --- |
| `emulator/Dynarec` | `false` | debug facilities need the interpreter |
| `emulator/Debug/Debug` | `true` | arms the debugger |
| `emulator/Debug/GdbServer` | `false` | see below |
| `emulator/Debug/WebServer` | `true` | no CLI flag exists for this |

**The dynarec and the debugger must not both be live.** Running the GDB
server with the dynarec enabled produced green block corruption over the
picture and an emulator that eventually wedged. The interpreter is slower
and that is fine.

**`-gdb` persists into the config.** Passing it once sets
`Debug/GdbServer: true` in `pcsx.json` for good. Dropping the flag from a
later launch does *not* turn the server off, so an unstable configuration
survives a restart that looks like it should have cleared it. This is why
`redux_launch.py` writes the setting rather than relying on flags.

**Redux rewrites `pcsx.json` on exit**, from memory. Editing settings while
it runs achieves nothing. Stop it first, then write, then launch --
the order `redux_launch.py` uses.

`redux_launch.py --no-launch` reports the settings without touching a
running emulator.

## Redux's web API

Read off `src/core/web-server.cc`, and corrected against the running
emulator. Several of these do not behave the way their names suggest.

| route | reality |
| --- | --- |
| `GET /api/v1/cpu/ram/raw` | **ignores `offset` and `size`**; returns all 2MB in ~34ms. Those apply to the POST (write) form only |
| `GET /api/v1/gpu/vram/raw` | 1MB of VRAM |
| `GET /api/v1/screen/still` | the GUI's *offscreen texture*: black unless the window is actively rendering. Unusable for capture -- use VRAM |
| `GET /api/v1/state/save\|load\|delete?slot=N` | save-state slots 0-9, or `?name=` |
| `POST /api/v1/lua/<name>` | dispatches to `PCSX.WebServer.Handlers[<name>]` |
| `GET /api/v1/cd/files?filename=` | read a file off the loaded disc |

**There is no keep-alive.** Redux closes the connection after every
response, so a persistent-connection client fails outright.

## Running Lua

`/api/v1/lua/<name>` only reaches a *pre-registered* handler, so arbitrary
Lua cannot be run until one exists. `tools/redux_probe.lua` is that
handler, loaded at startup by `redux_launch.py` via Redux's `-dofile`.

Two constraints shaped it:

- **Input arrives in the query string.** The handler receives
  `urlData`/`method`/`headers`/`form` and no raw body. `form` is populated
  only for multipart requests, and then from the MIME part *headers*
  rather than their contents, so it cannot carry a variable of your own.
- **URLs are capped at roughly 256 bytes.** A longer query does not error;
  dispatch simply fails with `URL Not found`, which reads like a missing
  handler rather than an oversized request. The handler therefore exposes
  an append buffer, and `tools/redux_lua.py` chunks into it.

```shell
.venv/bin/python3 tools/redux_lua.py 'return PCSX.getMemPtr()[0x15169C]'
echo 'return 1 + 1' | .venv/bin/python3 tools/redux_lua.py
```

`PCSX.getMemPtr()` returns a pointer to the 2MB of WRAM, indexed by the
address masked into that space -- so `0x801B0000` is `0x1B0000`.

## Capturing frames

`magic_probe.py arm --shots DIR` writes the framebuffer on every breakpoint
hit, from inside the emulator via `PCSX.GPU.takeScreenShot()`, so there is
no per-frame HTTP cost and no sampling race. Each dump is packed XBGR1555
at the size the record's `shot=WxH` field gives; decode with
`vram_png.py --screenshot --width W --height H`.

The image is the last *completed* frame, so it carries the same one-frame
lag as a memory read taken at the render function's entry. Pair image N
with record N and both describe the state going into that frame.

`/api/v1/screen/still` uses the same capture and works too, but only while
the window is actually rendering -- it returns solid black otherwise, which
is what makes it useless for unattended runs.

```shell
.venv/bin/python3 tools/vram_png.py out.png            # 15bpp, the default
.venv/bin/python3 tools/vram_png.py out.png --bpp 24   # FMV
```

**Get the bit depth right before concluding anything.** Battle and field
render 15bpp; FF7's FMV is 24bpp. Decoding one as the other produces
structured garbage that reads convincingly as emulator corruption -- it
cost a false report of a broken renderer here. If a capture looks wrong,
try the other depth before believing it.

Most of a full VRAM dump looks like noise regardless: texture pages and
CLUTs are not pictures. Only the display rectangle means anything.

## The overlapping-address problem

Every magic overlay loads at `0x801B0000` and only one is resident at a
time. The collision is not theoretical: `MabariaRenderModel` and
`ThunderRenderModel` are both at `0x801B0020`. A read at `0x801B0CA0`
returns bytes with no indication of which spell they belong to, and a
breakpoint at `0x801B0020` fires for whichever one is loaded.

This matters in an ordinary battle, not just in theory: enemies cast too,
and every spell they cast loads a different overlay over the same address.
`drain` reports how many hits the guard dropped, so filtering is something
the run tells you rather than something you assume.

So guard every sample: read 32 bytes at `0x801B0000` and compare against
`build/us/<overlay>.exe`. That needs no per-overlay knowledge -- the seven
magic overlays diverge by the twelfth byte, since the `jal` in the third
word of the prologue points somewhere different in each.

Addresses outside `0x801B0000`-`0x801D0CAC` are unaffected. The battle
effect slot (`D_80162978`, indexed by `D_8015169C`) lives in battle's own
memory and is shared by every spell.

## Reaching a spell the party cannot cast

Most magic overlays sit behind progression. `LV5DETH.BIN` is on Disc 1 and
`build/us/lv5deth.exe` matches it, but Lv5 Death is an Enemy Skill learned
in the Northern Crater, so a Disc 1 save never casts it.

`--force TYPE:ID` puts any overlay on screen from any action:

```shell
python3 tools/magic_probe.py lv5deth arm --at 0x801B0074 \
    --watch D_8015169C:4 --watch D_80162978:0x200 --force 13:19
```

Two bytes decide which overlay a queued command uses, at `0x22` and `0x23`
of the acting unit's `BattleModel` (`D_801518E4`, stride `0xB9C`).
`func_800D1110` turns them into a disc read and `func_800D0C80` calls the
entry point the loaded overlay exposes, so a breakpoint on the entry to
each, rewriting both bytes from `a0`, is enough: the game does the load
itself and the animation runs on real disc data. `drain` reports how many
commands were rewritten.

The patch rewrites every unit's command while armed, enemies included, so
`disarm` when the capture is done.

### Finding the pair

Command type is the `D_80151907` case in `func_800D0C80`: 2 is Magic, 13
Enemy Skill, 4/8/20/32 the other tables. The id indexes that type's
dispatch table, and the parallel table in `func_800D1110` maps it to a
file record in `D_800EEBB8`, whose `loc` is an LBA. So read the LBA of the
overlay out of the ISO directory and find the record that carries it:

| overlay | LBA | record | table | id |
| --- | --- | --- | --- | --- |
| `LV5DETH.BIN` | `0x8897` | 76 | `D_800EF6A8` (E.Skill) | 19 |
| `BARRIER.BIN` | `0x7980` | 11 | `D_800EF63C` (Magic) | 15 |
| `BRIZAD.BIN` | `0x7913` | 7 | `D_800EF63C` (Magic) | 30 |

The two magic rows are the check on the method: both were captured by
casting them normally, before the force patch existed.

## Do not poll for per-frame data

A RAM snapshot is all 2MB and takes ~34ms, giving 29.6 per second. Battle
runs at about 15fps, so that is **2 samples per frame** -- enough to alias
badly across a 15-frame animation, and no client-side tuning changes it:
the cost is the full-RAM `memcpy` and transfer, not the request.

Use a breakpoint instead. `PCSX.addBreakpoint(address, 'Exec', 4, cause,
callback, label)` fires the callback once per execution, so a breakpoint on
the render function yields one record per frame with no sampling race, and
the callback reads memory in-process at no cost.

**The callback must return `true`.** Returning `false` deletes the
breakpoint -- it is how the binding disposes of an invoker that threw. A
callback returning `false` therefore fires exactly once, which presents as
a spell that rendered a single frame rather than as a broken probe. `true`
keeps it armed and does *not* pause the emulator: measured at 940k hits
over three seconds on a hot address with emulation still running.

## Static reading first

`D_801B0CA0` and `D_801B0CA4` in `mabaria` look like animation parameters,
but grepping the overlay's assembly for every store to them finds exactly
one writer, `MabariaMainSetup`, with constant values. A RAM trace would
only have replayed arithmetic already settled on paper.

Reach for the emulator when the question is about behaviour over time, or
about what actually reaches the screen.

A frame capture measures the whole screen, so it can only isolate one
effect when the overlay draws one thing. brizad renders a single model
pass, so bright-pixel counts tracked its fade cleanly. thunder renders the
model plus two textured-quad passes, over an enemy flashing red from
damage; its brightness curve is consistent with the model dimming but
cannot be attributed to it. Check how many passes an overlay draws before
planning to prove anything from pixels.

And prefer memory to pixels wherever both could answer. Confirming that
brizad's ice block does not rotate is impossible from the images: the
crystal is an eight-pointed symmetric star, so a 45-degree rotation is
indistinguishable from none. `Rot` reading `(0,0,0)` every frame settles
it outright. Screenshots earn their place on the complementary question --
whether a value that provably reaches `IR0` actually darkens the picture --
which no memory read can answer.

# Running a rebuilt overlay

`make build` proves an overlay is byte-identical. It says nothing about
whether changed code *runs* -- and once a function is deliberately altered,
or a decomp is still non-matching, sha1 stops being an answer at all.

`./mako.sh redux` closes that gap: it pushes a freshly built overlay and its
symbols into a running PCSX-Redux. Nothing under `disks/` is written. The
emulator keeps the patch in a PPF layer over the loaded image, so the
original `.bin` stays untouched and a single command puts it back.

## Setup

```shell
make bin/pcsx-redux        # ~85 MB AppImage from distrib.app
```

Upstream publishes no tagged releases, only rolling builds, so there is no
hash to pin the way `bin/cc1-*` is pinned. The Makefile records the sha256
of whatever build it fetched in `bin/pcsx-redux.sha256`.

Launch it with the disc, the web server on, and the GDB server on:

```shell
./bin/pcsx-redux -iso "disks/Final Fantasy VII (USA) (Disc 1).cue" \
    -run -gdb -stdout
```

`-gdb` persists in the settings. The web server has no flag; turn it on once
under Configuration > Emulation > Enable Web Server. It listens on
`localhost:8080`, the GDB server on `3333`.

No retail BIOS is needed -- the bundled OpenBIOS boots the game.

## Use

```shell
make build                 # or ninja, for one object
./mako.sh redux savemenu   # patch + symbols + soft reset
```

| Flag | Effect |
| --- | --- |
| `--symbols-only` | Upload symbols, leave the disc image alone |
| `--no-reset` | Skip the soft reset, leaving the running game alone |
| `--clear` | Drop every patch, restoring the loaded image |
| `--host` | Point at a different `host:port` |

The overlay name is the one in `config/us.yaml` -- `savemenu`, `battle`,
`field`, `main`, and so on. `disk_path` supplies the file to overwrite, so
compressed overlays are re-wrapped (deflate plus the 8-byte header)
automatically.

## What you get, and what you don't

**Symbols are per-overlay, by necessity.** Overlays share load addresses --
every menu lives at `0x801D0000`, battle/field/world all at `0x800A0000` --
so uploading them all at once would produce collisions. Each run resets the
symbol table and uploads `main` plus the one overlay named.

**These symbols are Redux's, not GDB's.** They name functions in Redux's own
disassembly and memory views. A GDB client wants its own copy:

```shell
gdb-multiarch -ex 'target remote localhost:3333' \
    -ex 'add-symbol-file build/us/savemenu.elf'
```

`magic-probe.md` builds on this loop: it samples RAM and captures
frames while a spell plays, guarding every read against the overlay
residency problem that shared load addresses create.

## Limits

Read this before planning a change that adds code.

### A pushed file cannot grow past its extent

The emulator writes inside the file's ISO directory-record size -- not its
sector-rounded size, so the sector slack in `disc-building.md` is not
available here. Measured headroom, payload against the file on disc:

| overlay | on disc | headroom |
| --- | --- | --- |
| `battle` | 130,322 | 396 |
| `field` | 85,435 | 245 |
| `world` | 66,715 | 116 |
| `batini` | 5,696 | 2 |
| everything else | -- | **0** |

Ten of the fifteen overlays have no room at all: an uncompressed overlay
that matches is exactly the size of the file it replaces. The four with
headroom only have it because `gzip -9` happens to beat the original
stream. Go's `compress/gzip` runs about 2% fatter and overflows all of
them, which is why the re-wrap shells out.

So this loop is for changing *values* -- colours, coordinates, timings,
thresholds, table contents. `Payload` in `redux/overlay.go` refuses an
oversized push rather than letting the emulator truncate it.

The re-wrap is otherwise unconstrained: only the gzip FLG byte has to match
the original header. `Unzip()` in `src/main/unzip.c` checks the magic, the
method, and that no flags are set, then skips the remaining six header
bytes outright.

### `main` cannot change size at all

Growing a function in a leaf overlay is fine. Adding a statement to
`src/magic/brizad.c` took `brizad.exe` from 4116 to 4128 bytes, linked
clean, and changed no other overlay's sha1.

Doing the same in `main` breaks the build: every overlay fails to link with
hundreds of undefined references to functions in main's address range, and
a clean `make rebuild` does not clear it.

**The mechanism is not established.** The obvious explanation -- that
address-derived `func_<ADDR>` names stop matching -- does not survive the
evidence: the symbol export's names are identical before and after, only
the addresses move, which `ld` should handle. Something in the
splat/`config/sym_export.us.txt` regeneration chain fails to converge when
main's layout changes. Worth pinning down; until then, treat main as
size-frozen.

### Debug info is function level

cc1 is invoked with `-g -gcoff` and the C never touches disk
(`cpp | cc1 | maspsx | as`), so only bare `LM*` line labels survive into the
ELF. Expect named functions and instruction stepping, not C line stepping.

## Getting past the size limit

Neither limit is a property of the decomp -- the build produces grown code
happily. They are properties of *this delivery path*, and there are two
ways around them.

**Write to RAM instead of the disc.** `POST /api/v1/cpu/ram/raw?offset=&size=`
writes anywhere in the 2 MB with no extent check. Verified: 128 KB pushed
to `0x801B0000`, the magic overlays' load address, and read back identical
-- roughly 32x what those overlays occupy on disc. The budget becomes the
overlay's room in RAM, not its file size. The catch is timing: the game
reloads an on-demand overlay from disc on its next use, so a RAM write has
to land while the emulator is paused at the right moment. Not wired into
`./mako.sh redux` yet.

**Rebuild the disc.** Relocating a file removes the extent limit outright,
at the cost of patching the sector tables that name it. See
`disc-building.md` -- `cebix/ff7tools`' `fixup` already does most of this
for FF7 specifically.

## Follow-ups

In the order they are worth doing.

1. **Root-cause the `main` failure.** The highest-leverage one: it is the
   limit that actually constrains decomp work, and it may be nothing more
   than a build-ordering bug. Reproduce by changing the size of any function
   in `main`, then follow `config/sym_export.us.txt` and the splat-generated
   `build/us/undefined_syms.*.txt` through a clean rebuild and find where
   they stop agreeing. If it is a bug, main stops being size-frozen and the
   "cannot change size" section above goes away.
2. **Wire `--to-ram` into `./mako.sh redux`.** Writes the overlay to its
   `vram_start` through `cpu/ram/raw` instead of patching the disc, which
   makes grown overlays testable without any extent limit. Needs a way to
   land the write at the right moment -- probably a breakpoint on the
   overlay's entry point, since an on-demand overlay is re-read from disc
   every time it is used.
3. **Build the disc injector.** Turns this from a test loop into something
   that produces a playable image: inject within the sector slack first,
   then the table patching needed to relocate a file that outgrows it. See
   the order of work in `disc-building.md`.

## Checking the push actually landed

A stock build pushes identical bytes, so a successful patch and a silent
no-op look the same. Read the file back out of the emulator to tell them
apart:

```shell
curl -s -o /tmp/live.bin \
    'http://localhost:8080/api/v1/cd/files?filename=MENU/SAVEMENU.MNU;1'
cmp /tmp/live.bin build/us/savemenu.exe
```

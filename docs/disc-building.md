# Rebuilding a playable disc

Notes for getting modified overlays back into a bootable image. Nothing
in this repo does this today -- the pipeline is extraction and
verification only, one-directional. This doc records what was measured
about the on-disc formats, the two routes forward, and the prior art
worth copying rather than reinventing.

## What the repo does today

- `make disks` -- `bchunk` converts `.bin`/`.cue` to an ISO, then
  `7z x` extracts the file tree into `disks/us/`. Read-only, and it
  discards the CD-XA audio and STR video, which live in Mode 2 sectors
  that a plain ISO extraction cannot represent.
- `make build` -- produces `build/us/*.exe` and writes
  `build/us/check.sha1`, comparing each against the original file. The
  `<overlay>.exe: OK` lines are that comparison.
- The compressed overlays listed in `OVL_US` (`BATTLE/*.X`,
  `FIELD/FIELD.BIN`, `MINI/CHOCOBO.BIN`, `WORLD/WORLD.BIN`) are
  inflated to `.dec` files purely as diff targets.

There is no gzip *writer* anywhere in `tools/` -- the only
`gzip.NewReader` is in `tools/builder/deps/setup.go`, unpacking the
downloaded `cc1` binaries. And there is no ISO9660 packer.

## Two overlay storage formats

**MENU overlays are stored uncompressed**, and the build output is
byte-identical to the file on disc:

```shell
cmp disks/us/MENU/SAVEMENU.MNU build/us/savemenu.exe   # identical
```

Both are 113732 bytes. Patching one is therefore a straight
byte-for-byte overwrite at the same offset -- same start sector, same
length, same directory record, filesystem untouched.

**The `OVL_US` overlays are gzip-wrapped** behind an 8-byte header,
which is what the Makefile's `tail --bytes=+9` skips:

```
BATTLE.X: ac 38 05 00  d8 41 07 00  1f 8b 08 00 ...
          |            |            gzip magic
          |            second u32 = 0x000741D8 (475608), meaning unknown
          u32 LE = 0x000538AC (342188) = decompressed size
```

Re-wrapping one means deflating and rebuilding that header. A
recompressed stream lands within roughly half a percent of the original
size, but that is luck, not a guarantee.

## Sector slack

Files are 2048-byte sector aligned, so a patched file can grow into the
tail of its last sector before it has to move:

| file | on disc | slack |
| --- | --- | --- |
| `MENU/SAVEMENU.MNU` | 113,732 | 956 |
| `MENU/BGINMENU.MNU` | 2,200 | 1,896 |
| `BATTLE/BATTLE.X` | 130,322 | 750 |
| `FIELD/FIELD.BIN` | 85,435 | 581 |
| `WORLD/WORLD.BIN` | 66,715 | 869 |

Grow past the slack and the file has to be relocated, which means
rewriting directory records -- at that point you are rebuilding the
disc, not patching it.

## Route 1: in-place injection

Write the new bytes into the existing image. Everything else on the
disc -- XA audio, STR video, the other overlays, the original sector
layout -- is untouched, which is precisely what an extract-and-repack
round trip destroys.

Caveat on mechanics: a `.bin` is raw 2352-byte sectors, so this is not
a plain file-offset splice. Each sector carries EDC/ECC that has to be
recomputed. `PSX-Mode2`, CDmage and mkpsxiso's patch mode all do this.
Working on a 2048-byte/sector `.iso` makes it a straight splice, but
that variant has already lost the XA tracks.

This is the cheapest path for the uncompressed `MENU/*.MNU` overlays,
and it sidesteps the LBA question below entirely.

## Route 2: full disc rebuild

Needed once a file no longer fits, or for shipping a complete image.

**Known pitfall:** mkpsxiso does not reproduce a game's original
layout. Even with no files changed, the rebuilt image assigns different
LBAs. That is fine for a game that resolves files through the ISO9660
directory, and fatal for one that carries its own hardcoded sector
table. *Which of these FF7 does is not yet established* -- it is the
first thing to check, and the test is cheap: rebuild an unmodified
image and see whether it boots.

## Prior art

- **[sotn-decomp](https://github.com/Xeeynamo/sotn-decomp)** -- same
  maintainer as this project, and the closest template. `make disk`
  produces `build/sotn.us.cue` via `bin/sotn-disk`, a purpose-built Go
  tool with **its own ISO9660/CD-XA writer** rather than a wrapper
  around mkpsxiso. Subcommands: `list` (files in disc-storage order),
  `extract`, `inject` (route 1), `make` (route 2). It is a
  self-contained Go module (`tools/sotn-disk/go.mod`, packages `discs/`
  and `iso9660/`, no third-party deps), and this repo's builder is Go
  too -- so adopting it is mostly a matter of feeding it FF7's file
  list, not writing a disc writer.
- **[mkpsxiso](https://github.com/CookiePLMonster/mkpsxiso)** /
  `dumpsxiso` -- the scene-standard pair. `dumpsxiso` emits an XML
  layout that `mkpsxiso` consumes, preserving XA/STR and sector modes
  on the round trip.
- **[psxbuilder](https://github.com/SaturnKai/psxbuilder)** -- thin
  mkpsxiso wrapper aimed at the mod use case.

## Suggested order of work

1. Confirm the LBA question by rebuilding an unmodified image and
   booting it. Everything else depends on the answer.
2. Port `sotn-disk`'s `inject` first -- it makes the uncompressed MENU
   overlays moddable immediately, with no filesystem changes.
3. Add the gzip re-wrap (deflate plus the 8-byte header) for the
   `OVL_US` overlays. `sotn-disk` knows nothing about FF7's container.
4. Only then take on `make`, the full rebuild.

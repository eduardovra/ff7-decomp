# Rebuilding a playable disc

Notes for getting modified overlays back into a bootable image. Nothing
in this repo does this today -- the pipeline is extraction and
verification only, one-directional. This doc records what was measured
about the on-disc formats, the two routes forward, and the prior art
worth copying rather than reinventing.

To *test* a rebuilt overlay rather than ship one, see
`emulator-loop.md` -- `./mako.sh redux` patches the image loaded in
PCSX-Redux without writing any file at all.

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

The second u32 follows from no size that can be computed -- it is 0, 4,
16504 and 475608 across the seven files, uncorrelated with the
compressed or decompressed length. Carry it through verbatim.

Re-wrapping one means deflating and rebuilding that header. A
recompressed stream lands within roughly half a percent of the original
size, but that is luck, not a guarantee: `gzip -9` fits every overlay
while Go's `compress/gzip` overflows all of them.

Only one header field has to be right. `Unzip()` in `src/main/unzip.c`
checks the gzip magic, that the method is deflate, and that the flags
byte is zero -- then skips the remaining six bytes of the header without
looking. MTIME, XFL and OS are free.

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
table.

FF7 is the second kind. It has a full ISO9660 hierarchy and mostly
ignores it:

- `SCUS_941.63` carries `(LBA, size)` pairs at roughly `0x39500` to
  `0x39830`. `55000/85435` is `FIELD/FIELD.BIN`; `54165/114688` is
  `MENU/SAVEMENU.MNU`, with the rest of the MENU set following in
  sequence. Some entries hold the exact byte size, others the size
  rounded up to a sector.
- `MINT/MOVIE_ID.BIN` is a second table: 54 records, stride `0x14`,
  absolute movie sectors running from 129252.
- `MINI/CHOCOBO.BIN` and `FIELD/FIELD.BIN` hold further tables (see
  ff7tools below).

So relocating a file means patching every table that names it. Staying
inside the sector slack above avoids the question entirely.

## Prior art

- **[cebix/ff7tools](https://github.com/cebix/ff7tools)** -- the closest
  match to our problem, and FF7-specific. Its `fixup` tool patches an
  image in place after a rebuild, recomputing the LBN references in
  `MINT/MOVIE_ID.BIN` (offsets and stride `0x14`, checked against a
  known first value per region), `MINI/CHOCOBO.BIN` and
  `FIELD/FIELD.BIN`. It does *not* appear to touch the `SCUS_941.63`
  table above, which is where our MENU overlays are indexed. Workflow is
  `psxrip` -> edit -> `psxbuild` -> `fixup`. Its README warns that
  images mastered this way do not boot on real hardware, the disc
  signature not being reproducible; emulators are fine.
- **[cebix/psximager](https://github.com/cebix/psximager)** -- CD-XA
  aware dump/master pair. `psxinject` replaces a file inside a raw
  BIN/CUE "preserving the file's name, attributes, and start sector",
  which is Route 1 already implemented.
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

The LBA question is settled: FF7 reads by absolute sector, so any route
that moves a file has to patch the tables listed above.

1. Injection that stays inside the sector slack, so no table changes.
   Either port `sotn-disk`'s `inject` or wrap `psxinject`. The gzip
   re-wrap logic already exists in `tools/builder/redux/overlay.go`.
2. Table patching, for files that outgrow their slack. Port what
   `ff7tools/fixup` knows, and add the `SCUS_941.63` table it omits.
3. Only then take on the full rebuild.

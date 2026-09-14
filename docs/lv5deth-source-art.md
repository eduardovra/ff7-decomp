# Lv5 Death: source art and what shipped

The skull in `LV5DETH.BIN` is not geometry. It is eight pre-rendered
sprite frames in a texture. The 3D model it was rendered from never
reached the console, but a copy of it survives in the PC release, so both
ends of the asset pipeline can be read and compared.

This doc records that comparison. For the render path the overlay uses,
see `how-a-spell-is-drawn.md`; for the overlay structure, see
`magic-overlays.md`.

## 1. The source files in the PC release

Four files, all ASCII, all dated 17 July 1996. They carry their original
path on the developer's disk:

```
/1d/proj/ff7/CG_OBJ/effect_chara/deathlv5/geo/
```

| file | what it is |
| --- | --- |
| `deathlv5.geo` | the model: a group of two polyhedra, `head` and `jaw` |
| `deathlv5_sk.geo` | the skeleton: 4 bone nodes, 3 bones |
| `deathlv5.hrc` | bone names, parents and lengths; points at the skeleton |
| `script-1.script` | the animation |

These predate the 1998 PC port by two years and target the PlayStation.
They are Square's PSX-era working files, not anything the porting team
produced.

## 2. The authoring tool

The Symbolics S-Graphics suite. Package prefixes name the components:
`3D` is S-Geometry, `DYNA` is S-Dynamics, `SP` is the shared object
store. Files are Common Lisp source, which is the tool's native save
format rather than an export. Coordinates are floats, units are feet,
and faces are defined by edge loops.

Surfaces carry an attribute context tagged `:PSX`, so the target was
already known while the art was being made.

## 3. What the source describes

`deathlv5.geo` holds two polyhedra. The counts in its own description
strings match a parse of the vertex lists exactly:

| part | vertices | edges | faces |
| --- | --- | --- | --- |
| `jaw` | 91 | 251 | 162 |
| `head` | 212 | 582 | 372 |

Bounding box over both parts, after applying each part's matrix:

```
X   -20.11 ..  20.11   span 40.23   (symmetric about 0)
Y     0.42 ..  44.08   span 43.66
Z   -48.99 ..  12.54   span 61.53
```

The long axis is Z, so the model is authored lying back. The view that
corresponds to looking the skull in the face is X across and Z up.

Named face groups are `skin`, `BL` and `BL2`, plus a `HARDEDGE` edge
group. Diffuse colours are near-black greys and `:LIGHT-CALCULATION` is
off.

`deathlv5_sk.geo` and `deathlv5.hrc` agree on three bones: `head`, `jaw`
and one called `none` that is locked to another node. The head and jaw
meshes are bound as skins.

`script-1.script` is one `DYNA::SCRIPT` containing three actions and
eight cues. Only two operations are invoked, `3D::MAKE-VISIBLE` and
`DYNA::ANIMATE-POSE`. Times are ratios of 0.0 and 1.0, with a single
offset of 0.033333335, one frame at 30fps. The pose keys themselves are
not in this file.

## 4. The overlay's data layout

`LV5DETH.BIN` is raw, loaded at `0x801B0000`, no header. Segments as
split in `config/us.yaml`:

| offset | contents | size |
| --- | --- | --- |
| `0x00000` | code | 1504 |
| `0x005E0` | skull texture, TIM | 65824 |
| `0x10700` | sprite quad blocks | 360 |
| `0x10868` | ring model | 1500 |
| `0x10E44` | `.data` | 12 |
| `0x10E50` | two primitive buffers, zeroed on disc | 98304 |
| `0x28E50` | buffer pointer, flip slot, target count | 12 |

The rows sum to the file's 167516 bytes, most of it zeroed buffer space.

## 5. The texture

A standard TIM, 8bpp with a 256-entry CLUT.

```
magic 0x10  flags 0x09        (8bpp, CLUT present)
CLUT   256x1  at VRAM (768, 255)   524 bytes
image  128x255 halfwords = 256x255 pixels at VRAM (768, 0)   65292 bytes
```

The image data ends exactly at `0x10700`, where the quad blocks begin.

It is a 4-by-2 grid of skull frames, each 64 by 127 pixels. See
`lv5deth-frames.png`.

## 6. The sprite quad blocks

`g_Lv5DeathSpriteModel` is read by `func_800D4D90`, which skips an
8-byte header and then walks blocks. The header here is:

```
0x00002023   purpose unknown
0x00000008   frame count
```

Eight blocks follow, one per frame. Each block is a 4-byte header whose
low halfword at offset 2 is the quad count, always 2, followed by two
20-byte records. The record fields, as observed:

| offset | field |
| --- | --- |
| `0x0` | `0x0100` or `0x0300`, purpose unknown |
| `0x4` | base corner, x and y packed in one word |
| `0x8` | u and v into the texture page |
| `0xC` | tpage, then clut |
| `0x10` | width in the low byte, height in bits 16-23 |

Every record uses tpage `0x0AC` and clut `0x3FF0`. Those decode back to
the TIM exactly: page origin (768, 0), 8bpp, and a CLUT at (768, 255),
since `(255 << 6) | (768 >> 4)` is `0x3FF0`. The tpage also sets
semi-transparency mode 1, additive, which is why the black around the
skull reads as empty rather than being masked.

Each frame is drawn as two stacked 64 by 64 quads at corners
`(-32, -32)` and `(-32, 32)`, tiling the texture as:

| frame | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| u | 0 | 64 | 128 | 192 | 0 | 64 | 128 | 192 |
| v | 0 | 0 | 0 | 0 | 128 | 128 | 128 | 128 |

One detail identifies the converter as mechanical. In the bottom row the
lower quad is 64 by 62, not 64 by 64, because the image is 255 pixels
tall and the last two rows do not exist. A person would have padded the
texture to 256.

## 7. The frames really are separate renders

Measured against frame 0, the fraction of differing pixels:

```
frame 1  51.4%      frame 5  55.4%
frame 2  55.8%      frame 6  52.3%
frame 3  57.1%      frame 7  50.9%
frame 4  56.8%
```

They form a short rotation loop. This is the output of the pose
animation in `script-1.script`, baked to pixels.

## 8. Mesh against sprite

Projecting the source mesh onto X and Z, the plane identified in section
3, gives the same silhouette as the sprite: a rounded dome, a row of
small bumps down each side, and a fringe of long teeth below the jaw.
See `lv5deth-mesh-vs-sprite.png`.

The proportions differ:

| | width / height |
| --- | --- |
| sprite frame 0, non-black bounding box | 0.49 |
| mesh, same projection | 0.65 |

That is consistent with a perspective camera, since the model is half
again as deep as it is wide.

Nothing structural survived. The 212-vertex head, the 91-vertex jaw, the
three bones and the pose script existed only to produce eight images.
The jaw is a separate object in the source purely so it could move before
rendering; on the console it is part of the picture.

## 9. The ring model

`g_Lv5DeathRingModel` is the only real mesh in the overlay, and it
follows the format in section 11 of `how-a-spell-is-drawn.md`: a vertex
table size in bytes, then 8-byte vertices, then records holding vertex
byte-offsets and per-vertex colours with a GPU command byte.

```
vertex table  392 bytes = 49 vertices, all at z = -21
  1 vertex  at the centre
 16 vertices at radius 520-521
 16 vertices at radius 598-599
 16 vertices at radius 975-976
```

Two primitive groups follow, and the parse lands exactly on the segment
end at `0x10E44`:

| group | header | primitives | code | spans |
| --- | --- | --- | --- | --- |
| 1 | 12 bytes, last word 16 | 16 triangles | `0x30` | centre to r 520 |
| 2 | 4 bytes, value 32 | 32 quads | `0x38` | r 520 to 599, r 599 to 976 |

The first two words of group 1's header, `0x00200000` and `0x00000000`,
are not explained. Both groups are gouraud shaded and untextured. Only
four vertex colours appear across the whole mesh: black, white and two
reds.

So it is a flat disc built as a 16-spoke fan plus two rings of quads,
not a single ring. The comment on `g_Lv5DeathRingModel` in
`src/magic/lv5deth.c` currently calls it a 16-point ring of radius 976,
which describes only the outer edge.

## 10. What the missing converter did

The tool that turned section 1 into sections 5 through 9 is not in the
leak. Its job is visible from the two ends:

| in the source files | on the console |
| --- | --- |
| float coordinates in feet | 16-bit fixed point |
| faces defined by edge loops | flat vertex byte-offsets |
| named material groups with a diffuse colour | per-vertex colour words |
| no GPU concept | a command byte per primitive |
| a skinned, posed 3D model | eight rendered frames in a TIM |

The 8-byte vertex with its unused padding halfword is the clearest sign
of a PSX-specific target: it exists so the GTE can be loaded 32 bits at a
time by `lwc2`, without the data passing through a general-purpose
register.

For the skull specifically the converter was not a geometry exporter at
all. It was a renderer.

## 11. Reproducing this

Everything above comes from `disks/us/MAGIC/LV5DETH.BIN` and the four
source files. The offsets needed:

```
0x005E0  TIM: u32 magic, u32 flags, then CLUT block, then image block,
         each block being u32 length, u16 x, u16 y, u16 w, u16 h
0x10700  sprite: 8-byte header, then 8 blocks of
         (4-byte header, 2 records of 20 bytes)
0x10868  ring:   u32 vertex table size, vertices of 4 s16,
         then group 1 (12-byte header, 16 records of 20 bytes),
         then group 2 (4-byte header, 32 records of 24 bytes)
```

TIM block lengths include their own 12-byte header.

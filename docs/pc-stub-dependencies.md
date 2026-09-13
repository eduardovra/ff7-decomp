# PC stub dependencies

Upstream PR [#167](https://github.com/Xeeynamo/ff7-decomp/pull/167) ("Add PsyZ
support") boots the title screen natively. Xeeynamo will not merge it "until
most of the stubbed dependencies are decompiled", so that stub list is a
ready-made, externally motivated work queue: every entry we match shrinks the
gate on his PR *and* removes a placeholder from our own `src/pc/stubs.c`.

Derived from PR head `1fea402`, cross-referenced against our `personal` branch.
77 symbols are stubbed there; 3 are data (`delete`, `D_800A00CC`, `D_800A1158`),
leaving **74 functions -- 22 we already have in C, 52 still missing.**

Our `src/pc/stubs.c` is a different, much larger list (320 functions, 927
globals) because our CMake links the whole tree rather than just the boot path.
Do not confuse the two: the 52 below are the critical path to a title screen,
the rest are ordinary decomp backlog.

## Already decompiled on `personal` (22)

These are stubbed upstream but real here, so his list overstates the remaining
work by roughly a third:

`SysBattleSwirlInit` `SysBattleSwirlRender`
`SysCalculateTotalLureGilPreemptiveValue` `SysDecompKernStringWithF9`
`SysInitPlayerStatFromEquip` `SysInitPlayerStatFromMateria`
`SysMenuDrawMainMenu` `SysMenuDrawTexturedRect` `SysMenuInitInput`
`SysMovieAbortPlay` `func_800A14BC` `func_800A16CC` `func_800A17C0`
`func_800A19A4` `func_800A1EEC` `func_800A1F48` `func_800A3178` `func_800A358C`
`func_800CF60C` `func_801D080C` `func_801D0BA0` `func_801D3228`

## Still missing (52)

### Menu drawing and input -- 15

The densest cluster and where this fork already has momentum (`#166` landed
`SysGetSingleStringWidth` / `SysMenuDrawString`). 26 asm files still carry a
`SysMenuDraw*` label.

`SysMenuDrawWindow` `SysMenuDrawAddWindow` `SysMenuDrawMenuList`
`SysMenuDrawSingleFontLetter` `SysMenuDrawProgressBar`
`SysMenuDrawScrollbarSlider` `SysMenuDrawScrollbarTrack`
`SysMenuDrawDigitsWithLeadingZeroes` `SysMenuDrawDigitsWithoutLeadingZeroes`
`SysMenuDrawBattleResult` `SysMenuGetMenuListState` `SysMenuHandleButtons`
`SysMenuSetMenuListAnimation` `SysKernGetString` `SysBgFadeRender`

### CD-ROM and file loading -- 11

What his `src/pc/cdrom.c` currently papers over. Matching these is what lets
the PC build read real assets instead of a shim.

`SysCdromGetPackPointer` `func_80034410` `func_80034444` `func_80034D5C`
`func_80034F3C` `func_8003DDA4` `func_8003DE2C` `func_8003DE6C` `func_8003DE84`
`func_80041D28` `func_80041E30`

### `func_801D*` overlay block -- 10

61 asm files carry a `func_801D*` label, so this cluster is far larger than the
10 the boot path touches; scope carefully before starting.

`func_801D131C` `func_801D1A6C` `func_801D2D74` `func_801D2DA8` `func_801D2E84`
`func_801D2F00` `func_801D3018` `func_801D3138` `func_801D3478` `func_801D4118`

### Movie and audio -- 3

`SysMovieLoadMovieSettings` `SysMoviePlay` `SystemAkaoExecute`

### Unclassified -- 13

`MulMatrix2` `SysGetMinutesFromSeconds` `func_8001117C` `func_800211C4`
`func_80025ED4` `func_80029818` `func_8002988C` `func_80029998` `func_80036244`
`func_80036298` `func_800484A8` `func_80048540` `func_800D8D78`

## Regenerating this list

The list drifts as either side lands work. To refresh it:

```shell
gh pr diff 167 --repo Xeeynamo/ff7-decomp > /tmp/pr167.diff
awk '/^diff --git a\/src\/pc\/stubs.c/{p=1} /^diff --git a\/src\/pc\/title_data.c/{p=0} p' /tmp/pr167.diff \
  | grep '^+' | grep -oP '^\+(?:[A-Za-z_][A-Za-z0-9_ *]*\s+\**)?\K[A-Za-z_][A-Za-z0-9_]*(?=\()' \
  | sort -u | grep -vE '^(delete|D_)' > /tmp/his.txt
grep -rhoE "^[A-Za-z_][A-Za-z0-9_ *]*[ *]([A-Za-z_][A-Za-z0-9_]*)\(" src --include='*.c' \
  | grep -oE '[A-Za-z_][A-Za-z0-9_]*\($' | tr -d '(' | sort -u > /tmp/defined.txt
comm -23 /tmp/his.txt /tmp/defined.txt   # still missing
```

Note his `stubs.c` is hand-written, so a symbol leaving it means he decompiled
it *or* stopped calling it -- check the diff rather than assuming progress.

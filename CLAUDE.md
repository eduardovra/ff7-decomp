# ff7-decomp

PSX Final Fantasy VII decompilation. The goal is byte-exact matching
overlays, so **codegen is the specification** -- readability changes are only
valid if the sha1 still matches.

## Read first

- `docs/decomp-workflow.md` -- tooling traps and gcc 2.6.3 codegen gotchas.
  Read before touching a non-matching function.
- `docs/magic-overlays.md` -- architecture of the spell overlays under
  `src/magic/`.
- `docs/how-a-spell-is-drawn.md` -- one cast traced to the GPU packets.
  Section 13 lists names already found wrong and corrected. Read it before
  naming anything in the battle render path: several fields there mean
  different things depending on which renderer consumes them, and that has
  been re-derived from assembly more than once for want of reading it.
- `docs/emulator-loop.md` -- `./mako.sh redux`, which runs a rebuilt overlay
  in PCSX-Redux. Use it when sha1 cannot answer the question.
- `docs/magic-probe.md` -- sampling RAM and frames from a running spell,
  and the residency guard the shared `0x801B0000` load address needs.

## Verify every change

```shell
ninja build/us/src/<path>.c.o                              # fast syntax check
.venv/bin/python3 tools/asm-differ/diff.py -mows <func>    # score, 0 = match
make build                                                 # sha1, the real test
```

`<overlay>.exe: OK` is the only proof a change is correct. Per-function
scores of 0 are necessary but not sufficient -- data layout can still be
wrong.

Never claim a match without running `make build`.

## Working rules

- Make one change at a time and re-verify. When testing a hypothesis rather
  than applying a known fact, keep it as its own step so a break is
  attributable.
- Do not "simplify" code that matches. Casts, repeated loads and redundant
  re-tests are usually load-bearing -- they exist because the target's
  instructions require them. Add a comment instead of removing them.
- Restore the working tree after experiments, and say so.
- `docs/`, `CLAUDE.md` and the probe tooling are fork-only -- none of them
  exist on `main`. Never cite them from anything that goes upstream (`src/`,
  `include/`, `config/`). A code comment has to stand on its own, so put the
  finding in the comment rather than a pointer to where it was written up.
- `config/sym_ovl_export.us.txt` and `config/sym_export_battle.us.txt` are
  build-generated. Do not hand-edit them.
- Use `./mako.sh symbols add` rather than editing symbol files by hand.

## Conventions

- Identifiers derive from the ROM filename: `BRIZAD.BIN` -> `brizad.c` ->
  `BrizadData`. Abbreviated English words get repaired (`LV5DETH` ->
  `Lv5Death`); romanised Japanese names are not translated. Put the English
  gloss in a comment so the file stays greppable by both names.
- Fixed point is 4096. Prefer named constants over raw values once the
  meaning is known, but verify the build afterwards.

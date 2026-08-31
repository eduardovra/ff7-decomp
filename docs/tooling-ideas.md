# Matching-automation tooling ideas

Ideation notes on automating more of the m2c -> compiler -> permuter loop
by leaning into the specific compiler versions this project uses
(cc1-psx-26 / cc1-psx-272). Companion to the ideation doc that started
this (predictive register-allocation layer); the key correction to that
doc: don't *model* the allocator, *instrument* the real compiler.

## Key facts established

- The shipped `cc1-psx-26` accepts `-da` and emits per-pass RTL dumps
  (`jump`, `cse`, `loop`, `combine`, `lreg`, `greg`, `flow`, `dbr`).
  The `.greg` dump prints the allocator's answer directly
  (`Register dispositions: <pseudo> in <hard reg>`), so no compiler
  rebuild or reimplementation of `global.c` is needed.
- m2c's `--valid-syntax` guarantees parseable output with `M2C_*`
  macros marking semantic holes — greppable, localized repair targets.
- decomp-permuter mutates a real pycparser AST (~35 named passes) and
  has two extension surfaces usable without forking:
  - `PERM_*` macros in the input C scope/enumerate the search space
    exhaustively (`PERM_GENERAL`, `PERM_LINESWAP`, ...).
  - Per-pass weights, overridable via `[weights]` in
    `permuter_settings.toml`.

## Proposed pipeline

```
m2c (--valid-syntax, resolver-checked context)
  -> cc1 -O2 -G0 -da            (compile candidate, collect dumps)
  -> diagnosis                  (diff dumps + asm facts vs target:
                                 which pass, which variable, which
                                 direction)
  -> PERM annotations + weight override
  -> permuter (unmodified, bounded budget)
  -> strip macros from best candidate, repeat
  -> match, or "stuck" report with the diagnosis attached
```

Weights pick *what kind* of mutation, PERM macros pick *where*.
The permuter stays an unmodified subprocess; the intelligence lives in
the driver.

## Ranking

| # | Idea | Effort | Payoff | Risk |
|---|------|--------|--------|------|
| 1 | Corpus harness + m2c failure taxonomy (compile matched funcs, categorize misses) | days | informs every other row | ~none |
| 2 | Symbol/address resolver before m2c (no fabricated externs) | days | removes a whole cleanup class | low |
| 3 | `-da` dump diagnosis tool (directional "which variable/pass diverged") | weeks | the core thesis | medium — signal quality unproven |
| 4 | PERM-macro emitter driving unmodified permuter | days, after #3 | guided search, no fork | low |
| 5 | Per-diagnosis weight overrides | hours | trims wasted compiles | ~none |
| 6 | Outer loop driver (diagnose -> annotate -> permute -> repeat) | ~week | full automation glue | low |
| 7 | m2c context regeneration from diagnosis (structs/prototypes) | weeks | fixes type-class misses | medium |
| 8 | Version-specific idiom library as context | grows organically | better m2c starting points | low |
| 9 | Model fine-tuned on synthetic pairs from exact cc1 | months | highest ceiling | high |
| 10 | Own always-valid C emitter (replace m2c) | months | marginal — `--valid-syntax` covers most of it | high |
| 11 | Full pass-by-pass compiler inversion | research project | elegant but unneeded if #3–6 work | very high |

Sequencing logic: #1 first — it decides whether the dominant failure
class is regalloc (build #3), types (build #7), or invalid C
(reconsider #10). #2 is a no-regret quick win regardless. #3–6 form the
core product, in build order, each independently testable. #7–8 are the
"steer m2c" branch, only if the taxonomy says types dominate. #9 is the
long-game bet uniquely enabled by having the exact compiler. #10 and
#11 are explicitly parked: large, and mostly subsumed by cheaper rows.

## Taxonomy results (#1)

`tools/taxonomy_poc.py --batch` over all 1634 split functions with
target asm on disk: raw m2c `--valid-syntax` output, compiled with the
per-file toolchain (PSYQ/CC1/G annotations honored, pipefail on),
instruction streams compared with immediates normalized. Instruction
streams are stored in `build/taxonomy.jsonl`, so re-classification is
offline analysis — no recompilation.

| verdict | count | share |
|---|---|---|
| INSN_MATCH (instruction-exact) | 151 | 9.2% |
| STRUCTURAL, similarity >= 0.9 | 94 | 5.8% |
| STRUCTURAL, 0.7–0.9 | 348 | 21.3% |
| STRUCTURAL, < 0.7 | 511 | 31.3% |
| COMPILE_FAIL | 455 | 27.8% |
| M2C_FAIL (crash/timeout) | 75 | 4.6% |
| REGALLOC_ONLY / REGALLOC_FRAME | 0 | 0% |

Top COMPILE_FAIL causes: "too few arguments" (131), "conflicting
types" (68), "invalid type argument" (37), assignment type mismatches
(27) — i.e. m2c's inferred signatures/types disagreeing with the
project context, not invalid syntax.

Takeaways:

- **Raw m2c divergence is never pure register allocation.** Zero
  functions differ from target only by register naming or frame ops.
  The regalloc-focused predictor (idea #3 as originally framed)
  targets a failure mode that raw m2c output doesn't exhibit —
  regalloc presumably becomes the blocker only *after* shape/type
  problems are fixed. The #3 experiment should rerun on the
  human-perturbed-matched-function corpus before building anything.
- **The dominant machine-fixable class is context/type divergence**
  (~28% COMPILE_FAIL + a large share of STRUCTURAL): wrong signatures,
  wrong extern types, missing struct knowledge. This promotes the
  context-steering loop (#7) and the resolver (#2) above the regalloc
  tool in build order.
- ~9% of raw m2c output is instruction-exact with zero human input;
  ~6% more is within 0.9 similarity.
- Harness gotchas worth keeping: shell pipelines hide cc1 failures
  without pipefail; m2ctx's re-appended #defines collide with enum
  members when re-preprocessed; the ctx prototype of the function
  under test conflicts with m2c's inferred signature; `G=8` files
  need gp-relative compilation.
- Caveats: comparison is per-function instruction text, not linked
  bytes (no reloc/data checking); the corpus over-represents unmatched
  functions (stale asm for matched ones exists only for recent files,
  11 of 1634).

## PoC validation plan (for #3)

1. Dump parser: run `cc1 -O2 -G0 -da`, extract per-function
   pseudo->hard-reg map, callee-save set, frame size, pseudos live
   across calls, per-pass insn lists.
2. Target-side extractor: parse target asm for the same observable
   facts (prologue save set, `addiu $sp` constant, `$aN` -> `$sN`
   moves, regs live across `jal`s). No RTL lifting needed.
3. Comparator: align candidate pseudos to target registers by insn
   shape; report first divergence with direction.
4. Falsifiable test: take ~20 matched functions, mechanically perturb
   them the way humans break matches (hoist a duplicated load, reorder
   independent statements), check the tool names the actual edit.
5. The real measurement: raw m2c output for the same functions —
   matches reached in <=5 guided compiles vs the permuter's thousands.

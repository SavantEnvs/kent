# Finding: unbounded allocation from attacker-controlled blockCount (psl.c)

**Where:** `src/lib/psl.c` `pslLoadLm`/`pslLoad` — `blockCount = sqlUnsigned(row[17])`
is used directly as an allocation count: `lmAlloc(lm, sizeof(...) * blockCount)` for
`blockSizes`, `qStarts`, `tStarts` (three arrays), with no sanity bound against the
number of comma-separated values actually present.

**Impact:** A single PSL line declaring a huge `blockCount` (e.g. `4000000000`) makes
the loader try to allocate tens of GB before reading any block data — an out-of-memory
abort / denial of service on untrusted input. Reproduced as a libFuzzer `oom-*`
artifact (`rss_limit_mb` exceeded).

**Reproduce:**
```
/mayhem/fuzz_psl-standalone repro.psl      # or: /mayhem/fuzz_psl -rss_limit_mb=2560 repro.psl
```
`repro.psl` is a 21-column record with `blockCount = 4000000000`.

**One-line fix idea:** validate `blockCount` against a sane maximum (and against the
actual comma-count of the block arrays) before allocating in `pslLoad`/`pslLoadLm`.

Not guarded in the harness — OOMs are legitimate findings and must not be masked.

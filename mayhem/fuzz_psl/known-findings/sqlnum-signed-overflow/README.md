# Finding: signed integer overflow in kent's number parser (sqlNum.c)

**Where:** `src/lib/sqlNum.c:146` (the `x = x*10 + digit` accumulation loop used by
`sqlUnsigned`/`sqlSigned`), reached from `pslLoadLm`/`pslLoad` in `src/lib/psl.c`
while parsing any numeric PSL column.

**Impact:** A PSL field with more digits than fit in `int` overflows during parsing —
undefined behavior (`UndefinedBehaviorSanitizer: signed-integer-overflow`). kent
builds these tools with `-fno-sanitize-recover=all` off in production, so today it is
a silent wrong-value/UB rather than a controlled error; a hostile `.psl` gets a
garbage (implementation-defined) parsed value instead of a rejection.

**Reproduce:**
```
/mayhem/fuzz_psl-standalone repro.psl
```
`repro.psl` is a valid 21-column PSL record whose `match` field is `9999999999`
(> INT_MAX). Output:
```
lib/sqlNum.c:146:9: runtime error: signed integer overflow: 999999999 * 10 cannot be represented in type 'int'
```

**One-line fix idea:** parse into a 64-bit accumulator and range-check before
narrowing (or use `strtoul`/`strtol` with `errno`/`ERANGE` handling) in
`sqlNum.c`'s `sqlUnsigned`/`sqlSigned`.

Not guarded in the harness — this is a genuine parser robustness defect, exactly the
kind of finding fuzzing should surface.

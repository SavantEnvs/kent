#!/usr/bin/env bash
#
# mayhem/test.sh — behavioral oracle for the kent PSL parser.
#
# Runs the clean, dynamically-linked KAT probe (mayhem/kat_psl, built by build.sh)
# on one fixed canonical PSL record and asserts the EXACT parsed field values. This
# is a known-answer test: the neutered/no-op program (verify-repo's LD_PRELOAD
# sabotage shim _exit(0)s the probe before it prints) produces none of these values,
# so the greps miss and test.sh FAILS — which is what makes the oracle behavioral
# rather than a liveness/exit-code check. Does NOT compile anything (build.sh did).
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

KAT=/mayhem/kat_psl
if [ ! -x "$KAT" ]; then
  echo "test.sh: $KAT missing — build.sh did not produce the oracle probe" >&2
  exit 2
fi

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-${SRC:-.}/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

echo "== kent PSL parser known-answer test (mayhem/kat_psl) =="
out="$("$KAT" 2>&1)" || { echo "kat_psl exited non-zero:"; printf '%s\n' "$out"; emit_ctrf "kent-psl-kat" 0 1; exit 1; }
printf '%s\n' "$out"

# Each expected line is a field the parser must recover from the fixed PSL record.
# (values hand-computed from the record embedded in mayhem/kat_psl.c)
expected=(
  "match=30"
  "misMatch=2"
  "qNumInsert=1"
  "tBaseInsert=10"
  "strand=+"
  "qName=qSeq"
  "qSize=100"
  "tName=chr1"
  "tStart=200"
  "tEnd=242"
  "blockCount=2"
  "blockSize0=20"
  "blockSize1=10"
  "tStart0=200"
  "tStart1=232"
)

passed=0; failed=0
for e in "${expected[@]}"; do
  if printf '%s\n' "$out" | grep -qxF "$e"; then
    passed=$(( passed + 1 ))
  else
    echo "MISSING expected value: $e" >&2
    failed=$(( failed + 1 ))
  fi
done

emit_ctrf "kent-psl-kat" "$passed" "$failed"

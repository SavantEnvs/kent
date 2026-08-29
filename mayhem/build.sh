#!/usr/bin/env bash
#
# mayhem/build.sh — build the kent PSL-parser fuzz harness (sanitized + coverage-
# instrumented), a standalone reproducer, and a clean KAT-probe oracle binary.
# Runs inside the commit image as `mayhem` in /mayhem ($SRC=/mayhem).
#
# Fully self-contained / air-gappable: everything is compiled from the in-tree
# kent sources against libraries already in the org base image (OpenSSL/zlib/
# pthread/libm). kent bundles htslib as a git submodule and its core lib also
# reaches udc->net->https; the in-memory harness (lineFileOnString) never takes
# the tabix or URL code paths, so those are satisfied by tiny compile shims under
# mayhem/ (htslib_stub.*, udc_stub.c) rather than pulling in htslib/libcurl. No
# network, no package install -> the offline PATCH-tier re-run succeeds.
set -euo pipefail

# clang rejects SOURCE_DATE_EPOCH='' (empty) — must be unset or a valid integer.
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -g}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}"
: "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
: "${MAYHEM_JOBS:=$(nproc)}"
export SANITIZER_FLAGS DEBUG_FLAGS CC LIB_FUZZING_ENGINE STANDALONE_FUZZ_MAIN MAYHEM_JOBS

MAY="$SRC/mayhem"
KSRC="$SRC/src"

# kent's build flags for the core "jkweb" library (from src/inc/common.mk):
#   -D_GNU_SOURCE gives the `uint` type the headers use; the rest are its size/arch defs.
DEFS="-D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -DMACHTYPE_x86_64"
INC="-I$KSRC/inc -I$KSRC/lib -I$MAY/htslib_stub"
CSTD="-std=c99 -fno-strict-aliasing -w"
SYSLIBS="-lssl -lcrypto -lm -lz -lpthread"
# Coverage instrumentation for the FUZZED library — appended UNCONDITIONALLY (even
# when SANITIZER_FLAGS is empty) so the library records edges, not just the harness TU.
COVFLAG="-fsanitize=fuzzer-no-link"

export CC CSTD DEFS INC

# Files in src/lib excluded from our build: they need external deps we do not ship
# (bamFile/vcf -> htslib, curlWrap -> libcurl, hfileUdc -> htslib, uuid -> libuuid,
# pngwrite -> libpng, oswin9x -> Windows) or are replaced by a stub (udc). None are
# in the PSL parser's link closure; the linker pulls only what fuzz_psl/kat_psl need.
EXCLUDE="udc bamFile curlWrap hfileUdc oswin9x pngwrite uuid vcf"
is_excluded(){ case " $EXCLUDE " in *" $1 "*) return 0;; esac; return 1; }

# Compile every non-excluded src/lib/*.c plus the two stubs into <outdir>, then archive.
# $1=outdir  $2=extra compile flags  $3=output archive
build_archive(){
  local outdir="$1" extra="$2" arout="$3" c b
  rm -rf "$outdir"; mkdir -p "$outdir"
  export _OUTDIR="$outdir" _EXTRA="$extra"
  for c in "$KSRC"/lib/*.c; do
    b=$(basename "${c%.c}")
    is_excluded "$b" && continue
    printf '%s\n' "$c"
  done | xargs -P "$MAYHEM_JOBS" -n1 sh -c \
    '$CC $CSTD $DEFS $INC $_EXTRA -c "$0" -o "$_OUTDIR/$(basename "${0%.c}").o"'
  $CC $CSTD $DEFS $INC $extra -c "$MAY/htslib_stub.c" -o "$outdir/htslib_stub.o"
  $CC $CSTD $DEFS $INC $extra -c "$MAY/udc_stub.c"    -o "$outdir/udc_stub.o"
  rm -f "$arout"; ar rcs "$arout" "$outdir"/*.o
}

echo "build.sh: [1/3] sanitized + instrumented jkweb archive"
build_archive /tmp/jkweb-san "$SANITIZER_FLAGS $DEBUG_FLAGS $COVFLAG" /tmp/libjkweb_san.a

echo "build.sh: [2/3] fuzz target + standalone reproducer"
# In-process libFuzzer target over pslNext()/pslLoadLm() (mayhem/fuzz_psl.c).
# shellcheck disable=SC2086
$CC $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE $DEFS $INC \
    "$MAY/fuzz_psl.c" /tmp/libjkweb_san.a $SYSLIBS -o /mayhem/fuzz_psl
# Standalone (non-libFuzzer) run-once reproducer for triage of a saved artifact.
# shellcheck disable=SC2086
$CC $SANITIZER_FLAGS $DEBUG_FLAGS $DEFS $INC \
    "$STANDALONE_FUZZ_MAIN" "$MAY/fuzz_psl.c" /tmp/libjkweb_san.a $SYSLIBS \
    -o /mayhem/fuzz_psl-standalone

echo "build.sh: [3/3] clean oracle archive + KAT probe (no sanitizer, no -gdwarf-3)"
build_archive /tmp/jkweb-clean "-O2 -g" /tmp/libjkweb.a
# shellcheck disable=SC2086
$CC -O2 -g $DEFS $INC "$MAY/kat_psl.c" /tmp/libjkweb.a $SYSLIBS -o /mayhem/kat_psl
# The oracle MUST be dynamically linked so verify-repo's LD_PRELOAD sabotage shim
# can neuter it — assert that here so a regression fails the build, not the gate.
file /mayhem/kat_psl | grep -q 'dynamically linked' \
  || { echo "build.sh: ERROR kat_psl is not dynamically linked" >&2; exit 1; }

echo "build.sh: done — /mayhem/fuzz_psl, /mayhem/fuzz_psl-standalone, /mayhem/kat_psl"

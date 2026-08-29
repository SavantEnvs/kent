# syntax=docker/dockerfile:1.7
#
# Commit-image Dockerfile (SMALL form) for kent — builds ON TOP of the org C/C++
# base image, which already provides debian + clang/llvm-19 + libFuzzer + the
# identities + the build-contract ENV (SANITIZER_FLAGS/CC/LIB_FUZZING_ENGINE/...).
#
# Build (from the repo root):
#   DOCKER_BUILDKIT=1 docker build -f mayhem/Dockerfile \
#       --build-arg COMMIT=$(git rev-parse HEAD) -t kent-commit:test .

FROM ghcr.io/savantenvs/base:20260621

ARG COMMIT
ARG REPO_URL
# SOURCE_DATE_EPOCH stays an ARG (not ENV): an empty ENV breaks clang. build.sh unsets it when empty.
ARG SOURCE_DATE_EPOCH
# Sanitizer override (undefaulted, so the base ENV flows through unless --build-arg given).
ARG SANITIZER_FLAGS
# Build parallelism (optional; build.sh falls back to nproc when unset).
ARG MAYHEM_JOBS=

# Source enters from the build CONTEXT (the CI checkout, incl. full .git history)
# and is copied to /mayhem ($SRC). build.sh compiles the kent PSL parser + harness
# entirely from these in-tree sources against libraries already in the base image
# (OpenSSL/zlib/pthread/libm) — no apt install, no network, so the offline PATCH-tier
# re-run of build.sh succeeds unchanged. The htslib/zlib-ng git submodules are NOT
# needed (the tabix/bgzf path is stubbed under mayhem/), so the image does not fetch them.
USER mayhem
WORKDIR /mayhem
COPY --chown=2000:2000 . /mayhem

RUN bash mayhem/build.sh

# Run the functional oracle at build time (a failing suite fails the image build).
RUN if [ -f mayhem/test.sh ]; then bash mayhem/test.sh; else echo "no mayhem/test.sh"; fi

LABEL org.opencontainers.image.source="${REPO_URL}" \
      org.opencontainers.image.revision="${COMMIT}" \
      rlenv.tier="commit" \
      rlenv.source_date_epoch="${SOURCE_DATE_EPOCH}" \
      rlenv.user="mayhem"

USER mayhem
WORKDIR /mayhem

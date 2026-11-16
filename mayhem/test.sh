#!/usr/bin/env bash
#
# mayhem/test.sh — pg_auto_failover's functional tests (everything built by mayhem/build.sh):
#  1. upstream's monitor suite: `make -C src/monitor installcheck` (21 pg_regress tests from
#     src/monitor/regress_schedule + 6 isolation specs) in a throwaway PostgreSQL 17 cluster started
#     by pg_virtualenv with shared_preload_libraries=pgautofailover, as upstream's Dockerfile does.
#     Its TAP `ok`/`not ok` lines are counted; any failure (or a suite that never ran) fails.
#  2. mayhem/build/kat (src/bin/common compiled with normal flags; the monitor suite never touches
#     that code), whose output is compared line by line with the golden values below; a no-op or
#     exit(0) program prints nothing and fails.
# Upstream's other suites are not run: tests/ (Python) needs sudo, network namespaces and several
# nodes, and pgaftest drives Docker containers.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
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

GOLDEN=$(cat <<'EOF_GOLDEN'
pg_version 17.4 ok=1 value=1704
pg_version 14 ok=1 value=1400
pg_version 1.2.3 ok=0
pg_ctl_version ok=1 string=10.3 value=1003
extension_version ok=1 value=201
bool on ok=1 value=1
bool No ok=1 value=0
bool maybe ok=0
lsn 16/B374D848 ok=1 value=16b374d848
lsn 0/3000060 ok=1 value=3000060
lsn 123456789/0 ok=0
controldata ok=1 control=1700 catalog=202406281 sysid=7412345678901234567 tli=2 checkpoint=0/3000060 redo=0/3000028 minrecovery=0/0
scrub ok=1 uri=postgres://autoctl@monitor.example.com:5432/pg_auto_failover?password=****&sslmode=require
ssl ok=1 mode=verify-full ca=/ca.crt
notification ok=1 formation=default node=3 name=node_3 host=db3.example.com port=5433 reported=secondary goal=primary health=1
nodes ok=1 count=2
node id=1 name=a host=h1 port=5432 lsn=0/3000060 primary=1
node id=2 name=b host=h2 port=5433 lsn=1/A0 primary=0
timeline ok=1 count=3
tli=1 begin=0 end=3000060
tli=2 begin=3000060 end=50000a8
tli=3 begin=50000a8 end=0
ini ok=1 name=keeper-a pgdata=/var/lib/pg port=5433
EOF_GOLDEN
)

KAT="$SRC/mayhem/build/kat"
if [ ! -x "$KAT" ]; then
  echo "missing $KAT: mayhem/build.sh did not produce the KAT probe" >&2
  exit 1
fi
if [ ! -f "$(pg_config --pkglibdir)/pgautofailover.so" ] || ! command -v pg_virtualenv >/dev/null; then
  echo "missing the installed pgautofailover extension or pg_virtualenv: mayhem/build.sh / Dockerfile bug" >&2
  exit 1
fi

# --- 1. upstream monitor suite (pg_regress + isolation) ---
REG_LOG=$(mktemp)
pg_virtualenv -v 17 -o shared_preload_libraries=pgautofailover \
  make -C "$SRC/src/monitor" installcheck >"$REG_LOG" 2>&1
reg_rc=$?
cat "$REG_LOG"
reg_passed=$(grep -cE '^ok [0-9]+ ' "$REG_LOG")
reg_failed=$(grep -cE '^not ok [0-9]+ ' "$REG_LOG")
rm -f "$REG_LOG"
# 21 schedule tests + 6 isolation specs; a crash or an early stop must not look like a pass
REG_EXPECTED=27
if [ "$reg_rc" -ne 0 ] && [ "$reg_failed" -eq 0 ]; then reg_failed=1; fi
if [ $((reg_passed + reg_failed)) -lt "$REG_EXPECTED" ]; then
  echo "monitor suite reported $((reg_passed + reg_failed)) of $REG_EXPECTED tests" >&2
  reg_failed=$((REG_EXPECTED - reg_passed))
fi
echo "monitor suite (pg_regress): passed=$reg_passed failed=$reg_failed"

# --- 2. known-answer probe for src/bin/common ---
ACTUAL=$("$KAT" 2>/dev/null || true)

mapfile -t want <<<"$GOLDEN"
mapfile -t got <<<"$ACTUAL"
kat_passed=0 kat_failed=0
for i in "${!want[@]}"; do
  if [ "${got[$i]:-}" = "${want[$i]}" ]; then
    kat_passed=$((kat_passed + 1))
  else
    kat_failed=$((kat_failed + 1))
    echo "FAIL line $((i + 1)): want '${want[$i]}' got '${got[$i]:-<missing>}'" >&2
  fi
done
# extra output lines are also a mismatch
if [ "${#got[@]}" -gt "${#want[@]}" ]; then
  kat_failed=$((kat_failed + ${#got[@]} - ${#want[@]}))
fi
echo "known-answer probe (kat): passed=$kat_passed failed=$kat_failed"

emit_ctrf "pg_regress+pg_auto_failover-kat" $((reg_passed + kat_passed)) $((reg_failed + kat_failed)) 0

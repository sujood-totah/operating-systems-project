#!/usr/bin/env bash
# Self-test runner for milestones 1–5.
# Run from repo root: bash tests/run_tests.sh
#
# Milestone coverage:
#   M1 — stdout compared to tests/expected/*.out
#   M2–M3 — build + brief headless launch (no hang)
#   M4 — fork children print [PID=…] started; timeout before GUI blocks
#   M5 — IPC parent logs (needs xvfb-run + xdotool to auto-click PLAY)

set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

LOG_DIR="$ROOT/tests/logs"
EXPECTED_DIR="$ROOT/tests/expected"
INPUT_DIR="$ROOT/inputs/tests"

mkdir -p "$LOG_DIR"

PASS=0
FAIL=0
SKIP=0

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

log()  { printf '%s\n' "$*"; }
pass() { PASS=$((PASS + 1)); printf "${GREEN}PASS${NC} %s\n" "$1"; }
fail() { FAIL=$((FAIL + 1)); printf "${RED}FAIL${NC} %s\n" "$1"; }
skip() { SKIP=$((SKIP + 1)); printf "${YELLOW}SKIP${NC} %s\n" "$1"; }

have_cmd() { command -v "$1" >/dev/null 2>&1; }

# Run ./sim under xvfb when available; otherwise run directly (needs DISPLAY).
run_sim_headless() {
    local timeout_sec="$1"
    local input_file="$2"
    local log_file="$3"
    shift 3

    if have_cmd timeout; then
        local run=(timeout "$timeout_sec")
    else
        local run=()
        log "warning: 'timeout' not found; GUI test may hang on failure"
    fi

    if have_cmd xvfb-run; then
        "${run[@]}" xvfb-run -a ./sim "$input_file" "$@" >"$log_file" 2>&1
    else
        "${run[@]}" ./sim "$input_file" "$@" >"$log_file" 2>&1
    fi
}

# --- Build all milestones ---
log "=== Clean and build ==="
if ! make clean >"$LOG_DIR/make_clean.log" 2>&1; then
    fail "make clean (see $LOG_DIR/make_clean.log)"
    exit 1
fi

for target in milestone1 milestone2 milestone3 milestone4 milestone5; do
    if make "$target" >"$LOG_DIR/build_${target}.log" 2>&1; then
        pass "build $target"
    else
        fail "build $target (see $LOG_DIR/build_${target}.log)"
    fi
done

# --- Milestone 1: CLI output comparison ---
log ""
log "=== Milestone 1 (CLI) ==="

run_m1_compare() {
    local name="$1"
    local input_file="$2"
    local expected_file="$3"
    local out_file="$LOG_DIR/m1_${name}.out"

    if [ ! -x ./dijkstra ]; then
        skip "m1 $name (dijkstra not built)"
        return
    fi

    ./dijkstra "$input_file" >"$out_file" 2>&1
    local rc=$?

  case "$name" in
        negative_weight|no_args)
            if [ "$rc" -ne 1 ]; then
                fail "m1 $name (expected exit 1, got $rc)"
                return
            fi
            ;;
        *)
            if [ "$rc" -ne 0 ]; then
                fail "m1 $name (exit $rc, see $out_file)"
                return
            fi
            ;;
    esac

    if diff -u \
        <(tr -d '\r' <"$expected_file") \
        <(tr -d '\r' <"$out_file") \
        >"$LOG_DIR/m1_${name}.diff" 2>&1; then
        pass "m1 $name"
    else
        fail "m1 $name (diff in $LOG_DIR/m1_${name}.diff)"
    fi
}

# Requirement: exact shortest path and distance
run_m1_compare "shortest_path" "$INPUT_DIR/m1_shortest_path.txt" \
    "$EXPECTED_DIR/m1_shortest_path.out"
# Requirement: No path found
run_m1_compare "no_path" "$INPUT_DIR/m1_no_path.txt" \
    "$EXPECTED_DIR/m1_no_path.out"
# Requirement: source == destination → node then 0
run_m1_compare "same_source_dest" "$INPUT_DIR/m1_same_source_dest.txt" \
    "$EXPECTED_DIR/m1_same_source_dest.out"
# Requirement: negative weights → erro
run_m1_compare "negative_weight" "$INPUT_DIR/m1_negative_weight.txt" \
    "$EXPECTED_DIR/m1_negative_weight.out"
# Requirement: course-style input (last line source dest, no traveler count)
run_m1_compare "example1_path" "$INPUT_DIR/m1_example1_path.txt" \
    "$EXPECTED_DIR/m1_example1_path.out"

# Requirement: missing argument → Invalid input
if [ -x ./dijkstra ]; then
    ./dijkstra >"$LOG_DIR/m1_no_args.out" 2>&1
    rc=$?
    if [ "$rc" -eq 1 ] && diff -u \
        <(tr -d '\r' <"$EXPECTED_DIR/m1_negative_weight.out") \
        <(tr -d '\r' <"$LOG_DIR/m1_no_args.out") >/dev/null 2>&1; then
        pass "m1 no_args"
    else
        fail "m1 no_args (see $LOG_DIR/m1_no_args.out)"
    fi
fi

# --- Milestones 2–3: smoke launch (rebuild sim; last target was milestone5) ---
log ""
log "=== Milestones 2–3 (GUI build + smoke) ==="

if ! make milestone3 >"$LOG_DIR/rebuild_milestone3.log" 2>&1; then
    fail "rebuild milestone3 for m2/m3 smoke (see $LOG_DIR/rebuild_milestone3.log)"
elif [ ! -x ./sim ]; then
    skip "m2/m3 smoke (sim not built)"
else
    # M2: max 15 nodes loads; M3: single traveler file
    for tag in m2_max_nodes m3_single_traveler; do
        logfile="$LOG_DIR/${tag}_smoke.log"
        set +e
        run_sim_headless 5 "$INPUT_DIR/${tag}.txt" "$logfile"
        rc=$?
        set -e
        # timeout returns 124; window close / kill may return other codes
        if [ "$rc" -eq 124 ] || [ "$rc" -eq 0 ] || [ "$rc" -eq 137 ] || [ "$rc" -eq 143 ]; then
            if grep -qi 'Invalid input\|No path found\|fatal error\|Segmentation fault' "$logfile"; then
                fail "${tag} smoke (error in $logfile)"
            else
                pass "${tag} smoke launch"
            fi
        else
            fail "${tag} smoke (exit $rc, see $logfile)"
        fi
    done
fi

# --- Milestone 4: children print started ---
log ""
log "=== Milestone 4 (fork + started lines) ==="

if ! make milestone4 >"$LOG_DIR/rebuild_milestone4.log" 2>&1; then
    fail "rebuild milestone4 (see $LOG_DIR/rebuild_milestone4.log)"
elif [ ! -x ./sim ]; then
    skip "m4 started lines (sim not built)"
else
    logfile="$LOG_DIR/m4_multi_traveler.log"
    set +e
    run_sim_headless 8 "$INPUT_DIR/m4_multi_traveler.txt" "$logfile"
    set -e

    started_count=$(grep -c '\[PID=[0-9]*\] started' "$logfile" 2>/dev/null || true)
    if [ "${started_count:-0}" -ge 2 ]; then
        pass "m4 two [PID=…] started lines"
    else
        fail "m4 started lines (found ${started_count:-0}, want ≥2; see $logfile)"
    fi
fi

# --- Milestone 5: IPC parent log format ---
log ""
log "=== Milestone 5 (IPC logs) ==="

if ! have_cmd xvfb-run; then
    skip "m5 IPC logs (install xvfb-run for headless GUI)"
elif ! have_cmd xdotool; then
    skip "m5 IPC logs (install xdotool to auto-click PLAY; see tests/CHECKLIST.md)"
elif ! make milestone5 >"$LOG_DIR/rebuild_milestone5.log" 2>&1; then
    fail "rebuild milestone5 (see $LOG_DIR/rebuild_milestone5.log)"
elif [ ! -x ./sim ]; then
    skip "m5 IPC logs (sim not built)"
else
    logfile="$LOG_DIR/m5_ipc_fast.log"
    set +e
    if have_cmd timeout; then
        timeout 20 xvfb-run -a bash -c "
            cd '$ROOT' &&
            ./sim '$INPUT_DIR/m5_ipc_fast.txt' >'$logfile' 2>&1 &
            sim_pid=\$!
            sleep 2
            win=\$(xdotool search --name 'Graph GUI' 2>/dev/null | head -n1)
            if [ -n \"\$win\" ]; then
                xdotool windowactivate --sync \"\$win\" mousemove --window \"\$win\" 80 40 click 1
            fi
            sleep 6
            kill \"\$sim_pid\" 2>/dev/null || true
            wait \"\$sim_pid\" 2>/dev/null || true
        "
    else
        xvfb-run -a bash -c "
            cd '$ROOT' &&
            ./sim '$INPUT_DIR/m5_ipc_fast.txt' >'$logfile' 2>&1 &
            sim_pid=\$!
            sleep 2
            win=\$(xdotool search --name 'Graph GUI' 2>/dev/null | head -n1)
            if [ -n \"\$win\" ]; then
                xdotool windowactivate --sync \"\$win\" mousemove --window \"\$win\" 80 40 click 1
            fi
            sleep 6
            kill \"\$sim_pid\" 2>/dev/null || true
            wait \"\$sim_pid\" 2>/dev/null || true
        "
    fi
    set -e

    m5_ok=1
    for pattern in 'arrived at node' 'next node:' 'DESTINATION' 'finished'; do
        if ! grep -q "$pattern" "$logfile"; then
            fail "m5 missing log pattern: $pattern (see $logfile)"
            m5_ok=0
        fi
    done
    if [ "$m5_ok" -eq 1 ]; then
        pass "m5 IPC log patterns"
    fi
fi

# --- Summary ---
log ""
log "=== Summary ==="
log "Passed: $PASS  Failed: $FAIL  Skipped: $SKIP"
log "Logs: $LOG_DIR/"

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
exit 0

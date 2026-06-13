#!/usr/bin/env bash
# Professor-like strict grader for Operating Systems graph simulation (Milestones 1–6).
# Run from repo root: bash tests/professor_like/run_professor_like_tests.sh

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
if [ -f "$SCRIPT_DIR/../../Makefile" ]; then
    ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
else
    ROOT="$(pwd)"
fi
cd "$ROOT"

PROF_DIR="$ROOT/tests/professor_like"
INPUT_DIR="$PROF_DIR/inputs"
EXPECTED_DIR="$PROF_DIR/expected"
LOG_DIR="$PROF_DIR/logs"

mkdir -p "$LOG_DIR"

PASS=0
FAIL=0
SKIP=0
TOTAL=0

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

log()  { printf '%s\n' "$*"; }
pass() { TOTAL=$((TOTAL + 1)); PASS=$((PASS + 1)); printf "${GREEN}PASS${NC} %s\n" "$1"; }
fail() { TOTAL=$((TOTAL + 1)); FAIL=$((FAIL + 1)); printf "${RED}FAIL${NC} %s\n" "$1"; }
skip() { TOTAL=$((TOTAL + 1)); SKIP=$((SKIP + 1)); printf "${YELLOW}SKIP${NC} %s\n" "$1"; }

have_cmd() { command -v "$1" >/dev/null 2>&1; }

run_sim_headless() {
    local timeout_sec="$1"
    local input_file="$2"
    local log_file="$3"

    if have_cmd timeout; then
        local run=(timeout "$timeout_sec")
    else
        local run=()
    fi

    if have_cmd xvfb-run; then
        "${run[@]}" xvfb-run -a ./sim "$input_file" >"$log_file" 2>&1
    else
        "${run[@]}" ./sim "$input_file" >"$log_file" 2>&1
    fi
}

run_sim_with_play_click() {
    local timeout_sec="$1"
    local input_file="$2"
    local log_file="$3"

    if ! have_cmd xvfb-run; then
        return 2
    fi

    if have_cmd timeout; then
        timeout "$timeout_sec" xvfb-run -a bash -c "
            cd '$ROOT' &&
            ./sim '$input_file' >'$log_file' 2>&1 &
            sim_pid=\$!
            sleep 2
            if command -v xdotool >/dev/null 2>&1; then
                win=\$(xdotool search --name 'Graph GUI' 2>/dev/null | head -n1)
                if [ -n \"\$win\" ]; then
                    xdotool windowactivate --sync \"\$win\" mousemove --window \"\$win\" 80 40 click 1
                fi
            fi
            sleep 8
            kill \"\$sim_pid\" 2>/dev/null || true
            wait \"\$sim_pid\" 2>/dev/null || true
        "
    else
        xvfb-run -a bash -c "
            cd '$ROOT' &&
            ./sim '$input_file' >'$log_file' 2>&1 &
            sim_pid=\$!
            sleep 2
            if command -v xdotool >/dev/null 2>&1; then
                win=\$(xdotool search --name 'Graph GUI' 2>/dev/null | head -n1)
                if [ -n \"\$win\" ]; then
                    xdotool windowactivate --sync \"\$win\" mousemove --window \"\$win\" 80 40 click 1
                fi
            fi
            sleep 8
            kill \"\$sim_pid\" 2>/dev/null || true
            wait \"\$sim_pid\" 2>/dev/null || true
        "
    fi
}

smoke_ok_rc() {
    local rc=$1
    [ "$rc" -eq 0 ] || [ "$rc" -eq 124 ] || [ "$rc" -eq 137 ] || [ "$rc" -eq 143 ]
}

log_crash_in_file() {
    local log_file="$1"
    grep -qiE 'Segmentation fault|Aborted|fatal error|core dumped|stack smashing' "$log_file"
}

# ---------------------------------------------------------------------------
log "=== General repository checks ==="

[ -f README.md ] && pass "repo README.md exists" || fail "repo README.md missing"
[ -f Makefile ] && pass "repo Makefile exists" || fail "repo Makefile missing"

for target in milestone1 milestone2 milestone3 milestone4 milestone5 milestone6 clean; do
    if grep -qE "^${target}:|^${target} " Makefile || \
       grep -qE "^milestone[0-9]+ ${target}:" Makefile || \
       grep -qE "^milestone[0-9]+ ${target} " Makefile; then
        pass "Makefile target: $target"
    else
        fail "Makefile missing target: $target"
    fi
done

if grep -q 'all:.*milestone6' Makefile; then
    pass "make all includes milestone6"
else
    fail "make all does not include milestone6"
fi

for tag in milestone1 milestone2 milestone3 milestone4 milestone5 milestone6; do
    if git tag -l "$tag" | grep -qx "$tag"; then
        pass "git tag exists: $tag"
    else
        fail "git tag missing: $tag"
    fi
done

readme_checks=(
    "milestone1:Milestone 1"
    "milestone2:milestone2"
    "milestone3:milestone3"
    "milestone4:Milestone 4"
    "milestone5:Milestone 5"
    "milestone6:Milestone 6"
    "IPC:pipe"
    "sync:semaphore"
)
for item in "${readme_checks[@]}"; do
    key="${item%%:*}"
    pattern="${item##*:}"
    if grep -qi "$pattern" README.md; then
        pass "README mentions: $key ($pattern)"
    else
        fail "README missing: $key (pattern '$pattern')"
    fi
done

if grep -q 'builds milestones 1-6' README.md; then
    pass "README make all documents milestones 1-6"
else
    fail "README make all text does not say milestones 1-6"
fi

# Forbidden / allowed library scan
log ""
log "=== Forbidden-library scan (.c / .h) ==="

OS_HEADERS="unistd.h sys/wait.h sys/types.h signal.h fcntl.h sys/ipc.h sys/shm.h semaphore.h sys/stat.h"
ALLOWED_PROJECT="raylib.h graph.h graph_io.h dijkstra.h"
FORBIDDEN_FOUND=0

while IFS= read -r inc; do
    hdr="${inc#<}"
    hdr="${hdr%>}"
    hdr="${hdr#\"}"
    hdr="${hdr%\"}"
    hdr="$(printf '%s' "$hdr" | tr -d '\r')"

    case " $OS_HEADERS " in
        *" $hdr "*) continue ;;
    esac
    case " $ALLOWED_PROJECT " in
        *" $hdr "*) continue ;;
    esac
    case "$hdr" in
        stdio.h|stdlib.h|string.h|math.h|limits.h|errno.h|stddef.h|stdint.h|stdbool.h)
            continue ;;
    esac
    fail "forbidden or unexpected header: $hdr (in $inc)"
    FORBIDDEN_FOUND=1
done < <(grep -Rh '^#include' src --include='*.c' --include='*.h' | sed -E 's/^#include[[:space:]]+//' | tr -d '\r' | sort -u)

if [ "$FORBIDDEN_FOUND" -eq 0 ]; then
    pass "headers limited to standard C, raylib, project headers, and OS/POSIX"
    for oh in $OS_HEADERS; do
        if grep -Rql "$oh" src --include='*.c' --include='*.h' 2>/dev/null; then
            log "  info: OS header used for later milestones: $oh"
        fi
    done
fi

# ---------------------------------------------------------------------------
log ""
log "=== Milestone 1 (CLI exact output) ==="

if make milestone1 >"$LOG_DIR/build_milestone1.log" 2>&1; then
    pass "build milestone1"
else
    fail "build milestone1 (see $LOG_DIR/build_milestone1.log)"
fi

run_m1_compare() {
    local name="$1"
    local input_file="$2"
    local expected_file="$3"
    local expect_rc="${4:-0}"
    local out_file="$LOG_DIR/m1_${name}.out"

    if [ ! -x ./dijkstra ]; then
        skip "m1 $name (dijkstra not built)"
        return
    fi

    ./dijkstra "$input_file" >"$out_file" 2>&1
    local rc=$?

    if [ "$rc" -ne "$expect_rc" ]; then
        fail "m1 $name (expected exit $expect_rc, got $rc; see $out_file)"
        return
    fi

    if diff -u \
        <(tr -d '\r' <"$expected_file") \
        <(tr -d '\r' <"$out_file") \
        >"$LOG_DIR/m1_${name}.diff" 2>&1; then
        pass "m1 $name exact output"
    else
        fail "m1 $name output mismatch (see $LOG_DIR/m1_${name}.diff)"
    fi
}

run_m1_compare "normal_shortest" "$INPUT_DIR/m1_normal_shortest.txt" "$EXPECTED_DIR/m1_normal_shortest.out" 0
run_m1_compare "alternate_route" "$INPUT_DIR/m1_alternate_route.txt" "$EXPECTED_DIR/m1_alternate_route.out" 0
run_m1_compare "disconnected" "$INPUT_DIR/m1_disconnected.txt" "$EXPECTED_DIR/m1_disconnected.out" 0
run_m1_compare "negative_weight" "$INPUT_DIR/m1_negative_weight.txt" "$EXPECTED_DIR/m1_negative_weight.out" 1
run_m1_compare "same_source_dest" "$INPUT_DIR/m1_same_source_dest.txt" "$EXPECTED_DIR/m1_same_source_dest.out" 0
run_m1_compare "larger_graph" "$INPUT_DIR/m1_larger_graph.txt" "$EXPECTED_DIR/m1_larger_graph.out" 0
run_m1_compare "invalid_format" "$INPUT_DIR/m1_invalid_format.txt" "$EXPECTED_DIR/m1_invalid_format.out" 1
run_m1_compare "blank_lines_comments" "$INPUT_DIR/m1_blank_lines_comments.txt" "$EXPECTED_DIR/m1_blank_lines_comments.out" 1

if [ -x ./dijkstra ]; then
    ./dijkstra >"$LOG_DIR/m1_no_args.out" 2>&1
    rc=$?
    if [ "$rc" -eq 1 ] && grep -qx 'Invalid input' "$LOG_DIR/m1_no_args.out"; then
        pass "m1 no arguments prints Invalid input"
    else
        fail "m1 no arguments (exit $rc; see $LOG_DIR/m1_no_args.out)"
    fi
fi

# Valgrind
log ""
log "=== Milestone 1 valgrind ==="
if ! have_cmd valgrind; then
    skip "m1 valgrind (valgrind not installed)"
elif [ ! -x ./dijkstra ]; then
    skip "m1 valgrind (dijkstra not built)"
else
    valgrind --leak-check=full --error-exitcode=99 \
        ./dijkstra "$INPUT_DIR/m1_normal_shortest.txt" \
        >"$LOG_DIR/m1_valgrind.out" 2>&1
    vg_rc=$?
    if grep -qiE 'Invalid read|Invalid write' "$LOG_DIR/m1_valgrind.out"; then
        fail "m1 valgrind invalid memory access (see $LOG_DIR/m1_valgrind.out)"
    elif grep -qE 'definitely lost: [1-9][0-9]* bytes' "$LOG_DIR/m1_valgrind.out"; then
        fail "m1 valgrind definitely lost bytes (see $LOG_DIR/m1_valgrind.out)"
    elif grep -qE 'possibly lost: [1-9][0-9]* bytes' "$LOG_DIR/m1_valgrind.out"; then
        fail "m1 valgrind possibly lost bytes (see $LOG_DIR/m1_valgrind.out)"
    elif [ "$vg_rc" -eq 99 ]; then
        fail "m1 valgrind error-exitcode 99 (see $LOG_DIR/m1_valgrind.out)"
    else
        pass "m1 valgrind clean"
    fi
fi

# ---------------------------------------------------------------------------
log ""
log "=== Milestones 2–3 (GUI smoke) ==="

if make milestone2 >"$LOG_DIR/build_milestone2.log" 2>&1; then
    pass "build milestone2"
else
    fail "build milestone2 (see $LOG_DIR/build_milestone2.log)"
fi

if [ -x ./sim ]; then
    logfile="$LOG_DIR/m2_smoke.log"
    set +e
    run_sim_headless 7 "$INPUT_DIR/m2_smoke_max_nodes.txt" "$logfile"
    rc=$?
    set -e
    if smoke_ok_rc "$rc" && ! log_crash_in_file "$logfile"; then
        pass "m2 smoke (xvfb/timeout 7s, no crash)"
    else
        fail "m2 smoke (exit $rc; see $logfile)"
    fi
else
    skip "m2 smoke (sim not built)"
fi

if make milestone3 >"$LOG_DIR/build_milestone3.log" 2>&1; then
    pass "build milestone3"
else
    fail "build milestone3 (see $LOG_DIR/build_milestone3.log)"
fi

if [ -x ./sim ]; then
    logfile="$LOG_DIR/m3_smoke.log"
    set +e
    run_sim_headless 7 "$INPUT_DIR/m3_smoke_single_traveler.txt" "$logfile"
    rc=$?
    set -e
    if smoke_ok_rc "$rc" && ! log_crash_in_file "$logfile"; then
        pass "m3 smoke (xvfb/timeout 7s, no crash)"
    else
        fail "m3 smoke (exit $rc; see $logfile)"
    fi
else
    skip "m3 smoke (sim not built)"
fi

# ---------------------------------------------------------------------------
log ""
log "=== Milestone 4 (fork + started lines) ==="

if make milestone4 >"$LOG_DIR/build_milestone4.log" 2>&1; then
    pass "build milestone4"
else
    fail "build milestone4 (see $LOG_DIR/build_milestone4.log)"
fi

if [ -x ./sim ]; then
    logfile="$LOG_DIR/m4_multi_traveler.log"
    traveler_count=$(awk 'NR>1 && prev_edges>0 {print; exit} {if(NF==2 && $1 ~ /^[0-9]+$/ && $2 ~ /^[0-9]+$/ && prev_edges==0) next; if(NF==1 && $1 ~ /^[0-9]+$/) {tc=$1; prev_edges=1; next}} END{print tc+0}' "$INPUT_DIR/m4_multi_traveler.txt" 2>/dev/null || echo 2)
    # simpler: hardcode from file structure
    traveler_count=2

    set +e
    run_sim_headless 10 "$INPUT_DIR/m4_multi_traveler.txt" "$logfile"
    rc=$?
    set -e

    started_count=$(grep -c '\[PID=[0-9]*\] started' "$logfile" 2>/dev/null || echo 0)

    if smoke_ok_rc "$rc" && ! log_crash_in_file "$logfile"; then
        pass "m4 smoke no crash"
    else
        fail "m4 smoke crashed or bad exit (rc=$rc; see $logfile)"
    fi

    if [ "${started_count:-0}" -eq "$traveler_count" ]; then
        pass "m4 started lines count ($started_count == $traveler_count)"
    else
        fail "m4 started lines (found $started_count, expected $traveler_count; see $logfile)"
    fi
else
    skip "m4 tests (sim not built)"
fi

# ---------------------------------------------------------------------------
log ""
log "=== Milestone 5 (IPC + static checks) ==="

# Static: parent must not call dijkstra_get_path before fork
if [ -f src/main_IPC.c ]; then
    fork_line=$(grep -n 'pid = fork()' src/main_IPC.c | head -1 | cut -d: -f1)
    parent_violation=""
    while IFS= read -r line_num; do
        if [ -n "$line_num" ] && [ -n "$fork_line" ] && [ "$line_num" -lt "$fork_line" ]; then
            parent_violation=1
        fi
    done < <(grep -n 'dijkstra_get_path' src/main_IPC.c | cut -d: -f1)

    dijk_count=$(grep -c 'dijkstra_get_path' src/main_IPC.c || true)

    if [ -z "$parent_violation" ] && [ "${dijk_count:-0}" -ge 1 ]; then
        pass "m5 static: dijkstra_get_path only after fork (child)"
    else
        fail "m5 static: parent may call dijkstra_get_path before fork (see src/main_IPC.c)"
    fi
else
    fail "m5 static: src/main_IPC.c missing"
fi

if make milestone5 >"$LOG_DIR/build_milestone5.log" 2>&1; then
    pass "build milestone5"
else
    fail "build milestone5 (see $LOG_DIR/build_milestone5.log)"
fi

if [ ! -x ./sim ]; then
    skip "m5 runtime IPC logs (sim not built)"
elif ! have_cmd xvfb-run; then
    skip "m5 runtime IPC logs (xvfb-run not installed)"
else
    logfile="$LOG_DIR/m5_ipc_multi.log"
    set +e
    run_sim_with_play_click 20 "$INPUT_DIR/m5_ipc_multi.txt" "$logfile"
    rc=$?
    set -e

    if smoke_ok_rc "$rc" && ! log_crash_in_file "$logfile"; then
        pass "m5 smoke no crash"
    else
        fail "m5 smoke (exit $rc; see $logfile)"
    fi

    if grep -q '\[PID=[0-9]*\] started' "$logfile"; then
        fail "m5 children must not print [PID=...] started (found in $logfile)"
    else
        pass "m5 no child started lines in output"
    fi

    m5_patterns_ok=1
    for pattern in 'arrived at node' 'next node:' 'DESTINATION' 'finished'; do
        if ! grep -q "$pattern" "$logfile"; then
            fail "m5 missing parent log pattern: $pattern (see $logfile)"
            m5_patterns_ok=0
        fi
    done
    if [ "$m5_patterns_ok" -eq 1 ]; then
        pass "m5 parent IPC log patterns present"
    fi

    if ! have_cmd xdotool; then
        skip "m5 PLAY click used sleep-only fallback (install xdotool for reliable IPC logs)"
    fi
fi

# ---------------------------------------------------------------------------
log ""
log "=== Milestone 6 (synchronization + smoke) ==="

if [ -f src/main_node_sync.c ]; then
    sync_ok=1
    for sym in sem_init sem_trywait sem_post; do
        if ! grep -q "$sym" src/main_node_sync.c; then
            fail "m6 static: missing $sym in src/main_node_sync.c"
            sync_ok=0
        fi
    done
    if [ "$sync_ok" -eq 1 ]; then
        pass "m6 static: semaphore API used (sem_init/sem_trywait/sem_post)"
    fi

    if grep -qE 'STATE_WAITING_OUTSIDE|WAITING' src/main_node_sync.c && \
       grep -qiE 'orange|"W"|DrawCircleLines' src/main_node_sync.c; then
        pass "m6 static: waiting GUI state markers present"
    else
        fail "m6 static: waiting GUI markers not found in src/main_node_sync.c"
    fi

    if grep -q 'shmget\|shmat\|shmctl' src/main_node_sync.c; then
        pass "m6 static: System V shared memory used"
    else
        fail "m6 static: shared memory calls missing"
    fi
else
    fail "m6 static: src/main_node_sync.c missing"
fi

if make milestone6 >"$LOG_DIR/build_milestone6.log" 2>&1; then
    pass "build milestone6"
else
    fail "build milestone6 (see $LOG_DIR/build_milestone6.log)"
fi

if [ ! -x ./sim ]; then
    skip "m6 runtime smoke (sim not built)"
elif ! have_cmd xvfb-run; then
    skip "m6 runtime smoke (xvfb-run not installed)"
else
    logfile="$LOG_DIR/m6_three_waiting.log"
    set +e
    if have_cmd timeout; then
        timeout 15 xvfb-run -a bash -c "
            cd '$ROOT' &&
            ./sim '$INPUT_DIR/m6_three_waiting.txt' >'$logfile' 2>&1 &
            sim_pid=\$!
            sleep 2
            if command -v xdotool >/dev/null 2>&1; then
                win=\$(xdotool search --name 'Graph GUI' 2>/dev/null | head -n1)
                if [ -n \"\$win\" ]; then
                    xdotool windowactivate --sync \"\$win\" mousemove --window \"\$win\" 80 40 click 1
                fi
            fi
            sleep 10
            kill \"\$sim_pid\" 2>/dev/null || true
            wait \"\$sim_pid\" 2>/dev/null || true
        "
        rc=$?
    else
        run_sim_headless 15 "$INPUT_DIR/m6_three_waiting.txt" "$logfile"
        rc=$?
    fi
    set -e

    if smoke_ok_rc "$rc" && ! log_crash_in_file "$logfile"; then
        pass "m6 smoke (timeout 15s, no crash)"
    else
        fail "m6 smoke (exit $rc; see $logfile)"
    fi

    if grep -q 'waiting outside node' "$logfile"; then
        pass "m6 waiting log lines observed"
    else
        fail "m6 no waiting outside node logs (may need PLAY/xdotool; see $logfile)"
    fi
fi

# ---------------------------------------------------------------------------
log ""
log "=== Final summary ==="
log "Total checks: $TOTAL"
log "Passed:       $PASS"
log "Failed:       $FAIL"
log "Skipped:      $SKIP"
log "Logs:         $LOG_DIR/"

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
exit 0

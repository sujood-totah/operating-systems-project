# Professor-like test suite (Milestones 1–6)

Strict, automated checks modeled after course grader feedback for the Operating Systems graph simulation project.

## How to run

From the repository root on **Linux** (reference platform):

```bash
bash tests/professor_like/run_professor_like_tests.sh
```

Recommended packages:

```bash
sudo apt install build-essential libraylib-dev xvfb xdotool valgrind
```

Logs are written to `tests/professor_like/logs/`.

Exit code **0** = all required checks passed. Non-zero = at least one failure.

## What is tested

### General repository

| Check | Type |
|-------|------|
| `README.md` and `Makefile` exist | Automatic |
| Makefile targets `milestone1` … `milestone6`, `clean` | Automatic |
| `make all` includes milestone6 | Automatic |
| Git tags `milestone1` … `milestone6` | Automatic |
| README build/run per milestone, M5 IPC, M6 sync | Automatic |
| Header allowlist scan (no forbidden libraries) | Automatic |

### Milestone 1 — CLI Dijkstra

| Input | Expected behavior |
|-------|-------------------|
| `m1_normal_shortest.txt` | Exact path `0 -> 1 -> 2 -> 3`, distance `4` |
| `m1_alternate_route.txt` | Cheaper multi-hop path, not expensive direct edge |
| `m1_disconnected.txt` | `No path found` |
| `m1_negative_weight.txt` | `Invalid input`, exit 1 |
| `m1_same_source_dest.txt` | Node then `0` |
| `m1_larger_graph.txt` | Correct path on 8-node graph |
| `m1_invalid_format.txt` | `Invalid input`, exit 1 |
| `m1_blank_lines_comments.txt` | Blank lines OK; `#` comment lines → `Invalid input` (strict parser) |
| No arguments | `Invalid input`, exit 1 |
| Valgrind on valid input | No invalid access / leaks |

### Milestones 2–3 — GUI smoke

| Check | Type |
|-------|------|
| `make milestone2` / `make milestone3` | Automatic build |
| `xvfb-run -a timeout 7s ./sim <input>` | No crash / segfault |
| 15-node layout input (M2) | Automatic |
| Single-traveler input (M3) | Automatic |

Visual checks (arrows, weights, 300 ms steps, 1 s node wait) are **not** automated here.

### Milestone 4 — Multi-process

| Check | Type |
|-------|------|
| `make milestone4` | Automatic build |
| Two travelers, headless smoke | No crash |
| `[PID=xxxx] started` line count == traveler count | Automatic log grep |

### Milestone 5 — IPC

| Check | Type |
|-------|------|
| Static: no `dijkstra_get_path()` in parent before `fork()` | Source scan |
| `make milestone5` | Automatic build |
| Parent logs: `arrived`, `next node`, `DESTINATION`, `finished` | Automatic (needs xvfb + xdotool PLAY click) |
| Children must **not** print `[PID=...] started` | Automatic |

### Milestone 6 — Node synchronization

| Check | Type |
|-------|------|
| Static: `sem_init`, `sem_trywait`, `sem_post`, SysV shm | Source scan |
| Static: waiting GUI markers (`STATE_WAITING_OUTSIDE`, orange, `W`) | Source scan |
| `make milestone6` | Automatic build |
| `m6_three_waiting.txt` — 3 travelers contend for node 2 | Headless smoke 15s |
| Log contains `waiting outside node` (with PLAY) | Automatic |

## Manual checks (not fully automated)

These still require human verification for full submission credit:

1. **Milestone 3** — edge animation timing (300 ms per jump), 1 s wait at intermediate nodes only, destination message.
2. **Milestone 6 demo video** — 30–60 s recording showing three travelers waiting for the same node and entering one-by-one, 1 s inside each node.
3. **GUI clarity** — colors, readable layout, play/stop behavior under real X11 display.
4. **`#` comment support** — if the assignment requires comment lines, the current `graph_io.c` parser does not support them; professor test expects `Invalid input` until implemented.

## File layout

```
tests/professor_like/
  run_professor_like_tests.sh   # main runner
  README.md                     # this file
  inputs/                       # test graph files
  expected/                     # Milestone 1 expected stdout
  logs/                         # created when tests run
```

## Interpreting failures

- **Build failures** — install `libraylib-dev` and build tools.
- **M2–M6 smoke SKIP** — install `xvfb-run`; for M5/M6 IPC logs install `xdotool`.
- **M5 IPC log FAIL without xdotool** — PLAY was not clicked; children never started.
- **m1_blank_lines_comments FAIL** — expected until parser skips `#` lines.
- **README make all FAIL** — README text must match Makefile (`milestones 1-6`).

# Milestone self-test checklist (manual + automated)

Use `bash tests/run_tests.sh` from the repo root for automated checks. Use this list for manual GUI verification on Linux with a display (or `ssh -X`).

---

## Milestone 1 — CLI Dijkstra (`make milestone1`, `./dijkstra <file>`)

| # | Check | Input | Expected |
|---|--------|-------|----------|
| 1.1 | Build | `make milestone1` | Produces `./dijkstra` |
| 1.2 | Shortest path + distance | `inputs/tests/m1_shortest_path.txt` | `0 -> 1 -> 2 -> 3` then `4` |
| 1.3 | No path | `inputs/tests/m1_no_path.txt` | `No path found` |
| 1.4 | Same source/dest | `inputs/tests/m1_same_source_dest.txt` | `2` then `0` |
| 1.5 | Negative weight | `inputs/tests/m1_negative_weight.txt` | `Invalid input`, exit 1 |
| 1.6 | Missing file arg | `./dijkstra` | `Invalid input`, exit 1 |
| 1.7 | Course autograder graph | `inputs/tests/m1_example1_path.txt` | `0 -> 2 -> 5` then `12` |

---

## Milestone 2 — Graph GUI (`make milestone2`, `./sim <file>`)

| # | Check | How to verify |
|---|--------|----------------|
| 2.1 | Build | `make milestone2` → `./sim` exists |
| 2.2 | Window opens | Run with any valid input (e.g. `inputs/tests/m3_single_traveler.txt`) |
| 2.3 | Nodes visible | Numbered circles in a readable layout |
| 2.4 | Directed edges + weights | Arrows and weight labels on edges |
| 2.5 | Max 15 nodes | `inputs/tests/m2_max_nodes.txt` loads without error |
| 2.6 | Shortest path highlighted | Path edges drawn differently from other edges |

---

## Milestone 3 — Animated traveler (`make milestone3`, `./sim <file>`)

| # | Check | How to verify |
|---|--------|----------------|
| 3.1 | Build | `make milestone3` |
| 3.2 | Purple traveler | Circle starts at source |
| 3.3 | PLAY / STOP | Button toggles movement |
| 3.4 | Edge steps | Weight `W` edge crossed in `W` jumps (~300 ms each) |
| 3.5 | Intermediate wait | ~1 s pause at each intermediate node (not source/dest) |
| 3.6 | Destination message | On-screen arrival + distance when done |
| 3.7 | Same source/dest | PLAY shows `N/A`; immediate completion state |

---

## Milestone 4 — `fork()` + multi-traveler GUI (`make milestone4`, `./sim <file>`)

| # | Check | How to verify |
|---|--------|----------------|
| 4.1 | Build | `make milestone4` |
| 4.2 | Child start logs | Terminal shows `[PID=…] started` per traveler |
| 4.3 | Different colors | Each traveler has a distinct color in the GUI |
| 4.4 | Simultaneous motion | Click PLAY; all travelers move together |
| 4.5 | Parent manages children | Children exit after parent signals on completion |
| 4.6 | Parent waits | Process does not exit until children are reaped |

**Input:** `inputs/tests/m4_multi_traveler.txt` (2 travelers).

---

## Milestone 5 — IPC + parent logs (`make milestone5`, `./sim <file>`)

| # | Check | How to verify |
|---|--------|----------------|
| 5.1 | Build | `make milestone5` |
| 5.2 | Child computes path | Each child runs its own Dijkstra after `fork()` |
| 5.3 | IPC mechanism | Anonymous **pipe** per traveler (see README) |
| 5.4 | Parent-only logs | After PLAY, terminal shows: |
| | | `[PID=…] arrived at node X \| next node: Y` |
| | | `[PID=…] arrived at node X \| DESTINATION` |
| | | `[PID=…] finished` |
| 5.5 | GUI updates | Traveler positions update as messages arrive |

**Input:** `inputs/tests/m5_ipc_fast.txt` (fast `0 -> 1` path).

---

## Automated script limitations

- Milestones 2–5 need Raylib and (for headless runs) `xvfb-run`.
- Milestone 5 log checks need `xdotool` to click **PLAY** in a virtual framebuffer.
- Animation timing, colors, and on-screen text are **not** fully asserted by the script; use this checklist for those.

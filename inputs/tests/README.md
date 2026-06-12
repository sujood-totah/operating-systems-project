# Test input files (`inputs/tests/`)

Graph files use the assignment format (no inline comments). Each file name encodes the milestone and scenario; this table maps files to requirements.

| File | Milestone | Requirement covered |
|------|-----------|---------------------|
| `m1_shortest_path.txt` | 1 | Directed weighted graph; print exact shortest path and total distance (`0 -> 1 -> 2 -> 3`, distance `4`). |
| `m1_no_path.txt` | 1 | Unreachable destination prints `No path found`. |
| `m1_same_source_dest.txt` | 1 | `source == destination` prints the node alone, then `0`. |
| `m1_negative_weight.txt` | 1 | Negative edge weight is invalid input (`Invalid input`). |
| `m1_multi_traveler.txt` | 1 | Multiple `source dest` pairs; one shortest-path result per traveler. |
| `m2_max_nodes.txt` | 2 | Graph with **15 nodes** (GUI cap); readable layout, edges, weights. |
| `m3_single_traveler.txt` | 3 | Single traveler for PLAY/STOP animation (300 ms per jump, 1 s at intermediate nodes). |
| `m4_multi_traveler.txt` | 4 | Two travelers; parent forks children (`[PID=…] started`), simultaneous GUI colors. |
| `m5_ipc_fast.txt` | 5 | Shortest path `0 -> 1` for fast IPC log checks (`arrived`, `next node`, `DESTINATION`, `finished`). |

Expected CLI output for milestone 1 lives under `tests/expected/`.

# Operating Systems Project

Graph traffic simulation: load a directed weighted graph from a file, compute shortest paths with **Dijkstra**, and visualize the travelers with **Raylib** across milestones 1–7.

## Team

- sujood totah ,ID: 214018442
- doha abdelnabi ,ID:326131448

---

## Prerequisites (Linux — reference platform)

This project is built and graded on **Linux**. Install:

- **GCC** (`gcc`) with **C99**
- **GNU Make**
- **Raylib** development libraries (for `sim`)

Example (Debian/Ubuntu):

```bash
sudo apt install build-essential libraylib-dev
```

The `Makefile` links with `-lraylib -lGL -lm -lpthread -ldl -lrt -lX11` (typical native Linux + X11). Use a normal desktop session or SSH with **X11 forwarding** if you run `./sim` remotely.

---

## Input file format

Plain text, ordered as follows:

1. First line: `N M` — number of vertices (`N`), number of edges (`M`).
2. Next `M` lines: `u v w` — directed edge from `u` to `v` with non-negative integer weight `w`. Vertices are `0 … N-1`.
3. Next line: `T` — number of travelers (milestones 2–7).
4. Next `T` lines: `source dest` — one source/destination pair per traveler.

**Milestone 1 only:** step 3 is omitted; the file ends with a single `source dest` line.

Example (`inputs/example1.txt`):

```
6 8
0 1 4
0 2 2
1 3 5
2 1 1
2 3 8
3 4 2
4 5 3
2 5 10
3
0 5
1 4
2 3
```

---

## Build

From the repository root:

```bash
make milestone1    # builds ./dijkstra (CLI only)
make milestone2    # builds ./sim (GUI + graph + shortest-path animation)
make milestone3    # same artifact as milestone2 (per assignment milestones)
make milestone4    # builds ./sim for the multi-process GUI version
make milestone5    # builds ./sim for the IPC version
make milestone6    # builds ./sim for node synchronization
make milestone7    # builds ./sim for process scheduling
make all           # builds milestones 1-7
make clean         # removes ./dijkstra, ./sim, and *.o
```

---

## Run

### Milestone 1 — CLI shortest path

```bash
./dijkstra <graph_file>
# e.g. ./dijkstra inputs/example1.txt
```

Prints one shortest-path result per traveler: the path as `v0 -> v1 -> ...` on one line, then the total distance on the next line (or `No path found` / error messages as implemented).

### Milestones 2–3 — GUI (`sim`)

```bash
./sim <graph_file>
# e.g. ./sim inputs/example1.txt
```

**Milestone 2:** draws the graph (vertices, directed edges, weights).

**Milestone 3 (behavior in `sim`):**

- Computes and highlights the **shortest-path** edges; source / destination vertices use distinct colors.
- **Purple** circle = agent; starts at **source**.
- **PLAY / STOP** toggles movement along the computed path.
- Each edge of integer weight **W** is crossed in **W** steps; **each step lasts 300 ms**.
- At every **intermediate** vertex (not source, not destination), the agent **waits 1 second** before the next edge.
- When the destination is reached, an **on-screen message** shows arrival and the shortest-path cost.
- If source equals destination, completion is shown immediately (no animation); PLAY shows **N/A**.
- After finishing a run, **PLAY** restarts the animation from the source.

If there is **no path**, the program prints `No path found` and exits before opening the window.

### Milestone 4 — multiple travelers with `fork()`

```bash
make milestone4
./sim <graph_file>
```

- The parent process reads the full input file and computes the shortest path for each traveler before `fork()`.
- A child process is created for every traveler and prints `[PID=...] started`, then waits until the parent terminates it.
- The parent runs the Raylib loop and animates all travelers concurrently on the graph.
- Each traveler is shown with a different color in the GUI.
- When a traveler reaches the destination, the parent sends a signal to terminate that child and waits for all children before exiting.

### Milestone 5 — IPC between children and parent

```bash
make milestone5
./sim <graph_file>
```

- Each child computes its own shortest path with Dijkstra after `fork()`.
- The chosen IPC mechanism is an anonymous `pipe` per traveler.
- Each child reports every node arrival to the parent through its pipe.
- The parent is the only process that prints the runtime log:
    - `[PID=...] arrived at node X | next node: Y`
    - `[PID=...] arrived at node X | DESTINATION`
    - `[PID=...] finished`
- The parent updates the GUI according to the node reports it receives from the children.

`pipe` was chosen because communication is one-way (child -> parent), the message format is small and fixed, and it keeps the implementation simpler than shared memory for this milestone.

---

## Milestone 6 – Node Synchronization

Milestone 6 extends Milestone 5. Each traveler is a separate process created with `fork()`. Communication between children and the GUI still uses **pipes**; **synchronization** is implemented with **System V shared memory** and **POSIX semaphores** (`sem_init` with `pshared = 1`, one binary semaphore per graph node).

Before entering a node, a traveler must acquire that node’s semaphore. The traveler stays inside the node for exactly **1 second**, then releases the semaphore with `sem_post()`. If the node is occupied, the traveler waits outside; the GUI shows it as an **orange hollow circle** with a **"W"**. FIFO entry order is not required.

The parent prints runtime log lines for waiting, arrival, and completion:

- `[PID=...] waiting outside node X`
- `[PID=...] arrived at node X | next node: Y`
- `[PID=...] arrived at node X | DESTINATION`
- `[PID=...] finished`

### Build and Run

```bash
make milestone6
./sim inputs/tests/m6_three_waiting.txt
```

Click **PLAY** to start the travelers. Use `inputs/tests/m6_three_waiting.txt` to demonstrate multiple travelers competing for the same node.

---

## Milestone 7 – Process Scheduling

### Overview

Milestone 7 extends Milestone 6 by introducing **process scheduling** for travelers entering intersections (graph nodes). When several travelers are waiting to enter the same node, the **scheduler** decides which one is allowed to continue, according to the selected scheduling algorithm.

All Milestone 6 behavior is preserved: **IPC** (pipes), **shared memory**, **semaphore synchronization**, **GUI animation**, **shortest-path highlighting**, **waiting visualization (W)**, and **visual completion** separated from child-process completion.

### Features

- Scheduling integrated with the existing Milestone 6 implementation.
- Supports two scheduling algorithms: **FCFS** and **SJF**.
- Scheduling decisions are made when multiple travelers are waiting to enter the **same node**.
- IPC communication remains unchanged (anonymous `pipe` per traveler).
- Semaphore synchronization remains unchanged (one binary semaphore per node in shared memory).
- GUI continues using Milestone 4-style, weight-based animation.
- Waiting visualization (**W**, orange hollow circle) is preserved.
- Visual completion (`finished`) is separated from child-process completion (`child_finished`).

### Supported Scheduling Algorithms

#### FCFS (First Come First Served)

- Travelers are scheduled in **arrival order** at each node.
- The first traveler to request a node is the first to be granted access when the node becomes free.

#### SJF (Shortest Job First)

- **Execution time** is represented by the total shortest-path distance calculated by Dijkstra.
- `burst_time = total_distance` (sum of edge weights on the shortest path).
- The traveler with the **smallest** `total_distance` is selected first.
- `arrival_order` is used as a tie-breaker when two travelers have the same burst time.

### Command Line Usage

```bash
make milestone7

./sim -schd fcfs inputs/tests/m6_three_waiting.txt
./sim -schd sjf inputs/tests/m6_three_waiting.txt
```

Click **PLAY** to start the travelers after the window opens.

### Internal Design

| File | Role |
|------|------|
| `src/scheduler.h` | Scheduler types, per-node queue API, `choose_next_traveler()` |
| `src/scheduler.c` | FCFS and SJF selection logic |
| `src/main_scheduling.c` | Milestone 6 base + scheduling IPC, CLI parsing, and GUI |

Key concepts:

- **Per-node waiting queues** — each graph node has its own queue of travelers waiting for scheduler dispatch.
- **`burst_time`** — stored per queued traveler; set from `total_distance` (Dijkstra shortest-path cost). Used by SJF.
- **`arrival_order`** — monotonic counter assigned when a traveler requests a node; used by FCFS and as a tie-breaker for SJF.
- **`child_finished` vs `finished`** — the child sets `child_finished` when it reaches the destination; the GUI sets `finished` only when the animation visually arrives at the destination node.
- **SIGSTOP / SIGCONT** — before attempting to enter a node, each child sends a request and raises **SIGSTOP**; the parent scheduler chooses the next traveler and sends **SIGCONT** to grant access.

### GUI

The Milestone 7 GUI displays:

- **Selected scheduler** (`Scheduler: FCFS` or `Scheduler: SJF`) — shown permanently below the PLAY/STOP button.
- **Traveler status** lines (`T0: source -> dest | distance: …`, with `| finished` when the animation completes).
- **Waiting indicator (W)** — orange hollow circle when a traveler waits outside an occupied node.
- **Shortest path highlighting** — path edges drawn in green; other edges in gray.
- **Weight-based traveler animation** — movement speed follows edge weights (Milestone 4 style).

### Testing

Milestone 7 was tested manually with `inputs/tests/m6_three_waiting.txt` and other multi-traveler inputs:

| Area | What was verified |
|------|-------------------|
| **FCFS** | Travelers granted access in request order at contested nodes |
| **SJF** | Shorter `total_distance` travelers scheduled before longer ones |
| **Multiple travelers** | Concurrent animation and logging for all travelers |
| **Semaphore waiting** | Only one traveler inside a node at a time; others wait outside |
| **GUI animation** | Weight-based movement, PLAY/STOP, path highlighting |
| **Visual completion** | `| finished` and “All travelers arrived!” appear only after dots reach destination visually |

### Notes

Milestone 7 builds directly on Milestone 6 and does **not** replace any previous functionality. Milestones 1–6 remain available via their respective `make` targets.

---

## Source layout (high level)

| Component        | Role |
|-----------------|------|
| `src/graph.*`   | Adjacency-list graph, `add_edge`, `free_graph` |
| `src/graph_io.*`| Load graph + traveler source/destination pairs from file |
| `src/dijkstra.*`| `dijkstra()` for CLI; `dijkstra_get_path()` for GUI |
| `src/main_dijkstra.c` | Milestone 1 entry |
| `src/main_GUI.c`      | Raylib GUI and milestone 3 timing / controls |
| `src/multiple_GUI.c`  | Milestone 4 parent/children GUI with `fork()` |
| `src/main_IPC.c`      | Milestone 5 IPC-based GUI using `pipe` |
| `src/main_node_sync.c`| Milestone 6 node synchronization with shared memory and semaphores |
| `src/scheduler.h`     | Milestone 7 scheduler types and queue API |
| `src/scheduler.c`     | Milestone 7 FCFS / SJF logic |
| `src/main_scheduling.c`| Milestone 7 scheduling GUI and process dispatch |

---

## How to test milestones 1–5

From the repository root on **Linux** (reference platform):

```bash
bash tests/run_tests.sh
```

The script will:

1. `make clean`, then build `milestone1` … `milestone5`
2. Compare milestone 1 stdout to `tests/expected/*.out`
3. Smoke-launch milestones 2–3 under `xvfb-run` (if installed) with a short timeout
4. Run milestone 4 with a timeout and check for `[PID=…] started` from each child
5. Run milestone 5 with `xvfb-run` + `xdotool` (if installed) to click **PLAY** and verify IPC log lines

Logs are written to `tests/logs/`. For manual GUI checks (colors, timing, buttons), use **`tests/CHECKLIST.md`**. Test inputs and which requirement each file covers are documented in **`inputs/tests/README.md`**.

Optional packages for full automated GUI/IPC checks:

```bash
sudo apt install xvfb xdotool
```

On Windows, run the script inside **WSL** with the same Linux packages (`build-essential`, `libraylib-dev`, `xvfb`, `xdotool`).

---

## Troubleshooting

- **`undefined reference` when linking `sim`:** install `libraylib-dev` (or your distro’s Raylib `-dev` package). Compare with `pkg-config --libs raylib` if your distro uses different flags.
- **`DISPLAY` / blank window over SSH:** `sim` needs a GUI; run locally on Linux or enable **X11 forwarding** (`ssh -X`).

# Operating Systems Project

Graph traffic simulation: load a directed weighted graph from a file, compute shortest paths with **Dijkstra**, and visualize the travelers with **Raylib** across milestones 1-5.

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
3. Next line: `T` — number of travelers.
4. Next `T` lines: `source dest` — one source/destination pair per traveler.

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
make all           # builds milestones 1-5
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

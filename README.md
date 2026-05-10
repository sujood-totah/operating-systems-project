# Operating Systems Project

Graph traffic simulation: load a directed weighted graph from a file, compute a shortest path with **Dijkstra**, and (milestones 2–3) visualize it with **Raylib**.

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
3. Last line: `source dest` — start and goal vertices for Dijkstra.

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
0 5
```

---

## Build

From the repository root:

```bash
make milestone1    # builds ./dijkstra (CLI only)
make milestone2    # builds ./sim (GUI + graph + shortest-path animation)
make milestone3    # same artifact as milestone2 (per assignment milestones)
make all           # milestone1 + milestone2 + milestone3
make clean         # removes ./dijkstra, ./sim, and *.o
```

---

## Run

### Milestone 1 — CLI shortest path

```bash
./dijkstra <graph_file>
# e.g. ./dijkstra inputs/example1.txt
```

Prints the shortest path as `v0 -> v1 -> ...` on one line, then the total distance on the next line (or `No path found` / error messages as implemented).

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

---

## Source layout (high level)

| Component        | Role |
|-----------------|------|
| `src/graph.*`   | Adjacency-list graph, `add_edge`, `free_graph` |
| `src/graph_io.*`| Load graph + `source`/`destination` from file |
| `src/dijkstra.*`| `dijkstra()` for CLI; `dijkstra_get_path()` for GUI |
| `src/main_dijkstra.c` | Milestone 1 entry |
| `src/main_GUI.c`      | Raylib GUI and milestone 3 timing / controls |

---

## Troubleshooting

- **`undefined reference` when linking `sim`:** install `libraylib-dev` (or your distro’s Raylib `-dev` package). Compare with `pkg-config --libs raylib` if your distro uses different flags.
- **`DISPLAY` / blank window over SSH:** `sim` needs a GUI; run locally on Linux or enable **X11 forwarding** (`ssh -X`).

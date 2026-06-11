CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -g

SIM_SRC=src/main_GUI.c src/graph.c src/graph_io.c src/dijkstra.c
RAYLIB_LIBS=-lraylib -lGL -lm -lpthread -ldl -lrt -lX11

all: milestone1 milestone2 milestone3

milestone1:
	$(CC) $(CFLAGS) src/main_dijkstra.c src/graph.c src/graph_io.c src/dijkstra.c -o dijkstra

milestone2 milestone3: sim

sim: $(SIM_SRC)
	$(CC) $(CFLAGS) $(SIM_SRC) -o sim $(RAYLIB_LIBS)

clean:
	rm -f dijkstra sim *.o

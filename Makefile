CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -g

SINGLE_GUI_SRC=src/main_GUI.c src/graph.c src/graph_io.c src/dijkstra.c
MULTI_GUI_SRC=src/multiple_GUI.c src/graph.c src/graph_io.c src/dijkstra.c
IPC_GUI_SRC=src/main_IPC.c src/graph.c src/graph_io.c src/dijkstra.c
RAYLIB_LIBS=-lraylib -lGL -lm -lpthread -ldl -lrt -lX11

all: milestone1 milestone2 milestone3 milestone4 milestone5

milestone1:
	$(CC) $(CFLAGS) src/main_dijkstra.c src/graph.c src/graph_io.c src/dijkstra.c -o dijkstra

milestone2 milestone3:
	$(CC) $(CFLAGS) $(SINGLE_GUI_SRC) -o sim $(RAYLIB_LIBS)

milestone4:
	$(CC) $(CFLAGS) $(MULTI_GUI_SRC) -o sim $(RAYLIB_LIBS)

milestone5:
	$(CC) $(CFLAGS) $(IPC_GUI_SRC) -o sim $(RAYLIB_LIBS)

clean:
	rm -f dijkstra sim *.o
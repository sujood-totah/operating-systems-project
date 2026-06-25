CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -g

NODE_SYNC_SRC=src/main_node_sync.c src/graph.c src/graph_io.c src/dijkstra.c
SINGLE_GUI_SRC=src/main_GUI.c src/graph.c src/graph_io.c src/dijkstra.c
MULTI_GUI_SRC=src/multiple_GUI.c src/graph.c src/graph_io.c src/dijkstra.c
IPC_GUI_SRC=src/main_IPC.c src/graph.c src/graph_io.c src/dijkstra.c
RAYLIB_LIBS=-lraylib -lGL -lm -lpthread -ldl -lrt -lX11

all: milestone1 milestone2 milestone3 milestone4 milestone5 milestone6
milestone1:
	$(CC) $(CFLAGS) src/main_dijkstra.c src/graph.c src/graph_io.c src/dijkstra.c -o dijkstra

milestone2 milestone3:
	$(CC) $(CFLAGS) $(SINGLE_GUI_SRC) -o sim $(RAYLIB_LIBS)

milestone4:
	$(CC) $(CFLAGS) $(MULTI_GUI_SRC) -o sim $(RAYLIB_LIBS)

milestone5:
	$(CC) $(CFLAGS) $(IPC_GUI_SRC) -o sim $(RAYLIB_LIBS)

milestone6:
	$(CC) $(CFLAGS) $(NODE_SYNC_SRC) -o sim $(RAYLIB_LIBS)

clean:
	rm -f dijkstra sim *.o

install-raylib:
	sudo apt update
	sudo apt install -y build-essential cmake git \
		libx11-dev libxrandr-dev libxi-dev \
		libgl1-mesa-dev libxcursor-dev libxinerama-dev
	rm -rf /tmp/raylib
	git clone https://github.com/raysan5/raylib.git /tmp/raylib
	cd /tmp/raylib && mkdir -p build
	cd /tmp/raylib/build && cmake ..
	cd /tmp/raylib/build && make
	cd /tmp/raylib/build && sudo make install
	sudo ldconfig
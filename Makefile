CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -g

milestone1:
	$(CC) $(CFLAGS) src/main_dijkstra.c src/graph.c src/dijkstra.c -o dijkstra
mailstone2:
	$(CC) $(CFLAGS) src/main_GUI.c src/graph.c -o graph_gui -lraylib -lm -lX11
clean:
	rm -f dijkstra sim *.o

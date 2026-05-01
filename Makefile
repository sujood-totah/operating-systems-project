CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -g

milestone1:
	$(CC) $(CFLAGS) src/main_dijkstra.c src/graph.c src/dijkstra.c -o dijkstra

clean:
	rm -f dijkstra sim *.o

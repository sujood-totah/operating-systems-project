CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -g

milestone1:
	$(CC) $(CFLAGS) src/main_dijkstra.c src/graph.c src/dijkstra.c -o dijkstra
mailstone2:
       ^I$(CC) $(CFLAGS) src/main_GUI.c src/graph.c -o sim -lraylib -lGL -lm -lpthread -ldl -lrt -lX11$
clean:
	rm -f dijkstra sim *.o

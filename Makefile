all: qbdiff

qbdiff: main.c
	$(CC) main.c -o qbdiff -std=c99 -O3 -Wall -Wextra -Werror

clean:
	rm -f qbdiff

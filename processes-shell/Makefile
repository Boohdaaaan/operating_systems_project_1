CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=gnu99

wish: wish.c
	$(CC) $(CFLAGS) -o wish wish.c

clean:
	rm -f wish
	rm -rf tests-out

.PHONY: clean

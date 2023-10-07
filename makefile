.PHONY: clean build
clean:
build: ezed
all: ezed

CFLAGS := -Werror -Wall

ezed: ezed.c macros.c
	$(CC) $(CFLAGS) $^ -o $@ -Werror -Wall

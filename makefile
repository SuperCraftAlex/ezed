.PHONY: clean build
build: ezed
all: ezed
clean:

CFLAGS := -Werror -Wall

ezed: ezed.c macros.c
	$(CC) $^ -o $@ $(CFLAGS)

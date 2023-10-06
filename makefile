clean:
	echo x

check:
	echo a

distcheck:
	echo b

build: ezed.c
	cc ezed.c macros.c -o ezed -Werror -Wall

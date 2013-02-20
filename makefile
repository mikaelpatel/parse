CC	= gcc

all: test parse

clean:
	rm -f parse test test.g

new:
	make clean
	make

parse: parse.c parse.h bnf.c bnf.h
	$(CC) -DTEST parse.c bnf.c -o parse

test: test.g test.c parse.h
	$(CC) parse.c test.c -o test

test.g: test.bnf parse
	parse -c test.bnf > test.g


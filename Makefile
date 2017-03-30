CC=g++
CFLAGS=-g

all: types sudoku

.cpp.o:
	${CC} ${CFLAGS} -c $^ -o $@

types: types.o
	${CC} ${CFLAGS} $^ -o $@

sudoku: sudoku.o
	${CC} ${CFLAGS} $^ -o $@

vector: vector.o
	${CC} ${CFLAGS} $^ -o $@

clean:
	rm -rf *.o

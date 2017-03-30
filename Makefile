CC=g++
CFLAGS=-g

all: sudoku

.cpp.o:
	${CC} ${CFLAGS} -c $^ -o $@

sudoku: sudoku.o
	${CC} ${CFLAGS} $^ -o $@

clean:
	rm -rf *.o sudoku

all: parallel sequential

parallel:
	gcc -O3 board.c board.h main.c pcg_basic.c pcg_basic.h -lm -fopenmp -o parallel

sequential:
	gcc -O3 -Wall -Wpedantic board.c board.h main.c pcg_basic.c pcg_basic.h -lm -o sequential

clean:
	-@rm parallel sequential
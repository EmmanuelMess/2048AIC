#ifndef UNTITLED_BOARD_H
#define UNTITLED_BOARD_H

#include <stdbool.h>
#include <inttypes.h>
#include <stddef.h>
#include "pcg_basic.h"

struct board {
	uint8_t data[4][4];
};

struct board create_board(pcg32_random_t* rng);

struct board create_elem(pcg32_random_t* rng, bool *couldAdd, struct board b);
enum direction {
	LEFT = 0,
	RIGHT = 1,
	DOWN = 2,
	UP = 3
};
#define values ((long[]) { 0, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536 })

struct board gravity(size_t * score, uint8_t *max, bool *boardChanged, struct board b, enum direction d);

void show(struct board b) ;

bool isBoardUnmovable(struct board b);

void getPossibleMoves(struct board b, bool set[4]);

#endif //UNTITLED_BOARD_H

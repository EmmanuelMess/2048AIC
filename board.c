#include <stdio.h>
#include <stdlib.h>
#include "board.h"
#include "pcg_basic.h"

struct board create_board(pcg32_random_t* rng) {
	return create_elem(rng, NULL, create_elem(rng, NULL, (struct board) {
		.data = {{0}}
	}));
}

struct board create_elem(pcg32_random_t* rng, bool *couldAdd, struct board b) {
	int amount = 0;

	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if(b.data[i][j] == 0) {
				amount++;
			}
		}
	}

	if(amount == 0) {
		if(couldAdd != NULL) {
			*couldAdd = false;
		}
		return b;
	}

	int x = 1 + (int) pcg32_boundedrand_r(rng, amount);
	bool isATwo = pcg32_boundedrand_r(rng, 10) < 9;

	amount = 0;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if(b.data[i][j] == 0) {
				amount++;
				if(amount == x) {
					b.data[i][j] = isATwo ? 1 : 2;

					if(couldAdd != NULL) {
						*couldAdd = true;
					}
					return b;
				}
			}
		}
	}

	if(couldAdd != NULL) {
		*couldAdd = false;
	}
	return b;
}


void getPossibleMoves(struct board b, bool set[4]) {
	int amount = 0;

	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			if (b.data[row][col] > 0) {
				continue;
			}

			if (!set[DOWN]) {
				for (int col2 = 0; col2 < col; col2++) {
					if (b.data[row][col2] > 0) {
						set[DOWN] = true;
						amount++;
						break;
					}
				}
			}

			if (!set[UP]) {
				for (int col2 = col + 1; col2 < 4; col2++) {
					if (b.data[row][col2] > 0) {
						set[UP] = true;
						amount++;
						break;
					}
				}
			}

			if (!set[RIGHT]) {
				for (int row2 = 0; row2 < row; row2++) {
					if (b.data[row2][col] > 0) {
						set[RIGHT] = true;
						amount++;
						break;
					}
				}
			}

			if (!set[LEFT]) {
				for (int row2 = row + 1; row2 < 4; row2++) {
					if (b.data[row2][col] > 0) {
						set[LEFT] = true;
						amount++;
						break;
					}
				}
			}

			if (amount == 4) {
				return;
			}
		}
	}

	if (!set[DOWN] || !set[UP]) {
		for (int row = 0; row < 4; row++) {
			for (int col = 0; col < 4 - 1; col++) {
				if (b.data[row][col] > 0 && b.data[row][col] == b.data[row][col + 1]) {
					set[UP] = true;
					set[DOWN] = true;
				}
			}
		}
	}

	if (!set[RIGHT] || !set[LEFT]) {
		for (int col = 0; col < 4; col++) {
			for (int row = 0; row < 4 - 1; row++) {
				if (b.data[row][col] > 0 && b.data[row][col] == b.data[row + 1][col]) {
					set[LEFT] = true;
					set[RIGHT] = true;
				}
			}
		}
	}
}

struct board gravitate(bool *boardChanged, struct board b, enum direction d) {
#define GRAVITATE_DIRECTION(_v1, _v2, _xs, _xc, _xi, _ys, _yc, _yi, _x, _y) \
    {                                                                       \
        int break_cond = 0;                                                 \
        while (!break_cond) {                                               \
            break_cond = 1;                                                 \
            for (int _v1 = _xs; _v1 _xc; _v1 += _xi) {                      \
                for (int _v2 = _ys; _v2 _yc; _v2 += _yi) {                  \
                    if (!b.data[x][y] && b.data[x + _x][y + _y]) {          \
                        b.data[x][y] = b.data[x + _x][y + _y];              \
                        b.data[x + _x][y + _y] = 0;                         \
                        if(boardChanged != NULL) {                          \
                            *boardChanged = true;                           \
                        }                                                   \
                        break_cond = 0;                                     \
                    }                                                       \
                }                                                           \
            }                                                               \
        }                                                                   \
    }

	switch (d) {
		case LEFT:
		GRAVITATE_DIRECTION(x, y, 0, < 3, 1, 0, < 4, 1, 1, 0);
			break;
		case RIGHT:
		GRAVITATE_DIRECTION(x, y, 3, > 0, -1, 0, < 4, 1, -1, 0);
			break;
		case DOWN:
		GRAVITATE_DIRECTION(y, x, 3, > 0, -1, 0, < 4, 1, 0, -1);
			break;
		case UP:
		GRAVITATE_DIRECTION(y, x, 0, < 3, 1, 0, < 4, 1, 0, 1);
			break;
		default:
			perror("Error");
			exit(-1);
			break;
	}
#undef GRAVITATE_DIRECTION

	return b;
}

struct board merge(size_t * score, uint8_t *max, struct board b, enum direction d) {
#define MERGE_DIRECTION(_v1, _v2, _xs, _xc, _xi, _ys, _yc, _yi, _x, _y)          \
    {                                                                            \
        for (int _v1 = _xs; _v1 _xc; _v1 += _xi) {                               \
            for (int _v2 = _ys; _v2 _yc; _v2 += _yi) {                           \
                if (b.data[x][y] && (b.data[x][y] == b.data[x + _x][y + _y])) {  \
                    b.data[x][y] += 1;                                           \
                    b.data[x + _x][y + _y] = 0;                                  \
                    if (score != NULL) {                                         \
                        *score += values[b.data[x][y]];                          \
                    }                                                            \
                    if(max != NULL && b.data[x][y] > *max) {                     \
                        *max = b.data[x][y];                                     \
                    }                                                            \
                }                                                                \
            }                                                                    \
        }                                                                        \
    }

	switch (d) {
		case LEFT:
		MERGE_DIRECTION(x, y, 0, < 3, 1, 0, < 4, 1, 1, 0);
			break;
		case RIGHT:
		MERGE_DIRECTION(x, y, 3, > 0, -1, 0, < 4, 1, -1, 0);
			break;
		case DOWN:
		MERGE_DIRECTION(y, x, 3, > 0, -1, 0, < 4, 1, 0, -1);
			break;
		case UP:
		MERGE_DIRECTION(y, x, 0, < 3, 1, 0, < 4, 1, 0, 1);
			break;
		default:
			perror("Error");
			exit(-1);
			break;
	}
#undef MERGE_DIRECTION
	return b;
}

struct board gravity(size_t * score, uint8_t *max, bool *boardChanged, struct board b, enum direction d) {
	if(d == -1) perror("Error");

	b = gravitate(boardChanged, b, d);
	b = merge(score, max, b, d);
	b = gravitate(boardChanged, b, d);
	return b;
}

void show(struct board b) {
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			printf("%d", b.data[j][i]);
		}
		printf("\n");
	}
}

static bool hasEqualNeighbour(struct board b, const int row, const int column) {
	if ((row > 0 && b.data[row - 1][column] == b.data[row][column])
	    || (column < 4 - 1 && b.data[row][column + 1] == b.data[row][column])
	    || (row < 4 - 1 && b.data[row + 1][column] == b.data[row][column])
	    || (column > 0 && b.data[row][column - 1] == b.data[row][column])) {
		return true;
	} else {
		return false;
	}
}

bool isBoardUnmovable(struct board b) {
	for (int row = 0; row < 4; row++) {
		for (int column = 0; column < 4; column++) {
			if (b.data[row][column] == 0) {
				return false;
			}
		}
	}
	for (int row = 0; row < 4; row++) {
		for (int column = 0; column < 4; column++) {
			if (hasEqualNeighbour(b, row, column)) {
				return false;
			}
		}
	}
	return true;
}
#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>
#include <stddef.h>
#include "board.h"
#ifdef _OPENMP
#include <omp.h>
#endif

static const bool useSymmetries = true;
static const float alpha = 0.0025f;
#define c 15

struct loc { int i; int j; };

/*
#define m (17)

static struct loc locations[m][50] = {
	{{.i = 0, .j = 0}, {.i = 1, .j = 0}, {.i = 2, .j = 0}, {.i = 3, .j = 0}, {.i = -1}},//1x4
	{{.i = 0, .j = 1}, {.i = 1, .j = 1}, {.i = 2, .j = 1}, {.i = 3, .j = 1}, {.i = -1}},//1x4
	{{.i = 0, .j = 2}, {.i = 1, .j = 2}, {.i = 2, .j = 2}, {.i = 3, .j = 2}, {.i = -1}},//1x4
	{{.i = 0, .j = 3}, {.i = 1, .j = 3}, {.i = 2, .j = 3}, {.i = 3, .j = 3}, {.i = -1}},//1x4

	{{.i = 0, .j = 0}, {.i = 0, .j = 1}, {.i = 0, .j = 2}, {.i = 0, .j = 3}, {.i = -1}},//4x1
	{{.i = 1, .j = 0}, {.i = 1, .j = 1}, {.i = 1, .j = 2}, {.i = 1, .j = 3}, {.i = -1}},//4x1
	{{.i = 2, .j = 0}, {.i = 2, .j = 1}, {.i = 2, .j = 2}, {.i = 2, .j = 3}, {.i = -1}},//4x1
	{{.i = 3, .j = 0}, {.i = 3, .j = 1}, {.i = 3, .j = 2}, {.i = 3, .j = 3}, {.i = -1}},//4x1

	{{.i = 0, .j = 0}, {.i = 0, .j = 1}, {.i = 1, .j = 1}, {.i = 1, .j = 0}, {.i = -1}},//2x2
	{{.i = 0, .j = 1}, {.i = 0, .j = 2}, {.i = 1, .j = 2}, {.i = 1, .j = 1}, {.i = -1}},//2x2
	{{.i = 0, .j = 2}, {.i = 0, .j = 3}, {.i = 1, .j = 3}, {.i = 1, .j = 2}, {.i = -1}},//2x2

	{{.i = 1, .j = 0}, {.i = 1, .j = 1}, {.i = 2, .j = 1}, {.i = 2, .j = 0}, {.i = -1}},//2x2
	{{.i = 1, .j = 1}, {.i = 1, .j = 2}, {.i = 2, .j = 2}, {.i = 2, .j = 1}, {.i = -1}},//2x2
	{{.i = 1, .j = 2}, {.i = 1, .j = 3}, {.i = 2, .j = 3}, {.i = 2, .j = 2}, {.i = -1}},//2x2

	{{.i = 2, .j = 0}, {.i = 2, .j = 1}, {.i = 3, .j = 1}, {.i = 3, .j = 0}, {.i = -1}},//2x2
	{{.i = 2, .j = 1}, {.i = 2, .j = 2}, {.i = 3, .j = 2}, {.i = 3, .j = 1}, {.i = -1}},//2x2
	{{.i = 2, .j = 2}, {.i = 2, .j = 3}, {.i = 3, .j = 3}, {.i = 3, .j = 2}, {.i = -1}},//2x2
};
*/

#define m (4)

static const struct loc locations[m][50] = {
	{{.i = 0, .j = 2}, {.i = 1, .j = 2}, {.i = 2, .j = 2}, {.i = 3, .j = 2}, {.i = -1}},//1x4
	{{.i = 0, .j = 3}, {.i = 1, .j = 3}, {.i = 2, .j = 3}, {.i = 3, .j = 3}, {.i = -1}},//1x4

	{{.i = 0, .j = 0}, {.i = 0, .j = 1}, {.i = 1, .j = 1}, {.i = 2, .j = 1}, {.i = 2, .j = 0}, {.i = 1, .j = 0}, {.i = -1}},//2x3
	{{.i = 0, .j = 1}, {.i = 0, .j = 2}, {.i = 1, .j = 2}, {.i = 2, .j = 2}, {.i = 2, .j = 1}, {.i = 1, .j = 1}, {.i = -1}},//2x3
};

static float *LUT[m];
#ifdef _OPENMP
omp_lock_t writelock[m];
#endif

void initLUT() {
	for (int i = 0; i < m; ++i) {
		int cant;
		size_t size = 1;
		for (cant = 0; locations[i][cant].i != -1; ++cant) size *= c;
		LUT[i] = calloc(size, sizeof(float));
	}
}


struct loc transpose(struct loc location) {
	return (struct loc) {.i = location.j, .j = location.i};
}

struct loc vsym(struct loc location) {
	return (struct loc) {.i = location.i, .j = location.j < 2 ? 3 - location.j : -location.j + 3 };
}

struct loc hsym(struct loc location) {
	return (struct loc) {.i = location.i < 2 ? 3 - location.i : -location.i + 3, .j = location.j };
}

void indexLUT(size_t ind[8], int i, struct board s) {
	for (int j = 0; locations[i][j].i != -1; ++j) {
		for (int k = 0; k < 8; ++k) {
			ind[k] *= c;//Horner
		}

		struct loc l = locations[i][j];	//Normal
		ind[0] += s.data[l.i][l.j];

		struct loc tl = transpose(l); // Transpose
		ind[1] += s.data[tl.i][tl.j];

		struct loc vl = vsym(l); // vertical symmetry
		ind[2] += s.data[vl.i][vl.j];

		struct loc hl = hsym(l); // horizontal symmetry
		ind[3] += s.data[hl.i][hl.j];

		struct loc tvl = vsym(transpose(l)); // transpose vertical
		ind[4] += s.data[tvl.i][tvl.j];

		struct loc thl = hsym(transpose(l)); // transpose horizontal
		ind[5] += s.data[thl.i][thl.j];

		struct loc vhl = hsym(vsym(l)); // vertical horizontal
		ind[6] += s.data[vhl.i][vhl.j];

		struct loc vhtl = transpose(hsym(vsym(l))); // vertical horizontal transpose
		ind[7] += s.data[vhtl.i][vhtl.j];
	}
}

float getV(struct board s) {
	float v = 0;

	for (int i = 0; i < m; ++i) {
		size_t ind[8] = {0};
		indexLUT(ind, i, s);

#ifdef _OPENMP
		omp_set_lock(&(writelock[i]));
#endif
		for (int j = 0; j < (useSymmetries? 8 : 1); ++j) {
			v += LUT[i][ind[j]];
		}
#ifdef _OPENMP
		omp_unset_lock(&(writelock[i]));
#endif
	}

	return v;
}

void updateV(struct board s, float value) {
	for (int i = 0; i < m; ++i) {
		size_t ind[8] = {0};
		indexLUT(ind, i, s);

#ifdef _OPENMP
		omp_set_lock(&(writelock[i]));
#endif
		for (int j = 0; j < (useSymmetries? 8 : 1); ++j) {
			LUT[i][ind[j]] += value;
		}
#ifdef _OPENMP
		omp_unset_lock(&(writelock[i]));
#endif
	}
}

float evaluation(struct board s, enum direction a) {
	size_t score = 0;
	struct board sPrime = gravity(&score, NULL, NULL, s, a);
	return (float) score + getV(sPrime);
}

void learnEvaluation(struct board sPrime, struct board sPrimePrime) {
	bool set[4] = {0};
	getPossibleMoves(sPrimePrime, set);

	enum direction aNext = -1;

	float scorePrime = -INFINITY;
	for (int i = LEFT; i <= UP; ++i) {
		if(!set[i]) {
			continue;
		}

		if (evaluation(sPrimePrime, i) > scorePrime) {
			aNext = i;
			scorePrime = evaluation(sPrimePrime, i);
		}
	}

	size_t scoreNext = 0;
	bool boardChanged = false;
	struct board sPrimeNext = gravity(&scoreNext, NULL, &boardChanged, sPrimePrime, aNext);
	float value = 0;

	if (!isBoardUnmovable(sPrimeNext)) {
		value = (float) scoreNext + getV(sPrimeNext);
	}

	value -= getV(sPrime);

	updateV(sPrime, alpha * value);
}

int play(pcg32_random_t* rng, size_t *score, bool learn) {
	struct board b = create_board(rng);
	uint8_t max = 0;

	while(!isBoardUnmovable(b)) {
		bool set[4] = {0};
		getPossibleMoves(b, set);

		enum direction a = -1;
		float scorePrime = -INFINITY;

		for (int i = LEFT; i <= UP; ++i) {
			if(!set[i]) {
				continue;
			}

			if (evaluation(b, i) > scorePrime) {
				a = i;
				scorePrime = evaluation(b, i);
			}
		}

		struct board sPrime = gravity(score, &max, NULL, b, a);
		struct board sPrimePrime;
		bool couldAdd = false;
		sPrimePrime = create_elem(rng, &couldAdd, sPrime);

		if(!couldAdd) {
			perror("Couldn't add");
			exit(-1);
		}

		if (learn && !isBoardUnmovable(sPrimePrime)) {
			learnEvaluation(sPrime, sPrimePrime);
		}

		b = sPrimePrime;
	}

	return max;
}

void runModel(const size_t iterations, const bool train) {
	printf("0/%zu", iterations);

	size_t performance = 0;
	size_t ratio = 0;
	int maxTile = 0;

#pragma omp parallel for default(none) shared(iterations, train, performance, ratio, maxTile)
	for (size_t i = 0; i < iterations; ++i) {
		pcg32_random_t rng;
		pcg32_srandom_r(&rng, 42, i);

		int max = play(&rng, &performance, train);//This updates the global LUT table

		if (max > maxTile) {
			maxTile = max;
		}

		if(max >= 11) {
			ratio++;
		}

#ifndef _OPENMP
		printf("\r%lu/%zu, avg score: %f, ratio %f, maxTile %d", i + 1, iterations, (float) performance / (i + 1), (float) ratio / (i+1), 1 << maxTile);
#endif
	}

#ifdef _OPENMP
	printf("\r%lu/%zu, avg score: %f, ratio %f, maxTile %d", iterations, iterations, (float) performance / iterations, (float) ratio / iterations, 1 << maxTile);
#endif
}

int main() {
	initLUT();

#ifdef _OPENMP
	for (int i = 0; i < m; ++i) {
		omp_init_lock(&(writelock[i]));
	}
#endif

	runModel(5000, true);//train

	printf("\n");

	runModel(1000, false);//test

	printf("\n");

	while(true) {
		printf("Insert from left to right, from top to bottom, separated by space or enter:\n");

		struct board b;

		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				char v[15];
				int r = scanf("%s", v);
				long l = strtol(v, NULL, 10);
				b.data[j][i] = (int) log2l(l);
			}
		}

		enum direction a = -1;
		float scorePrime = 0;

		for (int i = LEFT; i <= UP; ++i) {
			if (evaluation(b, i) > scorePrime) {
				a = i;
				scorePrime = evaluation(b, i);
			}
		}

		switch (a) {
			case UP:
				printf("\nUp\n");
				break;
			case DOWN:
				printf("\nDown\n");
				break;
			case LEFT:
				printf("\nLeft\n");
				break;
			case RIGHT:
				printf("\nRight\n");
				break;
			default:
				perror("Error");
				exit(-1);
				break;
		}
	}

#ifdef _OPENMP
	for (int i = 0; i < m; ++i) {
		omp_destroy_lock(&(writelock[i]));
	}
#endif

	return 0;
}

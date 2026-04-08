#include "data_gen.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int generate_input_file(const char *path, int L, unsigned seed) {
	if (L < 12000) {
		fprintf(stderr, "L must be >= 12000 per assignment.\n");
		return -1;
	}
	if (L + HIDDEN_KEY_COUNT > INT_MAX / 2) {
		fprintf(stderr, "L too large.\n");
		return -1;
	}
	int total = L + HIDDEN_KEY_COUNT;
	int *buf = (int *)calloc((size_t)total, sizeof(int));
	if (!buf)
		return -1;

	srand(seed);
	for (int i = 0; i < L; i++)
		buf[i] = 1 + (rand() % 1000000);

	/* Pick L distinct slot indices among total positions for positives; rest get keys */
	int *slots = (int *)malloc((size_t)total * sizeof(int));
	if (!slots) {
		free(buf);
		return -1;
	}
	for (int i = 0; i < total; i++)
		slots[i] = i;
	for (int i = total - 1; i > 0; i--) {
		int j = rand() % (i + 1);
		int t = slots[i];
		slots[i] = slots[j];
		slots[j] = t;
	}
	/* First L slots in shuffled order hold positives (already in buf[0..L-1] order wrong) */
	int *out = (int *)calloc((size_t)total, sizeof(int));
	if (!out) {
		free(slots);
		free(buf);
		return -1;
	}
	for (int i = 0; i < L; i++)
		out[slots[i]] = buf[i];
	for (int i = L; i < total; i++) {
		int v = HIDDEN_MIN + (rand() % (HIDDEN_MAX - HIDDEN_MIN + 1));
		out[slots[i]] = v;
	}

	FILE *fp = fopen(path, "w");
	if (!fp) {
		perror(path);
		free(out);
		free(slots);
		free(buf);
		return -1;
	}
	for (int i = 0; i < total; i++) {
		if (fprintf(fp, "%d", out[i]) < 0) {
			fclose(fp);
			free(out);
			free(slots);
			free(buf);
			return -1;
		}
		if (i + 1 < total)
			fputc('\n', fp);
	}
	fclose(fp);
	free(out);
	free(slots);
	free(buf);
	return 0;
}

int *load_file_to_array(const char *path, int *out_n) {
	FILE *fp = fopen(path, "r");
	if (!fp) {
		perror(path);
		return NULL;
	}
	int cap = 8192;
	int n = 0;
	int *arr = (int *)malloc((size_t)cap * sizeof(int));
	if (!arr) {
		fclose(fp);
		return NULL;
	}
	int v;
	while (fscanf(fp, "%d", &v) == 1) {
		if (n >= cap) {
			cap *= 2;
			int *na = (int *)realloc(arr, (size_t)cap * sizeof(int));
			if (!na) {
				free(arr);
				fclose(fp);
				return NULL;
			}
			arr = na;
		}
		arr[n++] = v;
	}
	fclose(fp);
	*out_n = n;
	return arr;
}

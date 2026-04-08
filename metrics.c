#include "metrics.h"

#include <limits.h>
#include <string.h>

static int is_hidden_key(int v) {
	return v >= HIDDEN_MIN && v <= HIDDEN_MAX;
}

void compute_segment(const int *arr, int lo, int hi, int global_offset, SegmentResult *out) {
	memset(out, 0, sizeof(*out));
	if (lo >= hi) {
		out->max = INT_MIN;
		return;
	}
	out->max = arr[lo];
	out->sum = 0;
	out->count = hi - lo;
	for (int i = lo; i < hi; i++) {
		int x = arr[i];
		if (x > out->max)
			out->max = x;
		out->sum += x;
		if (is_hidden_key(x)) {
			if (out->hidden_pos_n < (int)(sizeof(out->hidden_pos) / sizeof(out->hidden_pos[0]))) {
				out->hidden_pos[out->hidden_pos_n++] = global_offset + (i - lo);
			}
			out->hidden_found++;
		}
	}
}

#ifndef METRICS_H
#define METRICS_H

#include "common.h"

/* Scan arr[lo, hi) for max, mean (integer avg rounded toward zero), hidden keys. */
void compute_segment(const int *arr, int lo, int hi, int global_offset, SegmentResult *out);

#endif

#ifndef DATA_GEN_H
#define DATA_GEN_H

#include "common.h"

/* Create text file with L positive integers and 150 hidden negatives [-100,-1] at random positions. */
int generate_input_file(const char *path, int L, unsigned seed);

/* Load integers from whitespace-separated text file into malloc'd array; *out_n set to count. */
int *load_file_to_array(const char *path, int *out_n);

#endif

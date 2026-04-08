/*
 * ECE 434 Sp26 — Project 1 Part 1 — shared definitions
 * Resources (per assignment):
 *   https://www.tutorialspoint.com/cprogramming/c_command_line_arguments.htm
 *   https://www.cprogramming.com/tutorial/c-tutorial.html
 *   https://www.gnu.org/software/libc/manual/html_node/Pipes-and-FIFOs.html
 *   https://www.gnu.org/software/libc/manual/html_node/Creating-a-Process.html
 *   https://www.gnu.org/software/libc/manual/html_node/Process-Completion.html
 */
#ifndef COMMON_H
#define COMMON_H

#include <limits.h>
#include <stddef.h>
#include <time.h>

#define PROGRAM_TAG "ECE 434 Sp26"
#define MAX_PN 50
#define HIDDEN_KEY_COUNT 150
#define HIDDEN_MIN (-100)
#define HIDDEN_MAX (-1)

#define TRACE_CSV "process_trace.csv"
#define POLL_TIMEOUT_MS 30000
#define MAX_BRANCHING 5

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct {
	long long sum;
	int max;
	int count;
	int hidden_found;
	/* global indices in original array where hidden keys (-1..-100) were seen */
	int hidden_pos[256];
	int hidden_pos_n;
} SegmentResult;

typedef struct {
	int start;
	int end;
} Range;

typedef struct {
	struct timespec t0;
	struct timespec t1;
	unsigned long bytes_via_pipe;
	size_t elems;
} ProcStats;

#endif

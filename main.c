/*
 * ECE 434 Sp26 — Project 1 Part 1 (Linux: processes, pipes, poll, waitpid)
 *
 * CLI:  ./ece434_proj1 L H PN BRANCH_IN_2_5 [--generate [SEED]] [--sleep SEC] INPUT.txt OUTPUT.txt
 * Docs: https://www.tutorialspoint.com/cprogramming/c_command_line_arguments.htm
 *       https://www.gnu.org/software/libc/manual/html_node/Pipes-and-FIFOs.html
 *       https://www.gnu.org/software/libc/manual/html_node/Creating-a-Process.html
 *       https://www.gnu.org/software/libc/manual/html_node/Process-Completion.html
 */
#define _POSIX_C_SOURCE 200809L

#include "common.h"
#include "data_gen.h"
#include "trace.h"
#include "tree_proc.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static void usage(const char *argv0) {
	fprintf(stderr,
		"Usage: %s L H PN BRANCH INPUT OUTPUT\n"
		"       L>=12000, H=how many hidden keys to hunt (info), PN process count 1..50, BRANCH in {2,3,5}\n"
		"Options before INPUT/OUTPUT:\n"
		"  --generate [SEED]   build INPUT with L positives + 150 hidden keys\n"
		"  --sleep SEC         leaf sleep for observing pstree (default 0)\n",
		argv0);
}

static int parse_int(const char *s, int *out) {
	char *end = NULL;
	long v = strtol(s, &end, 10);
	if (!end || *end || v < INT_MIN || v > INT_MAX)
		return -1;
	*out = (int)v;
	return 0;
}

int main(int argc, char **argv) {
	int L = 12000;
	int H = 10;
	int PN = 4;
	int branch = 2;
	int gen = 0;
	unsigned seed = 0;
	int leaf_sleep = 0;
	const char *in_path = "input.txt";
	const char *out_path = "output.txt";

	if (argc >= 6) {
		int ai = 1;
		if (parse_int(argv[ai++], &L) || L < 12000)
			return (usage(argv[0]), 1);
		if (parse_int(argv[ai++], &H) || H < 0)
			return (usage(argv[0]), 1);
		if (parse_int(argv[ai++], &PN) || PN < 1 || PN > MAX_PN)
			return (usage(argv[0]), 1);
		if (parse_int(argv[ai++], &branch) || (branch != 2 && branch != 3 && branch != 5))
			return (usage(argv[0]), 1);
		while (ai < argc - 2) {
			if (strcmp(argv[ai], "--generate") == 0) {
				gen = 1;
				ai++;
				if (ai < argc - 2 && argv[ai][0] != '-') {
					unsigned long sv = strtoul(argv[ai], NULL, 10);
					seed = (unsigned)sv;
					ai++;
				} else {
					seed = (unsigned)time(NULL);
				}
			} else if (strcmp(argv[ai], "--sleep") == 0 && ai + 1 < argc - 2) {
				if (parse_int(argv[ai + 1], &leaf_sleep) || leaf_sleep < 0)
					return (usage(argv[0]), 1);
				ai += 2;
			} else {
				fprintf(stderr, "Unknown option: %s\n", argv[ai]);
				return (usage(argv[0]), 1);
			}
		}
		in_path = argv[argc - 2];
		out_path = argv[argc - 1];
	} else if (argc == 1) {
		static char inbuf[4096];
		static char outbuf[4096];
		printf("Enter L (>=12000), H, PN (1-%d), branch (2,3,5): ", MAX_PN);
		if (scanf("%d%d%d%d", &L, &H, &PN, &branch) != 4)
			return 1;
		printf("Input file path: ");
		if (scanf("%4095s", inbuf) != 1)
			return 1;
		in_path = inbuf;
		printf("Output file path: ");
		if (scanf("%4095s", outbuf) != 1)
			return 1;
		out_path = outbuf;
		if (L < 12000 || PN < 1 || PN > MAX_PN || (branch != 2 && branch != 3 && branch != 5))
			return (usage(argv[0]), 1);
	} else {
		usage(argv[0]);
		return 1;
	}

	if (!seed)
		seed = (unsigned)time(NULL);
	if (gen) {
		if (generate_input_file(in_path, L, seed) != 0)
			return 1;
	}

	int n = 0;
	int *arr = load_file_to_array(in_path, &n);
	if (!arr || n <= 0)
		return 1;

	PlanNode plan[MAX_PN];
	int pn = build_process_plan(n, PN, branch, plan, MAX_PN);
	if (pn < 0) {
		fprintf(stderr, "Failed to build process plan.\n");
		free(arr);
		return 1;
	}

	unlink(TRACE_CSV);
	trace_init_file(TRACE_CSV);

	FILE *fout = fopen(out_path, "w");
	if (!fout) {
		perror(out_path);
		free(arr);
		return 1;
	}

	SegmentResult final;
	memset(&final, 0, sizeof(final));
	printf("%s: root loading %d integers; plan uses %d tree nodes, branch=%d, PN=%d, H=%d\n",
	       PROGRAM_TAG, n, pn, branch, PN, H);
	fflush(stdout);

	execute_plan_root(arr, n, plan, 0, fout, TRACE_CSV, leaf_sleep, &final);

	fprintf(fout, "Hidden keys encountered in scan: %d (file contains %d; hunt parameter H=%d)\n",
		final.hidden_found, HIDDEN_KEY_COUNT, H);
	printf("%s: Hidden keys encountered in scan: %d (file contains %d; hunt parameter H=%d)\n",
	       PROGRAM_TAG, final.hidden_found, HIDDEN_KEY_COUNT, H);
	fflush(stdout);

	fclose(fout);
	free(arr);
	return 0;
}

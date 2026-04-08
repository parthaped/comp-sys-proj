#ifndef TREE_PROC_H
#define TREE_PROC_H

#include <stdio.h>

#include "common.h"

typedef struct {
	int lo, hi;
	int exit_code;
	int subtree;
	int child[MAX_BRANCHING];
	int nchild;
} PlanNode;

/* Build a BFS-style partition plan: total processes = pn, branching factor 2..5. Returns plan count or -1. */
int build_process_plan(int n_elems, int pn, int branching, PlanNode *plan, int cap);

/*
 * Execute plan from root index 0. arr is shared read-only via fork COW.
 * parent_wr == -1 for root (does not exit on leaf/internal; returns in *final_out).
 */
void execute_plan_root(int *arr, int n, PlanNode *plan, int root_index, FILE *result_out,
		       const char *trace_path, int leaf_sleep_sec, SegmentResult *final_out);

#endif

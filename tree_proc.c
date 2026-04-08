#define _POSIX_C_SOURCE 200809L

#include "tree_proc.h"

#include "metrics.h"
#include "trace.h"
#include "wait_status.h"

#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static int build_rec(int lo, int hi, int subtree, int branching, PlanNode *plan, int cap, int *nplan,
		     int *next_code) {
	if (*nplan >= cap)
		return -1;
	int id = (*nplan)++;
	plan[id].lo = lo;
	plan[id].hi = hi;
	plan[id].subtree = subtree;
	plan[id].exit_code = (*next_code)++;
	if (plan[id].exit_code > 255)
		plan[id].exit_code = 2 + (id % 200);
	plan[id].nchild = 0;
	if (subtree <= 1 || hi - lo <= 1)
		return id;
	int below = subtree - 1;
	int k = branching;
	if (k > below)
		k = below;
	if (k < 1)
		return id;
	int sizes[MAX_BRANCHING];
	int base = below / k;
	int rem = below % k;
	for (int i = 0; i < k; i++)
		sizes[i] = base + (i < rem);
	int len = hi - lo;
	for (int i = 0; i < k; i++) {
		int clo = lo + (i * len) / k;
		int chi = lo + ((i + 1) * len) / k;
		int cid = build_rec(clo, chi, sizes[i], branching, plan, cap, nplan, next_code);
		if (cid < 0)
			return -1;
		plan[id].child[plan[id].nchild++] = cid;
	}
	return id;
}

int build_process_plan(int n_elems, int pn, int branching, PlanNode *plan, int cap) {
	(void)n_elems;
	if (branching < 2 || branching > 5 || pn < 1 || pn > MAX_PN)
		return -1;
	int nplan = 0;
	int code = 1;
	if (build_rec(0, n_elems, pn, branching, plan, cap, &nplan, &code) < 0)
		return -1;
	return nplan;
}

static void print_pstree_snapshot(pid_t me) {
	char cmd[256];
	snprintf(cmd, sizeof(cmd), "pstree -p %ld 2>/dev/null | head -40", (long)me);
	printf("%s: --- pstree -p %ld ---\n", PROGRAM_TAG, (long)me);
	fflush(stdout);
	if (system(cmd) < 0)
		perror("system/pstree");
	fflush(stdout);
}

static void run_node(int *arr, int n, PlanNode *plan, int nid, int parent_wr,
		     const char *trace_path, int leaf_sleep, SegmentResult *root_out) {
	(void)n;
	PlanNode *p = &plan[nid];
	pid_t mypid = getpid();
	pid_t ppid = getppid();
	struct timespec t0, t1;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	unsigned long bytes_ipc = 0;

	printf("%s: I'm process %d with return arg %d and my parent is %d.\n", PROGRAM_TAG,
	       (int)mypid, p->exit_code, (int)ppid);
	fflush(stdout);
	printf("%s: phase start pid %d range [%d,%d) subtree_nodes=%d\n", PROGRAM_TAG, (int)mypid,
	       p->lo, p->hi, p->subtree);
	fflush(stdout);

	if (p->nchild == 0) {
		SegmentResult res;
		compute_segment(arr, p->lo, p->hi, p->lo, &res);
		printf("%s: phase local compute done pid %d (leaf)\n", PROGRAM_TAG, (int)mypid);
		fflush(stdout);
		if (leaf_sleep > 0)
			sleep((unsigned)leaf_sleep);
		for (int i = 0; i < res.hidden_pos_n; i++) {
			printf("%s: I am process %d with return arg %d. I found the hidden key in position A[%d].\n",
			       PROGRAM_TAG, (int)mypid, p->exit_code, res.hidden_pos[i]);
			fflush(stdout);
		}
		if (parent_wr >= 0) {
			ssize_t w = write(parent_wr, &res, sizeof(res));
			if (w != (ssize_t)sizeof(res))
				perror("write leaf result");
			bytes_ipc += (unsigned long)(w > 0 ? (unsigned long)w : 0U);
		}
		clock_gettime(CLOCK_MONOTONIC, &t1);
		trace_write_row(trace_path, (int)mypid, (int)ppid, p->exit_code, &t0, &t1,
				(size_t)(p->hi - p->lo), bytes_ipc);
		if (parent_wr >= 0) {
			printf("%s: phase exiting leaf pid %d exit %d\n", PROGRAM_TAG, (int)mypid,
			       p->exit_code);
			fflush(stdout);
			exit(p->exit_code & 0xFF);
		}
		if (root_out)
			*root_out = res;
		return;
	}

	int k = p->nchild;
	int pr[MAX_BRANCHING][2];
	pid_t cpids[MAX_BRANCHING];

	for (int i = 0; i < k; i++) {
		if (pipe(pr[i]) < 0) {
			perror("pipe");
			if (parent_wr >= 0)
				exit(1);
			return;
		}
	}

	printf("%s: phase fork %d children pid %d\n", PROGRAM_TAG, k, (int)mypid);
	fflush(stdout);
	print_pstree_snapshot(mypid);

	for (int i = 0; i < k; i++) {
		pid_t pid = fork();
		if (pid < 0) {
			perror("fork");
			if (parent_wr >= 0)
				exit(1);
			return;
		}
		if (pid == 0) {
			for (int j = 0; j < k; j++) {
				close(pr[j][0]);
				if (j != i)
					close(pr[j][1]);
			}
			if (parent_wr >= 0)
				close(parent_wr);
			run_node(arr, n, plan, p->child[i], pr[i][1], trace_path, leaf_sleep, NULL);
			_exit(1);
		}
		cpids[i] = pid;
		close(pr[i][1]);
	}

	struct pollfd *pfds = (struct pollfd *)calloc((size_t)k, sizeof(struct pollfd));
	int *done = (int *)calloc((size_t)k, sizeof(int));
	SegmentResult *partial =
		(SegmentResult *)calloc((size_t)k, sizeof(SegmentResult));
	if (!pfds || !done || !partial) {
		perror("calloc");
		_exit(1);
	}
	for (int i = 0; i < k; i++) {
		pfds[i].fd = pr[i][0];
		pfds[i].events = POLLIN;
	}
	int remaining = k;
	while (remaining > 0) {
		int r = poll(pfds, (nfds_t)k, POLL_TIMEOUT_MS);
		if (r < 0) {
			perror("poll");
			break;
		}
		if (r == 0) {
			printf("%s: poll timed out (%d ms); sending SIGKILL to unfinished children\n",
			       PROGRAM_TAG, POLL_TIMEOUT_MS);
			fflush(stdout);
			for (int i = 0; i < k; i++) {
				if (!done[i])
					kill(cpids[i], SIGKILL);
			}
			break;
		}
		for (int i = 0; i < k; i++) {
			if (done[i])
				continue;
			short rev = pfds[i].revents;
			if (rev & POLLIN) {
				ssize_t nb = read(pr[i][0], &partial[i], sizeof(SegmentResult));
				if (nb == (ssize_t)sizeof(SegmentResult)) {
					done[i] = 1;
					remaining--;
					bytes_ipc += (unsigned long)nb;
				}
			} else if (rev & (POLLERR | POLLHUP | POLLNVAL)) {
				/* try drain once */
				ssize_t nb = read(pr[i][0], &partial[i], sizeof(SegmentResult));
				if (nb == (ssize_t)sizeof(SegmentResult)) {
					done[i] = 1;
					remaining--;
					bytes_ipc += (unsigned long)nb;
				}
			}
		}
	}

	for (int i = 0; i < k; i++)
		close(pr[i][0]);

	SegmentResult merged;
	memset(&merged, 0, sizeof(merged));
	merged.max = INT_MIN;
	long long sum = 0;
	int count = 0;
	for (int i = 0; i < k; i++) {
		if (!done[i])
			continue;
		if (partial[i].max > merged.max)
			merged.max = partial[i].max;
		sum += partial[i].sum;
		count += partial[i].count;
		merged.hidden_found += partial[i].hidden_found;
		for (int j = 0; j < partial[i].hidden_pos_n; j++) {
			if (merged.hidden_pos_n < (int)(sizeof(merged.hidden_pos) / sizeof(merged.hidden_pos[0])))
				merged.hidden_pos[merged.hidden_pos_n++] = partial[i].hidden_pos[j];
		}
	}
	merged.sum = sum;
	merged.count = count;

	printf("%s: phase waitpid all direct children pid %d\n", PROGRAM_TAG, (int)mypid);
	fflush(stdout);
	for (int i = 0; i < k; i++) {
		int st = 0;
		pid_t w = waitpid(cpids[i], &st, 0);
		if (w > 0)
			explain_wait_status(w, st);
		fflush(stdout);
	}

	clock_gettime(CLOCK_MONOTONIC, &t1);
	trace_write_row(trace_path, (int)mypid, (int)ppid, p->exit_code, &t0, &t1,
			(size_t)(p->hi - p->lo), bytes_ipc);

	if (parent_wr >= 0) {
		ssize_t w = write(parent_wr, &merged, sizeof(merged));
		if (w != (ssize_t)sizeof(merged))
			perror("write internal result");
		free(pfds);
		free(done);
		free(partial);
		printf("%s: phase exiting internal pid %d exit %d\n", PROGRAM_TAG, (int)mypid,
		       p->exit_code);
		fflush(stdout);
		exit(p->exit_code & 0xFF);
	}

	if (root_out)
		*root_out = merged;

	free(pfds);
	free(done);
	free(partial);
}

void execute_plan_root(int *arr, int n, PlanNode *plan, int root_index, FILE *result_out,
		       const char *trace_path, int leaf_sleep_sec, SegmentResult *final_out) {
	struct timespec troot0, troot1;
	clock_gettime(CLOCK_MONOTONIC, &troot0);
	run_node(arr, n, plan, root_index, -1, trace_path, leaf_sleep_sec, final_out);
	clock_gettime(CLOCK_MONOTONIC, &troot1);

	if (!final_out)
		return;
	int avg = final_out->count ? (int)(final_out->sum / final_out->count) : 0;
	printf("Max=%d, Avg=%d\n", final_out->max, avg);
	fflush(stdout);
	if (result_out) {
		fprintf(result_out, "Max=%d, Avg=%d\n", final_out->max, avg);
		fflush(result_out);
	}
	double tr = (double)(troot1.tv_sec - troot0.tv_sec) +
		    (double)(troot1.tv_nsec - troot0.tv_nsec) / 1e9;
	printf("%s: root total runtime %.6f s\n", PROGRAM_TAG, tr);
	fflush(stdout);
}

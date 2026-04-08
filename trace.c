#include "trace.h"

#include <stdio.h>

void trace_init_file(const char *csv_path) {
	FILE *fp = fopen(csv_path, "w");
	if (!fp) {
		perror(csv_path);
		return;
	}
	fprintf(fp, "pid,ppid,return_arg,runtime_s,elems,bytes_ipc\n");
	fclose(fp);
}

void trace_write_row(const char *csv_path, int pid, int ppid, int return_arg,
		     const struct timespec *t0, const struct timespec *t1,
		     size_t elems_processed, unsigned long bytes_ipc) {
	FILE *fp = fopen(csv_path, "a");
	if (!fp) {
		perror(csv_path);
		return;
	}
	double runtime = (double)(t1->tv_sec - t0->tv_sec) +
			 (double)(t1->tv_nsec - t0->tv_nsec) / 1e9;
	fprintf(fp, "%d,%d,%d,%.6f,%zu,%lu\n", pid, ppid, return_arg, runtime,
		elems_processed, bytes_ipc);
	fclose(fp);
}

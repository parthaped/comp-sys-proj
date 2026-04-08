#ifndef TRACE_H
#define TRACE_H

#include "common.h"

#include <stdio.h>
#include <time.h>

void trace_init_file(const char *csv_path);
void trace_write_row(const char *csv_path, int pid, int ppid, int return_arg,
		     const struct timespec *t0, const struct timespec *t1,
		     size_t elems_processed, unsigned long bytes_ipc);

#endif

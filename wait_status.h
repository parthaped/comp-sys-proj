#ifndef WAIT_STATUS_H
#define WAIT_STATUS_H

#include <sys/types.h>

/* Decode child status from waitpid(); print human-readable reason (GNU libc / Stevens style). */
void explain_wait_status(pid_t pid, int status);

#endif

#define _POSIX_C_SOURCE 200809L

#include "wait_status.h"
#include "common.h"

#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

void explain_wait_status(pid_t pid, int status) {
	printf("%s: waitpid result for pid %ld: ", PROGRAM_TAG, (long)pid);
	if (WIFEXITED(status)) {
		printf("exited normally with status %d\n", WEXITSTATUS(status));
	} else if (WIFSIGNALED(status)) {
		const char *sig = strsignal(WTERMSIG(status));
		printf("terminated by signal %d (%s)%s\n", WTERMSIG(status), sig ? sig : "?",
		       WCOREDUMP(status) ? ", core dumped" : "");
	} else if (WIFSTOPPED(status)) {
		const char *sig = strsignal(WSTOPSIG(status));
		printf("stopped by signal %d (%s)\n", WSTOPSIG(status), sig ? sig : "?");
	} else if (WIFCONTINUED(status)) {
		printf("continued\n");
	} else {
		printf("unknown status 0x%x\n", status);
	}
}

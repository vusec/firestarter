#define _GNU_SOURCE
#define LTCKPT_CHECKPOINT_METHOD fork
#include "../ltckpt_local.h"
LTCKPT_CHECKPOINT_METHOD_ONCE();
LTCKPT_DECLARE_EMPTY_STORE_HOOKS();

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>
#include <sys/syscall.h>

static pid_t pid = 0;
#define STACKSIZE (4*1024*1024)

char *ltckpt_fork_child_stack;


static int ltckpt_fork_child(void *arg)
{
	pause();
	exit(1);
}

static inline pid_t ltckpt_fork()
{
	/* clone acutally implements fork semantic but without SIGCHILD flag 
	 * sigchild signals will be sent to the parrent. (So we are the only one 
	 * seeing that the child died.) */
    return clone(ltckpt_fork_child, ltckpt_fork_child_stack + STACKSIZE, 0, NULL);
}

/* what the fork! */
LTCKPT_DECLARE_TOP_OF_THE_LOOP_HOOK()
{
	int ret;

	CTX_NEW_TOL_OR_RETURN();

	if (pid) {
		ret = kill(pid,SIGKILL);
		if (ret != 0) {
			ltckpt_panic("kill failed\n");
		}
		waitpid(pid, NULL, __WCLONE);
	}
	else {
		ltckpt_fork_child_stack = malloc(STACKSIZE);
		if (!ltckpt_fork_child_stack) {
			ltckpt_panic("malloc failed\n");
		}
	}
	pid = ltckpt_fork();
	if (pid <= 0) {
		ltckpt_panic("could not fork\n");
	}

	CTX_NEW_CHECKPOINT();
}

LTCKPT_DECLARE_BEFORE_EXIT_HOOK()
{
	(void)(status);
	if (pid) {
		kill(pid,SIGKILL);
		waitpid(pid, NULL, __WCLONE);
	}
}


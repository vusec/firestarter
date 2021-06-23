#define _GNU_SOURCE
#include "ltckpt_local.h"

#include <stdlib.h>

#define LTCKPT_EXIT_WRAPPER(E) \
	void E(int status); \
	LTCKPT_NORET_WRAPPER(void, E, \
		LTCKPT_CONCAT(int status), \
		LTCKPT_CONCAT(status), \
		ltckpt_before_exit(status); \
	)

void ltckpt_before_exit(int status)
{
	if (CTX(exited))
		return;

	if (CTX(atexit_dump))
		ltckpt_ctx_print();

	if (CONF(before_exit_hook))
		CONF(before_exit_hook)(status);
	CTX(exited)=1;
}

/* exit() */
LTCKPT_EXIT_WRAPPER(exit)

/* _exit() */
LTCKPT_EXIT_WRAPPER(_exit)

/* _Exit() */
LTCKPT_EXIT_WRAPPER(_Exit)

void __ltckpt_before_exit(void)
{
	ltckpt_before_exit(0);
}

void __attribute__((constructor)) __ltckpt_before_exit_init()
{
	atexit(__ltckpt_before_exit);
}

/*
 * Override assertion failure behavior.
 */
const int ltckpt_hang_on_assert = LTCKPT_HANG_ON_ASSERT;
void __assert_fail(const char * assertion, const char * file,
        unsigned int line, const char * function)
{
        ltckpt_printf_error("ERROR: Assertion \"%s\" failed in file %s, line %u, function %s\n",
            assertion, file, line, function);
        while (ltckpt_hang_on_assert) {
            usleep(1000*200);
        }
        abort();
}


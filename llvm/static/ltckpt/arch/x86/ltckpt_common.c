#include <dlfcn.h>
#include <elf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "../../ltckpt_common.h"
#include "../../ltckpt_mechanism.h"

extern char **environ;

/*
 * Early init function.
 */
int __libc_start_main(int (*main) (int, char * *, char * *),
                      int argc,
					  char * * ubp_av,
					  void (*init) (void),
					  void (*fini) (void),
					  void (*rtld_fini) (void),
					  void * stack_end)
{
	/* Get configuration from the pass. */
	ltckpt_conf_setup();

	if(CONF(early_init_hook)) {

		ltckpt_early_init_data_t data = {
			.main      = main,
			.argc      = argc,
			.ubp_av    = ubp_av,
			.init      = init,
			.fini      = fini,
			.rtld_fini = rtld_fini,
			.stack_end = stack_end,
		};

		ltckpt_debug_print("calling early init hook\n");

		CONF(early_init_hook)(&data);

	} else {

		ltckpt_debug_print("no early init_hook\n");
		void* volatile handle = dlopen("/lib/i386-linux-gnu/libc.so.6", RTLD_LAZY | RTLD_GLOBAL);

		if (!handle) {
			ltckpt_panic("LTCKPT: could not find libc\n");
		}

		void *entry  = dlsym(handle, "__libc_start_main");
		if (entry == 0) {
			ltckpt_panic("could not lookup __libc_stat_main");
		}

		typeof (__libc_start_main) *next_start_main = (typeof(&__libc_start_main))entry;
		return next_start_main(main, argc, ubp_av, init, fini, rtld_fini, stack_end);
	}
	return 0;
}

/**
 * This functions sets up a new stack using the information
 * contained in the old one. It returns the new stack address
 **/
ltckpt_va_t
ltckpt_setup_stack(
	ltckpt_early_init_data_t *eid,
	ltckpt_va_t new_stack_top)
{
	/**
     * ------------------------------
     *  [ 0 ]  <-- top of the stack
     *  [ envp strings ]
     *  [ argv strings ]
     *  [ 0 ]
     *  [ auxv ]
     *  [ 0 ]
     *  [ envp ]
     *  [ 0 ]
     *  [ argv ]
     *  [ argc ] <-- stack pointer
     * -----------------------------
	 **/

	/* strategy: locate top of the stack copy values over and than adjust
	   pointers of envp, and argv
	*/
	char **  stack_top_tmp = eid->ubp_av;
	char  *  last_string = 0;

	unsigned long argv_start, argv_end, env_start, env_end;

	/* ARGV (Pointer to arguments)*/
	argv_start =(unsigned long) *stack_top_tmp;
	while (*stack_top_tmp != 0) {
		if (last_string < *stack_top_tmp) {
			last_string = *stack_top_tmp;
		}
		stack_top_tmp++;
	}

	stack_top_tmp++;

	argv_end = (unsigned long) *stack_top_tmp;

	/* env */
	environ = stack_top_tmp;

	env_start = (unsigned long) *stack_top_tmp;

	while (*stack_top_tmp != 0) {
		if (last_string < *stack_top_tmp) {
			last_string = *stack_top_tmp;
		}
		stack_top_tmp++;
	}

	env_end = (unsigned long) *stack_top_tmp;

	/* go to end of last string.*/
	while (*last_string++ != 0);

	/* there seems to be another string on the stack */
	while (*last_string++ != 0);

	/* the stack should end here... */
	uint32_t  top_of_stack = (uint32_t)last_string;

    /* acutally here: */
	top_of_stack = (top_of_stack + 0x3) & ~(0x3);

	/* check for top stack marker */
	if ( *(uint32_t*)top_of_stack != 0) {
		ltckpt_panic("LTCKPT: couldn't locate top of the stack\n");
	}

	uint32_t  stack_offset =
		top_of_stack + 4 - new_stack_top;

	/* fix the pointers to the new argv and env values */
	char  **tmp = eid->ubp_av;

	while (*tmp != 0)
		*tmp++-=stack_offset; /* for argvs */

	tmp++;

	while (*tmp != 0)
		*tmp++-=stack_offset; /* ... and for envs */

	/* now we can copy the stack */

	uint32_t  from = (uint32_t)eid->ubp_av-4; /* argc */

	size_t stack_size= top_of_stack - from;

	uint32_t *newstack =
		(void*) ((uint32_t ) new_stack_top - 4 - stack_size);

	memcpy((void *)newstack,(void *)from, stack_size);

	/* now that we have done that we have to prepare the new statck for
	   __libc_start_main */

	/* first it wants garbage */
	newstack--;
	newstack--;

	/* than the stack_end */
	*newstack-- = (unsigned) eid->stack_end-stack_offset;

	/* than the rtld_fini */
	*newstack-- = (uint32_t)eid->rtld_fini;

	/* than the fini */
	*newstack-- = (uint32_t)eid->fini;
	*newstack-- = (uint32_t)eid->init;
	*newstack-- = (uint32_t)eid->ubp_av - stack_offset;
	*newstack-- = eid->argc;
	*newstack   = (uint32_t)eid->main;
	environ     = (void *)(((uint32_t)environ) - stack_offset);

if (CTX(allow_prctl)) {

	/* inform the Linux kernel about the changes we made */

	/* TODO: we should check if own CAP_SYS_RESOURCE here and fall
	 *       back to keeping the old stack arround for setcmdline.
	 */

	/* not all prctl calls are exposed in the userland header
	 * so define the ones that are missing */

#ifndef PR_SET_MM_ARG_START
# define PR_SET_MM_ARG_START		8
#endif
#ifndef PR_SET_MM_ARG_END
# define PR_SET_MM_ARG_END		9
#endif
#ifndef PR_SET_MM_ENV_START
# define PR_SET_MM_ENV_START		10
#endif
#ifndef PR_SET_MM_ENV_END
# define PR_SET_MM_ENV_END		11
#endif
#ifndef PR_SET_MM_AUXV
# define PR_SET_MM_AUXV			12
#endif
#ifndef PR_SET_MM_EXE_FILE
# define PR_SET_MM_EXE_FILE		13
#endif

	int ret;

	if ( (ret= prctl(PR_SET_MM, PR_SET_MM_START_STACK, new_stack_top, 0, 0)) ) {
		ltckpt_panic("ERROR: prctl failed (%d) on line %d in %s\n ", errno, __LINE__, __FILE__);
	}

	if ( (ret= prctl(PR_SET_MM,  PR_SET_MM_ARG_START, argv_start - stack_offset, 0,0)) ) {
		ltckpt_panic("ERROR: prctl failed (%d) on line %d in %s\n ", errno, __LINE__, __FILE__);
	}

	if ( (ret= prctl(PR_SET_MM, PR_SET_MM_ARG_END, argv_end - stack_offset, 0,0)) ) {
		ltckpt_panic("ERROR: prctl failed (%d) on line %d in %s\n ", errno, __LINE__, __FILE__);
	}

	if ( (ret= prctl(PR_SET_MM, PR_SET_MM_ENV_START, env_start - stack_offset, 0,0)) ) {
		ltckpt_panic("ERROR: prctl failed (%d) on line %d in %s\n ", errno, __LINE__, __FILE__);
	}

	if ( (ret= prctl(PR_SET_MM,  PR_SET_MM_ENV_END, env_end - stack_offset, 0,0)) ) {
		ltckpt_panic("ERROR: prctl failed (%d) on line %d in %s\n ", errno, __LINE__, __FILE__);
	}
} /* end of CTX(allow_prctl). */
	return (uint32_t)newstack;
}

/**
 * Switches to a new stack and continiues execution from the address
 * specified in entry
 **/
void ltckpt_start(
	ltckpt_early_init_data_t *eid,
	ltckpt_va_t newstack)
{
	
	void* volatile handle = dlopen("/lib/i386-linux-gnu/libc.so.6", RTLD_LAZY | RTLD_GLOBAL);

	if (!handle) {
		ltckpt_panic("LTCKPT: could not find libc\n");
	}

	void *entry  = dlsym(handle, "__libc_start_main");

	if (entry == 0) {
		ltckpt_panic("could not lookup __libc_stat_main");
	}

	if (newstack) {
		asm volatile ( "xor    %%ebp, %%ebp\n\t"
	      "movl %0, %%esp\n\t"
		  "call *%1"
         :
         : "r"(newstack), "r"(entry) );
	}
}


ltckpt_va_t ltckpt_allocate_stack(ltckpt_va_t start, size_t size)
{
	ltckpt_va_t ret = ltckpt_mmap(start, size,
		LTCKPT_PROT_W | LTCKPT_PROT_R,
		LTCKPT_MAP_FIXED | LTCKPT_MAP_PRIVATE |
			LTCKPT_MAP_NORESERVE | LTCKPT_MAP_STACK);

	if (ret == LTCKPT_MAP_FAILED) {
		ltckpt_panic("could not map shadow stack\n");
	}

	return ret;
}


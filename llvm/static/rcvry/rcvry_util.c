/* Description: Utilities for use in rcvry library
 * All recovery actions are defined here
 *
 * Author : Koustubha Bhat
 * Date   : 17-April-2017
 * Vrije Universiteit, Amsterdam.
 */

#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include "rcvry_common.h"
#include "rcvry_util.h"

#define __NR__clone	56
#define INLINE_SYSCALL(NAME, ARG1, ARG2, ARG3, ARG4)	({		\
	unsigned long int resultvar;						\
	register long int _arg5 asm ("r8") = (long int) ARG4;			\
	register long int _arg4 asm ("rcx") = (long int) ARG3;			\
	register long int _arg3 asm ("rdx") = (long int) ARG2;			\
	register long int _arg2 asm ("rsi") = (long int) ARG1;			\
	register long int _arg1 asm ("rdi") = (long int) NAME;			\
										\
	asm volatile (								\
		"syscall\n\t"							\
		: "=a" (resultvar)						\
		: "0" (NAME), "r" (ARG1), "r" (ARG2), "r" (ARG3), "r" (ARG4) 	\
		: "memory", "cc", "r11", "cx");					\
										\
	(long int) resultvar;							\
})

rcvry_child_state_t rcvry_child_state = RCVRY_CHILD_INVALID;

int rcvry_fork()
{
    /* Using 'clone' directly because the semantics of
       glibc 'clone()' is different, in that it is easier this way
       to start the execution from the point of call, like that of 'fork()'
     */
    pid_t pid;
    pid = INLINE_SYSCALL(__NR__clone,
		 /* flags */ 0, //CLONE_CHILD_SETTID | CLONE_CHILD_CLEARTID,
		 /* child_stack COW */ 0,
		 /* ctid */ NULL,
		 /* newtls */0);
    return pid;
}

void rcvry_child_handler(int signum)
{
   rcvry_print_info("Child received signal: %d", signum);
   exit(RCVRY_CHILD_EXIT_FAULTY);
}

#ifdef RCVRY_WINDOW_PROFILING
void rcvry_prof_count_rcvry_branch_hits(unsigned site_id)
{
    rcvry_info[site_id].rcvry_prof.num_hits_rcvry_branch++;
}

// Following three functions are used to count the number of
// libcall "windows" that we encounter at runtime.

void rcvry_prof_count_libcalls__skipped_nonfaultable(unsigned site_id)
{
    rcvry_info[site_id].rcvry_prof.libcalls__skipped_nonfaultable++;
}

void rcvry_prof_count_libcalls__skipped_nousers(unsigned site_id)
{
    rcvry_info[site_id].rcvry_prof.libcalls__skipped_nousers++;
}

void rcvry_prof_count_libcalls__protected(unsigned site_id)
{
    rcvry_info[site_id].rcvry_prof.libcalls__protected++;
}
#endif

#ifdef RCVRY_WINDOW_PROFILING_BBTRACING
#include <string.h>

extern uint32_t edfi_bbtrace_next_index, edfi_bbtrace_num_bbs;
extern uint32_t edfi_bbtrace_history[], edfi_bbtrace_hashlist[];

static int _qcmp(const void *a, const void *b)
{
	return ( *(unsigned*)a - *(unsigned*)b );
}

static void _merge(unsigned list1[], unsigned *plen1, unsigned list2[], unsigned len2, unsigned maxsize)
{
	int duplicates_found = 0;
	qsort(list2, len2, sizeof(unsigned), _qcmp);

	// If list2 contains what is already in list1, mark it zero. (Zero value is invalid)
	for (unsigned i=0; i < *plen1; i++) {
		for (unsigned j=0; j < len2; j++) {
			if (list2[j] == list1[i]) {
				list2[j] = 0;
				duplicates_found++;
			}
		}
	}
	assert((*plen1) + len2 - duplicates_found <= maxsize);

	// append those from list2 to list1, at the end
	for (unsigned j=0; j < len2; j++) {
		if (0 != list2[j]) {
			list1[(*plen1)++] = list2[j];
		}
	}

	qsort(list1, *plen1, sizeof(unsigned), _qcmp);
	return;
}

void rcvry_prof_bbtrace_flush()
{
extern uint32_t edfi_disable_tracing;
        if (edfi_disable_tracing != 0) {
            return;
        }
	if (0 != rcvry_current_site_id) {
		_merge(rcvry_info[rcvry_current_site_id].rcvry_prof.bbs,
			   &(rcvry_info[rcvry_current_site_id].rcvry_prof.num_bbs),
			   edfi_bbtrace_history, edfi_bbtrace_next_index, RCVRY_BBTRACING_MAXSIZE);
	}
	edfi_bbtrace_next_index = 0;
	memset(&edfi_bbtrace_hashlist[0], 0, edfi_bbtrace_num_bbs * sizeof(unsigned));
}
#endif

#ifndef RCVRY_UTIL_H
#define RCVRY_UTIL_H

typedef enum rcvry_child_state_e {
	RCVRY_CHILD_EXIT_OK = 0,	/* Child is exiting normally */
	RCVRY_CHILD_ERROR,		/* Infra error occurred */
	RCVRY_CHILD_STARTED,		/* Child is executing */
	RCVRY_CHILD_EXIT_FAULTY,	/* Fault occured during child's execution */
	RCVRY_CHILD_INVALID,
	__NUM_RCVRY_CHILD_STATES
} rcvry_child_state_t;

extern rcvry_child_state_t rcvry_child_state;

int rcvry_fork();
void rcvry_child_handler(int signum);
#ifdef RCVRY_WINDOW_PROFILING
void rcvry_prof_count_rcvry_branch_hits(unsigned site_id);
void rcvry_prof_count_libcalls__skipped_nonfaultable(unsigned site_id);
void rcvry_prof_count_libcalls__skipped_nousers(unsigned site_id);
void rcvry_prof_count_libcalls__protected(unsigned site_id);
#endif
#ifdef RCVRY_WINDOW_PROFILING_BBTRACING
void rcvry_prof_bbtrace_flush();
#endif
#endif

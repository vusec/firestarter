/* dEscription: Recovery actions dispatching
 * 
 * Author : Koustubha Bhat
 * Date   : 08-March-2017
 * Vrije Universiteit, Amsterdam.
 */

#include <stdlib.h>
#include <pthread.h>
#include "rcvry_common.h"
#include "rcvry_dispatch.h"
#include "rcvry_actions.h"
#include "../../include/edfi/df/df.h"

#ifdef RCVRY_WINDOW_PROFILING
#define SCRIBE(FPTR, ...)                                 \
          if (NULL != FPTR) {                             \
                  fprintf(FPTR, __VA_ARGS__);             \
                  fflush(FPTR);                           \
          }
#define rcvry_fscribe(...)          	SCRIBE(rcvry_log_fptr, __VA_ARGS__);
#define rcvry_bbtscribe(...)        	SCRIBE(rcvry_bbt_fptr, __VA_ARGS__);
#define rcvry_bbt_startup_scribe(...)   SCRIBE(rcvry_bbt_startup_fptr, __VA_ARGS__);
#define rcvry_actscribe(...)        	SCRIBE(rcvry_act_fptr, __VA_ARGS__);

#define RCVRY_PROFILING_OUTFILE	"/tmp/rcvry_profile_dump.txt"
#define RCVRY_BBTRACE_OUTFILE   "/tmp/rcvry_bbtrace_dump.json"
#define RCVRY_ACTION_OUTFILE     "/tmp/rcvry_action_dump.txt"
#endif

uint32_t  rcvry_libcall_gates[RCVRY_MAX_LIBCALL_SITES] = { LTCKPT_TYPE_TSX };
uint32_t  rcvry_libcall_max_site_id = 0;		// To be initialized by instrumentation
uint32_t  rcvry_current_site_id = 0; 			// 0 is invalid
uint32_t  rcvry_just_noop_for_tsx = 0;
static volatile rcvry_type_t rcvry_dispatched_action = RCVRY_TYPE_INVALID; // Initialized to the invalid value

#ifdef RCVRY_TIME_TO_RECOVER
extern FILE *rcvry_time_fptr;
#endif

#ifdef RCVRY_AUTO_ADAPT
uint32_t rcvry_auto_adapt_sample_size = 0, rcvry_auto_adapt_threshold = 0, rcvry_auto_adapt_scale = 1;
#endif

#ifdef RCVRY_CKPT_DEFAULT_TO_UNDOLOG
uint32_t  rcvry_current_tx_type = LTCKPT_TYPE_UNDOLOG;	// for dynamic switching between tx types
#else
uint32_t  rcvry_current_tx_type = LTCKPT_TYPE_TSX;	// for dynamic switching between tx types
#endif

#ifdef RCVRY_WINDOW_PROFILING
static FILE *rcvry_log_fptr = NULL, *rcvry_bbt_fptr = NULL, *rcvry_act_fptr = NULL, *rcvry_bbt_startup_fptr = NULL;
#endif

#ifdef RCVRY_PRINT_TO_FILE
#define RCVRY_PRINT_TO_FILE_NAME	"/tmp/rcvry.log.txt"
FILE *rcvry_fprint_fptr = NULL;
#endif

rcvry_info_t rcvry_info[RCVRY_MAX_LIBCALL_SITES];
rcvry_action_t rcvry_actions_map[__NUM_RCVRY_TYPES];
char *rcvry_type_strs[] = {
    "RCVRY_TYPE_INVALID",
    "RCVRY_TYPE_DEFAULT",
    "RCVRY_TYPE_NOOP",
    "RCVRY_TYPE_SET_RET_ERRNO",
    "RCVRY_TYPE_SET_RET_ERRNO_EACCES",
    "RCVRY_TYPE_SET_RET_ERRNO_ENOMEM",
    "RCVRY_TYPE_SET_RET_ERRNO_ENOSPC",
    "RCVRY_TYPE_SET_RET_ERRNO_EBADF",
    "RCVRY_TYPE_SET_RET_ERRNO_EINVAL",
    "RCVRY_TYPE_SET_RET_ERRNO_EFAULT",
    "RCVRY_TYPE_SET_RET_ERRNO_EMFILE",
    "RCVRY_TYPE_SET_RET_ERRNO_ZSTREAM_ERROR",
    "RCVRY_TYPE_SET_RET_NULL_ERRNO",
    "RCVRY_TYPE_SET_RET_NULL_ERRNO_ENOMEM",
    "RCVRY_TYPE_SET_RET_NULL_ERRNO_EBADF",
    "RCVRY_TYPE_HW_TSX",
    "RCVRY_TYPE_UNDO_MALLOC",
    "RCVRY_TYPE_UNDO_MEMALIGN",
    "RCVRY_TYPE_UNDO_REALLOC",
    "RCVRY_TYPE_UNDO_SETLOCALE",
    "RCVRY_TYPE_CLOSE_SOCKET",
    "RCVRY_TYPE_CLOSE_RET_NULL",
    "RCVRY_TYPE_CLOSE",
    "RCVRY_TYPE_DLCLOSE",
    "RCVRY_TYPE_UNDO_FORK",
    "RCVRY_TYPE_UNDO_ADDRINFO",
    "RCVRY_TYPE_UNDO_MKDIR",
    "RCVRY_TYPE_UNDO_MMAP",
    "RCVRY_TYPE_UNDO_PTHRDMUTX_INIT",
    "RCVRY_TYPE_UNDO_PTHRDMUTX_LOCK",
    "RCVRY_TYPE_CKPT_BUFF_PREAD",
    "RCVRY_TYPE_FAIL",
};

int rcvry_dispatch(void *info)
{
    rcvry_info_t *rinfo;

    assert(rcvry_current_site_id <= rcvry_libcall_max_site_id && "Invalid rcvry_current_site_id");

    rinfo = &rcvry_info[rcvry_current_site_id];
    rcvry_print_info("rcvry_type: %s (%p)\n", rcvry_type_strs[rinfo->rcvry_type], rcvry_actions_map[rinfo->rcvry_type]);
    rcvry_dispatched_action = rinfo->rcvry_type;
    rcvry_print_info("\nRecovery_action_applied: %s\n", rcvry_type_strs[rcvry_dispatched_action]);
#ifdef RCVRY_WINDOW_PROFILING
    rcvry_actscribe("\nRecovery_action_applied: %s\n", rcvry_type_strs[rcvry_dispatched_action]);
#endif
#ifdef RCVRY_TIME_TO_RECOVER
    if (NULL != rcvry_time_fptr) {
        fprintf(rcvry_time_fptr, "\nRecovery_action_applied: %s\n", rcvry_type_strs[rcvry_dispatched_action]);
        fflush(rcvry_time_fptr);
    }
#endif
    if (RCVRY_E_OK != (rcvry_actions_map[rinfo->rcvry_type])()) {
	rcvry_print_error("Recovery attempt failed for site-id: %d\n", rcvry_current_site_id);
#ifdef RCVRY_WINDOW_PROFILING
    rcvry_prof_dump();
#endif
	exit(1);
    }
    return RCVRY_E_OK;
}

#ifdef RCVRY_AUTO_ADAPT

#define RCVRY_AUTO_ADAPT__VAR_SAMPLE_SIZE       "RCVRY_AUTO_ADAPT__SAMPLE_SZ"
#define RCVRY_AUTO_ADAPT__VAR_THRESHOLD         "RCVRY_AUTO_ADAPT__THRESHOLD"
#define RCVRY_AUTO_ADAPT__VAR_SCALE         	"RCVRY_AUTO_ADAPT__SCALE"

__attribute__((constructor))
void rcvry_auto_adapt_init()
{
    const char *envVar = NULL;
    envVar = getenv(RCVRY_AUTO_ADAPT__VAR_SAMPLE_SIZE);
    if (NULL != envVar) {
        rcvry_auto_adapt_sample_size
        = strtoul(envVar, NULL, 0);
    }
    envVar = getenv(RCVRY_AUTO_ADAPT__VAR_THRESHOLD);
    if (NULL != envVar) {
        rcvry_auto_adapt_threshold
        = strtoul(envVar, NULL, 0);
    }
    envVar = getenv(RCVRY_AUTO_ADAPT__VAR_SCALE);
    if (NULL != envVar) {
        rcvry_auto_adapt_scale
        = strtoul(envVar, NULL, 0);
    }
    for (unsigned i=0; i < RCVRY_MAX_LIBCALL_SITES; i++) {
        rcvry_info[i].sample_count = 0;
        rcvry_info[i].num_tsx_hits = 0;
        rcvry_info[i].num_tsx_fails = 0;
    }
    rcvry_print_info("Autoadapt sample-size: %d, threshold: %d\n", 
                     rcvry_auto_adapt_sample_size, rcvry_auto_adapt_threshold);
    return;
}

#endif

#ifdef RCVRY_PRINT_TO_FILE
__attribute__((constructor))
void rcvry_print_to_file_init()
{
    if (NULL == rcvry_fprint_fptr) {
	char *pstr = malloc(sizeof(char)*50);
	sprintf(pstr, "%s", RCVRY_PRINT_TO_FILE_NAME);
        rcvry_fprint_fptr = fopen(pstr, "a+");
    }
}
#endif

#ifdef RCVRY_WINDOW_PROFILING
__attribute__((constructor))
void rcvry_prof_init()
{
   rcvry_prof_at_newproc();
#ifdef RCVRY_PER_PROCESS_PROFILE_OUTPUT
   pthread_atfork(NULL, NULL, rcvry_prof_at_newproc);
   atexit(rcvry_prof_at_exit);
#endif
}

void rcvry_prof_at_newproc()
{
#ifdef EDFI_BB_TRACING_FOR_RCVRY
extern uint32_t edfi_bbtrace_is_startup;
edfi_bbtrace_is_startup = 1;
#endif

#ifdef RCVRY_PER_PROCESS_PROFILE_OUTPUT
    char *pstr = malloc(sizeof(char)*50);
    sprintf(pstr, "%s.%d", RCVRY_PROFILING_OUTFILE, getpid());
    rcvry_log_fptr = fopen(pstr, "w+");
    sprintf(pstr, "%s.%d", RCVRY_ACTION_OUTFILE, getpid());
    rcvry_act_fptr = fopen(pstr, "w");
#else
    if (NULL == rcvry_log_fptr) {
        rcvry_log_fptr = fopen(RCVRY_PROFILING_OUTFILE, "w");
    }
    if (NULL == rcvry_act_fptr) {
        rcvry_act_fptr = fopen(RCVRY_ACTION_OUTFILE, "w");
    }
#endif
    return;
}

void rcvry_prof_at_exit()
{
    rcvry_prof_dump();
#ifdef RCVRY_WINDOW_PROFILING_BBTRACING
    rcvry_prof_bbtrace_dump();
#endif
}

void rcvry_prof_dump()
{
    rcvry_print_info("Dumping rcvry profiling data.\n");
    printf("Dumping rcvry profiling data.\n");
    if (NULL == rcvry_log_fptr) {
#ifdef RCVRY_PER_PROCESS_PROFILE_OUTPUT
	char *pstr = malloc(sizeof(char)*50);
	sprintf(pstr, "%s.%d", RCVRY_PROFILING_OUTFILE, getpid());
        rcvry_log_fptr = fopen(pstr, "w+");
#else
        rcvry_log_fptr = fopen(RCVRY_PROFILING_OUTFILE, "w+");
#endif
    }
    rcvry_fscribe("site_id, num_hits_tsx, num_hits_ckpt, num_rcvry_success__switchtxtype,\
 num_hits, num_rcvry_successes, num_rcvry_success__errpath,\
 branch_hits, libcalls__protected, libcalls__skipped_nonfaults, libcalls__skipped_nousers");
#ifdef RCVRY_WINDOW_PROFILING_BBTRACING
    rcvry_fscribe(", num_bbs_in_trace");
#endif
    rcvry_fscribe("\n");

    for (unsigned i=0; i < RCVRY_MAX_LIBCALL_SITES; i++) {
       if ((0 != rcvry_info[i].rcvry_prof.num_hits_rcvry_branch)
	   || (0 != rcvry_info[i].rcvry_prof.libcalls__skipped_nonfaultable)
	   || (0 != rcvry_info[i].rcvry_prof.libcalls__skipped_nousers)) {
            rcvry_fscribe("%4d,%8u,%8u,%8u,%8u,%8u,%8u,%8u,%8u,%8u,%8u", i,
                          rcvry_info[i].rcvry_prof.num_hits_tsx,
                          rcvry_info[i].rcvry_prof.num_hits_ckpt,
                          rcvry_info[i].rcvry_prof.num_rcvry_success__switchtxtype,
                          rcvry_info[i].rcvry_prof.num_hits,
                          rcvry_info[i].rcvry_prof.num_rcvry_successes,
                          rcvry_info[i].rcvry_prof.num_rcvry_success__errpath,
                          rcvry_info[i].rcvry_prof.num_hits_rcvry_branch,
                          rcvry_info[i].rcvry_prof.libcalls__protected,
                          rcvry_info[i].rcvry_prof.libcalls__skipped_nonfaultable,
                          rcvry_info[i].rcvry_prof.libcalls__skipped_nousers);
#if RCVRY_WINDOW_PROFILING_BBTRACING
            rcvry_fscribe(",%8u", rcvry_info[i].rcvry_prof.num_bbs);
#endif
            rcvry_fscribe("\n");
       }
    }
    rcvry_fscribe("\nrcvry_num_delayed_free_calls: %8u\n", rcvry_num_delayed_free_calls);
    return;
}

#ifdef RCVRY_WINDOW_PROFILING_BBTRACING
__attribute__((destructor))
void rcvry_prof_bbtrace_dump()
{
    int disable_dump = 0;
    static int already_dumped = 0;

/*	if (already_dumped) {
        return;
    }
*/
    char *env_val = getenv("DISABLE_BBTRACE_DUMP");
    if (NULL != env_val) {
    	disable_dump = strtoul(env_val, NULL, 0);
    }
    if (0 != disable_dump) {
        return;
    }
    rcvry_print_info("Dumping rcvry basic block tracing data (in json).\n");
    char *pstr = malloc(sizeof(char)*50);
    char *pstr_startup = malloc(sizeof(char)*60);
    int per_process_outfile = 0;
    env_val = getenv("BBTRACE_OUPUT_PER_PROCESS");
    if (NULL != env_val) {
        per_process_outfile = strtoul(env_val, NULL, 0);
    }
    if (per_process_outfile) {
        sprintf(pstr, "%s.%d", RCVRY_BBTRACE_OUTFILE, getpid());
        sprintf(pstr_startup, "%s.%s.%d", RCVRY_BBTRACE_OUTFILE, "startup", getpid());
    } else  {
        sprintf(pstr, "%s", RCVRY_BBTRACE_OUTFILE);
        sprintf(pstr_startup, "%s.%s", RCVRY_BBTRACE_OUTFILE, "startup");
    }
    if ((NULL == rcvry_bbt_fptr) || per_process_outfile) {
#ifdef RCVRY_BBTRACE_APPEND
        rcvry_bbt_fptr = fopen(pstr, "a+");
        rcvry_bbt_startup_fptr = fopen(pstr_startup, "a+");
#else
        rcvry_bbt_fptr = fopen(pstr, "w+");
        rcvry_bbt_startup_fptr = fopen(pstr_startup, "w+");
#endif
    }
    rcvry_bbtscribe("{");
    rcvry_bbt_startup_scribe("[");
    int isFirst = 1, isFirstStartup = 1;
    for (unsigned i=0; i < RCVRY_MAX_LIBCALL_SITES; i++) {
        if (0 != rcvry_info[i].rcvry_prof.num_hits_rcvry_branch) {
            if (!isFirst) {
                rcvry_bbtscribe(",");
            }
            rcvry_bbtscribe("\"%u\":[", i);
            isFirst = 0;
            uint32_t prev = 0;
            for (uint32_t j=0; j < rcvry_info[i].rcvry_prof.num_bbs; j++) {
#ifdef RCVRY_BBTRACE_EXCLUDE_STARTUP
                extern uint32_t edfi_bbtrace_startuplist[];
                if (edfi_bbtrace_startuplist[rcvry_info[i].rcvry_prof.bbs[j]] != 0) {
                    if (j == rcvry_info[i].rcvry_prof.num_bbs - 1) {
                        // this is the last one, so rewrite prev to keep formatting valid
                        rcvry_bbtscribe("%u", prev);
                    }
                    continue;
                }
#endif
                rcvry_bbtscribe("%u", rcvry_info[i].rcvry_prof.bbs[j]);
                prev = rcvry_info[i].rcvry_prof.bbs[j];
                if (j != rcvry_info[i].rcvry_prof.num_bbs - 1) {
                    rcvry_bbtscribe(",");
                }
            }
            rcvry_bbtscribe("]");
        }
    }

	isFirst = 1;
    extern uint32_t edfi_bbtrace_startuplist[];
	for (uint32_t i=0; i < EDFI_BB_TRACING_HASHLIST_SIZE; i++) {
		if (edfi_bbtrace_startuplist[i] != 0) {
			if (isFirst) {
	    		rcvry_bbt_startup_scribe("%u", edfi_bbtrace_startuplist[i]);
				isFirst = 0;
			} else {
	    		rcvry_bbt_startup_scribe(", %u", edfi_bbtrace_startuplist[i]);
			}
		}
	}
#ifdef RCVRY_BBTRACE_APPEND
    rcvry_bbtscribe("}\n");
    rcvry_bbt_startup_scribe("]\n");
#else
    rcvry_bbtscribe("}");
    rcvry_bbt_startup_scribe("]");
#endif
    already_dumped = 1;
    return;
}
#endif
#endif

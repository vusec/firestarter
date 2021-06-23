/* Description: Signal handling
 * To detect fail-stop faults and invoke recovery
 * 
 * Author : Koustubha Bhat
 * Date	  : 8-March-2017
 * Vrije Universiteit, Amsterdam.
 */

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "rcvry_common.h"
#include "rcvry_dispatch.h"
#include "../../sharedlib/ltckpt/ltckpt_regs.h"
#include "../ltckpt/ltckpt_common.h"
#ifdef RCVRY_TIME_TO_RECOVER
#include "../../include/common/util/time.h"
FILE *rcvry_time_fptr = NULL;
#endif
static inline int is_expected_signal(int signo);
static inline int is_in_child_process();
static inline int just_return_after_special_handling(int signo);
static inline void unblock_sig(int signo);
#if 0
static inline int check_sw_and_decide(rcvry_info_t *rinfo);
#endif
static inline int invoke_stm_recovery(rcvry_info_t *rinfo);

/* Our THE custom signal handler */
void rcvry_signal_action(int signo, siginfo_t *info, void *context)
{
#ifdef RCVRY_TIME_TO_RECOVER
  static unsigned long long tsc1 = 0, tsc2 = 0, tsc_ckpt=0, tsc_ra=0;
  tsc1 = util_time_tsc_read();
#endif

  int r_ckpt = RCVRY_E_FAIL;
  rcvry_info_t *rinfo = &rcvry_info[rcvry_current_site_id];
  static uint32_t prev_site_id = 0;
  static uint32_t repeat_count = 0;

  rcvry_print_info("Received signal: %d, current_tx_type: %d\n", signo, RCVRY_CURR_TX_TYPE);
  if (!is_expected_signal(signo)) {
      exit(1);
  }

#ifdef RCVRY_WINDOW_PROFILING
      rinfo->rcvry_prof.num_hits++;
#endif

  if (signo == SIGUSR1) {
    /* TSX would have failed, so switch over to STM and return */
    rcvry_print_info("Switching to undolog based checkpointing.\n");
    RCVRY_CURR_TX_TYPE = LTCKPT_TYPE_UNDOLOG;
    rcvry_libcall_gates[rcvry_current_site_id] = LTCKPT_TYPE_UNDOLOG;
#ifdef RCVRY_WINDOW_PROFILING
    rinfo->rcvry_prof.num_rcvry_success__switchtxtype++;  // If child comes here, it won't affect the parent's statistics.
#endif
  } else if (signo == SIGSEGV) {

      if (prev_site_id == rcvry_current_site_id) {
        // possibly getting into infinite recovery
        rcvry_print_info("POSSIBLE INFINITE RECOVERY ATTEMPTS. Exiting\n");
	repeat_count++;
	if (repeat_count > 64) {
        	exit(2);
	}
      } else {
	repeat_count = 0;
      }
      prev_site_id = rcvry_current_site_id;

      // Induce deviation to error handling path
      rcvry_libcall_gates[rcvry_current_site_id] = RCVRY_LIBCALL_GATE_CLOSE;
#ifdef RCVRY_WINDOW_PROFILING
      rinfo->rcvry_prof.num_rcvry_success__errpath++;
#endif
      rcvry_print_info("Starting libcall recovery action...\n");
#ifdef RCVRY_TIME_TO_RECOVER
      tsc_ra = util_time_tsc_read();
#endif
      if (RCVRY_E_FAIL == rcvry_dispatch(NULL)) {
#ifdef RCVRY_TIME_TO_RECOVER
        fprintf(rcvry_time_fptr, "[ltckpt_tsx] time diff (ra_fail): %llu\n", util_time_tsc_read() - tsc_ra);
#endif
        rcvry_print_error("Failed performing libcall recovery actions.\n");
        exit(1);
      }
#ifdef RCVRY_TIME_TO_RECOVER
        fprintf(rcvry_time_fptr, "[ltckpt_tsx] time diff (ra): %llu\n", util_time_tsc_read() - tsc_ra);
#endif
      if (RCVRY_CURR_TX_TYPE == LTCKPT_TYPE_UNDOLOG) {
#ifdef RCVRY_TIME_TO_RECOVER
      tsc_ckpt = util_time_tsc_read();
#endif
        r_ckpt = invoke_stm_recovery(rinfo);
#ifdef RCVRY_TIME_TO_RECOVER
        fprintf(rcvry_time_fptr, "[ltckpt_tsx] time diff (stm recov): %llu\n", util_time_tsc_read() - tsc_ckpt);
#endif
        rcvry_print_info("Finished ltckpt restoration. [ret: %d]\n", r_ckpt);
      }
      unblock_sig(SIGSEGV);
  } // end of, if SIGSEGV

  // 3. Restore registers
  //    When following is not a NOOP, hereafter, the execution goes back to TOL point
#ifdef RCVRY_WINDOW_PROFILING
      rinfo->rcvry_prof.num_rcvry_successes++;
#endif
#ifdef RCVRY_TIME_TO_RECOVER
  tsc2 = util_time_tsc_read();
  fprintf(rcvry_time_fptr, "[ltckpt_tsx] time diff: %llu\n", tsc2 - tsc1);
  fflush(rcvry_time_fptr);
#endif
#ifdef RCVRY_LONGJMP
  longjmp(ltckpt_registers, 1);
#else
  ltckpt_asm_restore_registers(ltckpt_registers, 1);
#endif
  return;
}


#ifndef RCVRY_DBG_DISABLE_SIGNAL_HANDLER
__attribute__((constructor))
#endif
void rcvry_signal_register_handler()
{
  struct sigaction sa;

#ifdef RCVRY_TIME_TO_RECOVER
  if (NULL == rcvry_time_fptr) {
     char fname[100];
     sprintf(fname, "/tmp/rcvry_time__%d.txt", getpid());
     rcvry_time_fptr = fopen(fname, "w+");
  }
#endif
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_sigaction = rcvry_signal_action;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO | SA_NODEFER;

  if (0 > sigaction(SIGSEGV, &sa, NULL)) {
    rcvry_print_error("Failed to register signal action handler for SIGSEGV.\n");
    exit(1);
  }
  if (0 > sigaction(SIGUSR1, &sa, NULL)) {
    rcvry_print_error("Failed to register signal action handler for SIGUSR1.\n");
    exit(1);
  }
  printf("/* Registered rcvry signal handlers. */\n");
  return;
}

static inline int is_expected_signal(int signo)
{
  return (SIGUSR1 == signo
          || SIGSEGV == signo);
}

static inline __attribute__((unused)) int is_in_child_process()
{
  return (RCVRY_CHILD_STARTED == rcvry_child_state);
}

/* If returns 1, just return. Otherwise, continue */
static inline __attribute__((unused)) int just_return_after_special_handling(int signo)
{
  if (SIGUSR1 == signo) {
      rcvry_just_noop_for_tsx = 0;
      return 1;
  }
  return 0;
}

#if 0
static inline int check_sw_and_decide(rcvry_info_t *rinfo)
{
  int r_ckpt = rcvry_check_swfault_exists();

  switch(r_ckpt) {

    case RCVRY_TSX_RA_SW_OK:
        if (!is_in_child_process()) {
          rcvry_print_info("Switching to undolog based checkpointing.\n");
          RCVRY_CURR_TX_TYPE = LTCKPT_TYPE_UNDOLOG;
          rcvry_libcall_gates[rcvry_current_site_id] = LTCKPT_TYPE_UNDOLOG;
#ifdef RCVRY_WINDOW_PROFILING
          rinfo->rcvry_prof.num_rcvry_success__switchtxtype++;  // If child comes here, it won't affect the parent's statistics.
#endif
        }
        r_ckpt = RCVRY_E_OK;
        break;

    case RCVRY_TSX_RA_SW_FAULT:
        // Induce deviation to error handling path
        rcvry_libcall_gates[rcvry_current_site_id] = RCVRY_LIBCALL_GATE_CLOSE;
#ifdef RCVRY_WINDOW_PROFILING
        rinfo->rcvry_prof.num_rcvry_success__errpath++;
#endif
        break;

    case RCVRY_E_OK:
        return RCVRY_E_OK;

    default:
        break;
  }

#ifdef RCVRY_WINDOW_PROFILING
  rinfo->rcvry_prof.num_rcvry_successes++;
#endif
  return r_ckpt;
}
#endif

static inline int invoke_stm_recovery(rcvry_info_t *rinfo)
{
  int r_ckpt;
  rcvry_print_info("Starting checkpoint rollback...\n");
  if (RCVRY_E_FAIL == (r_ckpt = ltckpt_restart(NULL))) {
      rcvry_print_error("Error in ltckpt_restart.\n");
      return r_ckpt;
  }
  return r_ckpt;
}

static inline void unblock_sig(int signo)
{
    sigset_t set, oldset;

    sigfillset(&set);
    sigaddset(&set, signo);
    sigprocmask(SIG_UNBLOCK, &set, &oldset);
    return;
}

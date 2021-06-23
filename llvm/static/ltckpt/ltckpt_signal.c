#include "ltckpt_common.h"
#include "ltckpt_fi.h"

#ifdef LTCKPT_WINDOW_PROFILING
#include "ltckpt_profiling.h"
#endif
#ifdef RCVRY_WINDOW_PROFILING
#include "../rcvry/rcvry_dispatch.h"
#endif

#include <signal.h>

#ifdef LTCKPT_NOP_UNTIL_SIGNALLED
int ltckpt_tsx_disabled = 1;
#endif

void ltckpt_signal_action(int signo, siginfo_t *info, void *context)
{
  ltckpt_printf("Received signal: %d\n", signo);
  if ((SIGTTIN != signo) && (SIGTTOU != signo) && (SIGPROF != signo) && (SIGWINCH != signo) && (SIGTSTP != signo) && (SIGSTKFLT != signo) && (SIGURG != signo))
        return;

  switch (signo) {
      case SIGPROF:
      case SIGTTIN:
      case SIGTSTP:
#ifdef LTCKPT_NOP_UNTIL_SIGNALLED
      extern int ltckpt_tsx_disabled;
      ltckpt_tsx_disabled = 0;
#endif
#ifdef LTCKPT_WINDOW_PROFILING
  ltckpt_windows_dump();
#endif
#if RCVRY_WINDOW_PROFILING
  rcvry_prof_dump();
#endif
#ifdef RCVRY_WINDOW_PROFILING_BBTRACING
  rcvry_prof_bbtrace_dump();
#endif
#if !defined(LTCKPT_WINDOW_PROFILING) && !defined(RCVRY_WINDOW_PROFILING)
  ltckpt_suicide_enabled = ltckpt_suicide_enabled ? 0 : 1;
#endif
	break;

       case SIGWINCH:
       case SIGTTOU:
       case SIGURG:
#ifdef LTCKPT_WINDOW_PROFILING
	memset(ltckpt_windows, 0, LTCKPT_MAX_PROFILING_WINDOWS * sizeof(ltckpt_window_t));
	ltckpt_printf("Zeroed out profiling windows.\n");
#endif
#ifdef RCVRY_WINDOW_PROFILING
        for (unsigned i=0; i < RCVRY_MAX_LIBCALL_SITES; i++) {
            memset(&(rcvry_info[i].rcvry_prof), 0, sizeof(rcvry_prof_t));
        }
        ltckpt_printf("Zeroed out rcvry profiling counters.\n");
#ifdef RCVRY_WINDOW_PROFILING_BBTRACING
        extern uint32_t edfi_bbtrace_is_startup;
        edfi_bbtrace_is_startup = !(edfi_bbtrace_is_startup);
#endif
#endif
	break;
  }

  return;
}

void __attribute__((constructor)) ltckpt_signal_register_handler()
{
  struct sigaction sa;

  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_sigaction = ltckpt_signal_action;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;

  if (0 > sigaction(SIGTTIN, &sa, NULL)) {	/* Repurposing this signal for user interaction */
    ltckpt_printf("Failed to register signal action handler.\n");
    exit(1);
  }
  if (0 > sigaction(SIGPROF, &sa, NULL)) {	/* Repurposing this signal for user interaction */
    ltckpt_printf("Failed to register signal action handler.\n");
    exit(1);
  }
#ifndef LTCKPT_NO_WINCH
  if (0 > sigaction(SIGWINCH, &sa, NULL)) {	/* Repurposing this signal for user interaction */
    ltckpt_printf("Failed to register signal action handler.\n");
    exit(1);
  }
#endif
  if (0 > sigaction(SIGURG, &sa, NULL)) {	/* Repurposing this signal for user interaction */
    ltckpt_printf("Failed to register signal action handler.\n");
    exit(1);
  }
  if (0 > sigaction(SIGTTOU, &sa, NULL)) {	/* Repurposing this signal for user interaction */
    ltckpt_printf("Failed to register signal action handler.\n");
    exit(1);
  }
  if (0 > sigaction(SIGTSTP, &sa, NULL)) {	/* Repurposing this signal for user interaction */
    ltckpt_printf("Failed to register signal action handler.\n");
    exit(1);
  }
  if (0 > sigaction(SIGSTKFLT, &sa, NULL)) {	/* Repurposing this signal for user interaction */
    ltckpt_printf("Failed to register signal action handler.\n");
    exit(1);
  }
  ltckpt_printf("/* Registered ltckpt signal handler. */\n");
  return;
}

#ifndef RCVRY_COMMON_H
#define RCVRY_COMMON_H

#include <assert.h>
#include <signal.h>
#include "rcvry_log.h"
#include "rcvry_dispatch.h"
#include "rcvry_util.h"

#define RCVRY_E_FAIL		-1
#define RCVRY_E_OK		0
#define RCVRY_TSX_RA_SW_FAULT	1	/* Found out existence of s/w fault */
#define RCVRY_TSX_RA_SW_OK	2	/* Found no evidence of s/w fault */

void rcvry_signal_action(int signo, siginfo_t *info, void *context);

#endif

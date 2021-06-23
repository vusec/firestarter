#ifndef LTCKPT_FI_H
#define LTCKPT_FI_H

#include "ltckpt_common.h"

#define LTCKPT_MAX_SUICIDE_SITES	4096

extern uint32_t	 ltckpt_suicide_enabled;
extern uint32_t  ltckpt_suicide_board[LTCKPT_MAX_SUICIDE_SITES];
extern uint32_t  ltckpt_suicide_last_site_id; // Set by instrumentation

void ltckpt_fi_suicide();
int ltckpt_fi_suicide_per_site(uint32_t site_id, int only_once);

#endif

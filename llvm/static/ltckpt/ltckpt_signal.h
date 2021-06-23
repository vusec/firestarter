#ifndef LTCKPT_SIGNAL_H
#define LTCKPT_SIGNAL_H

#ifdef LTCKPT_NOP_UNTIL_SIGNALLED
extern int ltckpt_tsx_disabled;
#endif

int ltckpt_register_signal();

#endif

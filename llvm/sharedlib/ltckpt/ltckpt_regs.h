#ifndef LTCKPT_REGS_H
#define LTCKPT_REGS_H 1
#include <setjmp.h>

extern void __attribute__((weak)) ltckpt_asm_save_registers(jmp_buf *buf, int val);
extern void __attribute__((weak)) ltckpt_asm_restore_registers(jmp_buf *buf, int val);

#endif

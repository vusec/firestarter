/*------------------------------------------------------------------
 A test module to verify saving and restoring registers.

 $ clang test_ctx_registers.c ctx_registers.S -o test_ctx_registers
-------------------------------------------------------------------*/

#include <stdio.h>
#include <setjmp.h>
#include <stdint.h>
#include "ltckpt_regs.h"

#define WRITE_TO_REGS(ARG1, ARG2, ARG3, ARG4) ({				\
	uint64_t a1 = (ARG1), a2 = (ARG2), a3 = (ARG3), a4 = (ARG4);		\
	asm volatile (								\
		"movq %0, %%r12\n\t"						\
		"movq %1, %%r13\n\t"						\
		"movq %2, %%r14\n\t"						\
		"movq %3, %%rbx"						\
		:	/* no output */						\
		: "g" (a1), "g" (a2), "g" (a3), "g"(a4) 			\
		: );								\
	})

__thread sigjmp_buf saved_registers;
//extern int ltckpti_save_all_registers(sigjmp_buf *buf, int mask);
//extern int ltckpt_restore_all_registers(sigjmp_buf *buf, int mask);
char *regnames[9] = { "RBX", "RBP", "R12", "R13", "R14", "R15", "RSP", "PC" };
int restored=0;

int main()
{
   WRITE_TO_REGS(0x7, 0x8, 0x9, 0xA);
   printf("Saving registers...\t");
   ltckpt_asm_save_registers(&saved_registers, 0);
   printf("[done]\n");

   printf("\nTheir values:\n");
   uint64_t *pbuf = (uint64_t *)&saved_registers;
   for (int i = 0; i < 8; i++) {
 	printf("reg %s: %8lx\n", regnames[i], pbuf[i]);
   }
   printf("\nThat's all\n");
   if (restored) return 0;	// When restored, we start from where we saved!

   printf("\nRestoring registers...\t");
   restored = 1;
   ltckpt_asm_restore_registers(&saved_registers, 0);
   return -1;
}

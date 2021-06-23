/*------------------------------------------------------------------
 A test module to verify saving and restoring registers.

 $ clang test_ctx_registers.c ctx_registers.S -o test_ctx_registers
-------------------------------------------------------------------*/

#include <stdio.h>
#include <setjmp.h>
#include <stdint.h>
#include "ltckpt_regs.h"
#include <signal.h>

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
char *regnames[9] = { "RBX", "RBP", "R12", "R13", "R14", "R15", "RSP", "PC" };
int restored=0;
volatile int raised=0;

__attribute__((noinline))
void save_regs()
{
   ltckpt_asm_save_registers(&saved_registers, 0);
}

void handler(int signo, siginfo_t *info, void *context)
{
    if (SIGSEGV != signo)
        return;

    raised = 1;
    ltckpt_asm_restore_registers(&saved_registers, 0);
}

void register_handler()
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (0 > sigaction(SIGSEGV, &sa, NULL)) {
      printf("ERROR: Failed to register signal action handler.\n");
      exit(1);
    }
    printf("INFO: Registered signal handler for SIGSEGV(%d).\n", SIGSEGV);
    return;
}

void raise_signal()
{
   printf("\nIn func: %s\n", __func__);
   fflush(stdout);
   if (!raised) {
       raised++;
       printf("raising SIGSEGV\n");
       raise(SIGSEGV);
   }
}

int main()
{
   register_handler();
   WRITE_TO_REGS(0x7, 0x8, 0x9, 0xA);
   printf("Saving registers...\t");
   ltckpt_asm_save_registers(&saved_registers, 0);
   //save_regs();
   printf("[done] raised = %d\n", raised);

   printf("\nTheir values:\n");
   uint64_t *pbuf = (uint64_t *)&saved_registers;
   for (int i = 0; i < 8; i++) {
 	printf("reg %s: %8lx\n", regnames[i], pbuf[i]);
   }
   raise_signal();
   printf("\nThat's all\n");
   if (restored) return 0;	// When restored, we start from where we saved!

   printf("\nRestoring registers...\t");
   restored = 1;
   ltckpt_asm_restore_registers(&saved_registers, 0);

   return -1;
}

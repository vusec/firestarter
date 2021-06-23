ROOT=../../..

MODULE=ltckpt_regs

ARCH=x64

SRCS = arch/$(ARCH)/ltckpt_regs.S
OBJS = $(SRCS:%.S=%.o)

include $(ROOT)/llvm/sharedlib/Makefile.inc

LINKFLAGS+= "-lltckpt_regs"
LDFLAGS += "-L./"

test: test_save_registers.c
	$(QUIET) $(ECHO) "[GOLD_CC] $(LDFLAGS) $(CFLAGS) $< -o test_sv_regs $(LINKFLAGS)"
	$(QUIET) $(LLVMGOLD_CC) $(LDFLAGS) $< -o test_sv_regs $(LINKFLAGS)

testsig: test_signals_with_save_registers.c
	$(QUIET) $(ECHO) "[GOLD_CC] $(LDFLAGS) $(CFLAGS) $< -o test_sigs_sv_regs $(LINKFLAGS)"
	$(QUIET) $(LLVMGOLD_CC) $(LDFLAGS) $< -o test_sigs_sv_regs $(LINKFLAGS)

testclean:
	$(QUIET) $(ECHO) "[rm -f] test_sv_regs test_sigs_sv_regs"
	$(QUIET) rm -f test_sv_regs test_sigs_sv_regs

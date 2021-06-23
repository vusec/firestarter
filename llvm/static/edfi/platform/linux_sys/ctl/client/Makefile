#Makefile for creating the edfi_ctl_client static library

ifneq ($(Plat),Minix)

ROOT=../../../../../../..

include $(ROOT)/Makefile.inc

MODULE = edfi_ctl_client
SRCS = edfi_ctl_client.c
OBJS = $(SRCS:.c=.o)
HEADERS = $(wildcard $(LLVMINC)/edfi/*.h) $(wildcard $(LLVMINC)/edfi/ctl/*.h)

ifeq ($(LTCKPT_SET_PIC), 1)
CFLAGS+= -fpic
endif

include $(ROOT)/llvm/static/Makefile.inc

endif


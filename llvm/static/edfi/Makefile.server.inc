#Makefile for creating the edfi_ctl_server static library

ROOT=../../../../../../..

include $(ROOT)/Makefile.inc

MODULE = edfi_ctl_server
SRCS = edfi_ctl_server.c
OBJS = $(SRCS:.c=.o)
HEADERS = $(wildcard $(LLVMINC)/edfi/*.h) $(wildcard $(LLVMINC)/edfi/ctl/*.h)

ifneq ($(EDFI_CONTEXT_RELOCATE),)
CFLAGS+=-DEDFI_CONTEXT_RELOCATE=$(EDFI_CONTEXT_RELOCATE)
endif

USE_SECTIONIFY=YES
SECTIONIFY_DATA_SECTION_MAP=.*/edfi_data
SECTIONIFY_FUNCTION_SECTION_MAP=.*/edfi_functions

ifeq ($(LTCKPT_SET_PIC), 1)
CFLAGS+= -fpic
endif

include $(ROOT)/llvm/static/Makefile.inc

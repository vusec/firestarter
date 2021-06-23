#Makefile for creating the edfi_df static library

ROOT=../../../..

include $(ROOT)/Makefile.inc

MODULE = edfi_df
SRCS = edfi_df.c
OBJS = $(SRCS:.c=.o)
HEADERS = $(wildcard $(LLVMINC)/edfi/*.h) $(wildcard $(LLVMINC)/edfi/df/*.h)

USE_SECTIONIFY=YES
SECTIONIFY_DATA_SECTION_MAP=.*/edfi_data
SECTIONIFY_FUNCTION_SECTION_MAP=.*/edfi_functions

ifeq ($(EDFI_BB_TRACING_FOR_RCVRY), 1)
        CFLAGS+=-DEDFI_BB_TRACING_FOR_RCVRY
endif
ifeq ($(LTCKPT_SET_PIC), 1)
CFLAGS+= -fpic
endif

include $(ROOT)/llvm/static/Makefile.inc


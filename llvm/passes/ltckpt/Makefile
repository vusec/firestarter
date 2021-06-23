# Makefile for the ltckpt pass
ROOT=../../..
CXXFLAGS+=-std=c++11
include $(ROOT)/Makefile.inc
PASSNAME := ltckpt
OBJS := ltCkptPass.o ltCkptPassWC.o ltCkptPassBasic.o ltCkptPassTsx.o ltCkptPassFI.o

ifeq ($(LLVM_HAS_DSA), yes)
OBJS= ltCkptPassHeap.o
endif


INCLUDES = -I../include

include $(ROOT)/llvm/passes/Makefile.inc

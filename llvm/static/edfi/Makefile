ROOT=../../..

include $(ROOT)/Makefile.inc

DIRS := ctl df

ifeq ($(Plat),Minix)
EDFI_DEFAULT_PLAT=minix_sys
else
EDFI_DEFAULT_PLAT=unix_apps
endif

ifeq ($(EDFI_PLAT),)
	EDFI_PLAT=$(EDFI_DEFAULT_PLAT)
endif

ifeq ($(EDFI_TRACING_DEBUG), 1)
CFLAGS+= -DEDFI_TRACING_DEBUG
endif

ifeq ($(LTCKPT_SET_PIC), 1)
CFLAGS+= -fpic
endif

all: $(patsubst %,build_%,$(DIRS))

$(patsubst %,build_%,$(DIRS)): DIR = $(subst build_,,$@)
$(patsubst %,build_%,$(DIRS)):
	@echo Building $(DIR) static library...
	$(MAKE) -C $(DIR) install

clean:
	$(foreach DIR, $(DIRS), $(MAKE) -C $(DIR) distclean;)

distclean: clean

install: all

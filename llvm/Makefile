ROOT=..

include ${ROOT}/Makefile.inc

ifeq ($(HAVE_LLVM),1)
DIRS := $(DIRS) passes
endif

DIRS := $(DIRS) static

all: $(patsubst %,build_%,$(DIRS))

$(patsubst %,build_%,$(DIRS)): DIR = $(subst build_,,$@)
$(patsubst %,build_%,$(DIRS)):
	@echo Building LLVM $(DIR)...
	$(MAKE) -C $(DIR) install

clean:
	$(foreach DIR, $(DIRS), $(MAKE) -C $(DIR) distclean;)


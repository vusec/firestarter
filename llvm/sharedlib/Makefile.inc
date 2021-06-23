include $(ROOT)/Makefile.inc
include $(ROOT)/llvm/Makefile.inc

MODULE_NAME=lib$(MODULE).so

$(MODULE_NAME): $(OBJS) 
	$(QUIET) $(ECHO) "  [GOLD_LINK] -shared -o $@ $(OBJS)"
	$(QUIET) $(LLVMGOLD_CC) -shared -o $@ $(OBJS)

%.o: %.S
	$(QUIET) $(ECHO) "  [GOLD_CC] $<"
	$(QUIET) $(LLVMGOLD_CC) $(INCLUDES) $(INCLUDES_$<) -c -o $@ $<

all:$(MODULE_NAME)

install: $(INSTALL_DIR)/$(MODULE_NAME)

$(INSTALL_DIR)/$(MODULE_NAME): $(MODULE_NAME)
	install -c -D -m 744 $? $@

clean:
	$(QUIET) $(ECHO) "  [RM] *.o $(OBJS) $(MODULE_NAME)"
	$(QUIET) rm -f *.o *.so $(OBJS) $(MODULE_NAME)

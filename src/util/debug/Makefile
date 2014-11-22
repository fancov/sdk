C_FILE += $(SROUCE)/util/debug/assert.c \
			$(SROUCE)/util/debug/debug_cli.c \
			$(SROUCE)/util/debug/exception.c \
			$(SROUCE)/util/debug/dos_debug.c \
			$(SROUCE)/util/debug/debug_cmd.c
			

C_OBJ_FILE += assert.$(SUFFIX) \
			debug_cli.$(SUFFIX) \
			exception.$(SUFFIX) \
			dos_debug.$(SUFFIX) \
			debug_cmd.$(SUFFIX)

assert.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/debug/assert.c
	
debug_cli.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/debug/debug_cli.c

exception.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/debug/exception.c

dos_debug.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/debug/dos_debug.c
	
debug_cmd.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/debug/debug_cmd.c



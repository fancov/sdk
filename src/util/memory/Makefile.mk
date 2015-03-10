C_FILE += $(SROUCE)/util/memory/memory.c

C_OBJ_FILE += memory.$(SUFFIX) 

memory.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/memory/memory.c

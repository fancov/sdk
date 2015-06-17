C_FILE += $(SROUCE)/util/memory/memory.c $(SROUCE)/util/memory/dos_iobuf.c 

C_OBJ_FILE += memory.$(SUFFIX) dos_iobuf.$(SUFFIX) 

memory.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/memory/memory.c

dos_iobuf.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/memory/dos_iobuf.c

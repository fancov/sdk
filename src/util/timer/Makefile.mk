C_FILE += $(SROUCE)/util/timer/timer.c 			

C_OBJ_FILE += timer.$(SUFFIX) 

timer.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/timer/timer.c
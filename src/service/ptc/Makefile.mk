C_FILE += $(SROUCE)/service/ptc/ipcc.c \
			$(SROUCE)/service/ptc/ipcc_msg.c 

C_OBJ_FILE += ipcc.$(SUFFIX) \
				ipcc_msg.$(SUFFIX) 


ipcc.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/ptc/ipcc.c

ipcc_msg.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/ptc/ipcc_msg.c
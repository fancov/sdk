C_FILE += $(SROUCE)/service/pts/amc.c \
			$(SROUCE)/service/pts/amc_msg.c 

C_OBJ_FILE += amc.$(SUFFIX) \
				amc_msg.$(SUFFIX) 


amc.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/pts/amc.c

amc_msg.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/pts/amc_msg.c
C_FILE += $(SROUCE)/service/ptc/ptc.c \
			$(SROUCE)/service/ptc/ptc_msg.c \
			$(SROUCE)/service/ptc/ptc_web.c \
			$(SROUCE)/service/ptc/ptc_telnet.c 

C_OBJ_FILE += ptc.$(SUFFIX) \
				ptc_msg.$(SUFFIX) \
				ptc_web.$(SUFFIX) \
				ptc_telnet.$(SUFFIX)


ptc.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/ptc/ptc.c

ptc_msg.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/ptc/ptc_msg.c

ptc_web.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/ptc/ptc_web.c

ptc_telnet.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/ptc/ptc_telnet.c
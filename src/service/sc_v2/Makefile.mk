C_FILE += $(SROUCE)/service/sc_v2/sc_ini.c \
	$(SROUCE)/service/sc_v2/sc_acd.c \
	$(SROUCE)/service/sc_v2/sc_bs.c \
	$(SROUCE)/service/sc_v2/sc_bs_adapter.c \
	$(SROUCE)/service/sc_v2/sc_debug.c \
	$(SROUCE)/service/sc_v2/sc_esl.c \
	$(SROUCE)/service/sc_v2/sc_event.c \
	$(SROUCE)/service/sc_v2/sc_event_fsm.c \
	$(SROUCE)/service/sc_v2/sc_res.c \
	$(SROUCE)/service/sc_v2/sc_res_mngt.c \
	$(SROUCE)/service/sc_v2/sc_su_mngt.c \
	$(SROUCE)/service/sc_v2/sc_lib.c \
	$(SROUCE)/service/sc_v2/sc_hint.c
	

C_OBJ_FILE += sc_ini.$(SUFFIX) \
	sc_acd.$(SUFFIX) \
	sc_bs.$(SUFFIX) \
	sc_bs_adapter.$(SUFFIX) \
	sc_debug.$(SUFFIX) \
	sc_esl.$(SUFFIX) \
	sc_event.$(SUFFIX) \
	sc_event_fsm.$(SUFFIX) \
	sc_res.$(SUFFIX) \
	sc_res_mngt.$(SUFFIX) \
	sc_lib.$(SUFFIX) \
	sc_su_mngt.$(SUFFIX) \
	sc_hint.$(SUFFIX) 

sc_hint.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_hint.c

sc_ini.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_ini.c
	
sc_acd.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_acd.c
	
sc_debug.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_debug.c

sc_bs_adapter.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_bs_adapter.c
	
sc_bs.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_bs.c
	
sc_lib.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_lib.c

sc_su_mngt.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_su_mngt.c
	
sc_res_mngt.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_res_mngt.c

sc_res.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_res.c
	
sc_esl.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_esl.c

sc_event.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_event.c

sc_event_fsm.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_event_fsm.c

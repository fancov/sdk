C_FILE += $(SROUCE)/service/sc/mod_dipcc_sc.c \
	$(SROUCE)/service/sc/sc_acd.c \
	$(SROUCE)/service/sc/sc_api_process.c \
	$(SROUCE)/service/sc/sc_config_update.c  \
	$(SROUCE)/service/sc/sc_debug.c \
	$(SROUCE)/service/sc/sc_dialer.c \
	$(SROUCE)/service/sc/sc_ep_bs_adapter.c \
	$(SROUCE)/service/sc/sc_ep_bs_fsm.c \
	$(SROUCE)/service/sc/sc_event_process.c \
	$(SROUCE)/service/sc/sc_httpd.c \
	$(SROUCE)/service/sc/sc_task.c \
	$(SROUCE)/service/sc/sc_lib.c \
	$(SROUCE)/service/sc/sc_tasks_mngt.c \
	$(SROUCE)/service/sc/sc_cw_queue.c \
	$(SROUCE)/service/sc/sc_ep_extensions.c
	

C_OBJ_FILE += mod_dipcc_sc.$(SUFFIX) \
	sc_api_process.$(SUFFIX) \
	sc_acd.$(SUFFIX) \
	sc_config_update.$(SUFFIX) \
	sc_debug.$(SUFFIX) \
	sc_dialer.$(SUFFIX) \
	sc_ep_bs_adapter.$(SUFFIX) \
	sc_ep_bs_fsm.$(SUFFIX) \
	sc_event_process.$(SUFFIX) \
	sc_httpd.$(SUFFIX) \
	sc_task.$(SUFFIX) \
	sc_lib.$(SUFFIX) \
	sc_tasks_mngt.$(SUFFIX) \
	sc_cw_queue.$(SUFFIX) \
	sc_ep_extensions.$(SUFFIX)

sc_cw_queue.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc/sc_cw_queue.c
	
mod_dipcc_sc.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc/mod_dipcc_sc.c

sc_api_process.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc/sc_api_process.c
	
sc_acd.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc/sc_acd.c

sc_config_update.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc/sc_config_update.c
	
sc_debug.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc/sc_debug.c
	
sc_dialer.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc/sc_dialer.c
	
sc_ep_bs_adapter.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc/sc_ep_bs_adapter.c
	
sc_ep_bs_fsm.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc/sc_ep_bs_fsm.c
	
sc_event_process.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc/sc_event_process.c

sc_httpd.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc/sc_httpd.c
	
sc_task.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc/sc_task.c
	
sc_lib.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc/sc_lib.c

sc_tasks_mngt.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc/sc_tasks_mngt.c

sc_ep_extensions.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc/sc_ep_extensions.c


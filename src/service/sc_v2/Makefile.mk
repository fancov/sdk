C_FILE += $(SROUCE)/service/sc_v2/sc_acd.c \
	$(SROUCE)/service/sc_v2/sc_bs.c \
	$(SROUCE)/service/sc_v2/sc_bs_adapter.c \
	$(SROUCE)/service/sc_v2/sc_caller_setting.c \
	$(SROUCE)/service/sc_v2/sc_cw_queue.c \
	$(SROUCE)/service/sc_v2/sc_data_syn.c \
	$(SROUCE)/service/sc_v2/sc_db.c \
	$(SROUCE)/service/sc_v2/sc_debug.c \
	$(SROUCE)/service/sc_v2/sc_esl.c \
	$(SROUCE)/service/sc_v2/sc_event.c \
	$(SROUCE)/service/sc_v2/sc_event_fsm.c \
	$(SROUCE)/service/sc_v2/sc_event_lib.c \
	$(SROUCE)/service/sc_v2/sc_extensions.c \
	$(SROUCE)/service/sc_v2/sc_hint.c \
	$(SROUCE)/service/sc_v2/sc_http_api.c \
	$(SROUCE)/service/sc_v2/sc_httpd.c \
	$(SROUCE)/service/sc_v2/sc_ini.c \
	$(SROUCE)/service/sc_v2/sc_lib.c \
	$(SROUCE)/service/sc_v2/sc_log_digest.c \
	$(SROUCE)/service/sc_v2/sc_publish.c \
	$(SROUCE)/service/sc_v2/sc_res.c \
	$(SROUCE)/service/sc_v2/sc_res_mngt.c \
	$(SROUCE)/service/sc_v2/sc_su_mngt.c \
	$(SROUCE)/service/sc_v2/sc_limit_caller.c \
	$(SROUCE)/service/sc_v2/sc_schedule_task.c \
	$(SROUCE)/service/sc_v2/sc_pthread.c


C_OBJ_FILE += sc_acd.$(SUFFIX) \
	sc_bs.$(SUFFIX) \
	sc_bs_adapter.$(SUFFIX) \
	sc_caller_setting.$(SUFFIX) \
	sc_cw_queue.$(SUFFIX) \
	sc_data_syn.$(SUFFIX) \
	sc_db.$(SUFFIX) \
	sc_debug.$(SUFFIX) \
	sc_esl.$(SUFFIX) \
	sc_event.$(SUFFIX) \
	sc_event_fsm.$(SUFFIX) \
	sc_event_lib.$(SUFFIX) \
	sc_extensions.$(SUFFIX) \
	sc_hint.$(SUFFIX) \
	sc_http_api.$(SUFFIX) \
	sc_httpd.$(SUFFIX) \
	sc_ini.$(SUFFIX) \
	sc_lib.$(SUFFIX) \
	sc_log_digest.$(SUFFIX) \
	sc_publish.$(SUFFIX) \
	sc_res.$(SUFFIX) \
	sc_res_mngt.$(SUFFIX) \
	sc_su_mngt.$(SUFFIX) \
	sc_task.$(SUFFIX) \
	sc_limit_caller.$(SUFFIX) \
	sc_schedule_task.$(SUFFIX) \
	sc_pthread.$(SUFFIX)


sc_acd.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_acd.c

sc_bs.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_bs.c

sc_bs_adapter.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_bs_adapter.c

sc_caller_setting.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_caller_setting.c
	
sc_cw_queue.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_cw_queue.c
	
sc_data_syn.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_data_syn.c
	
sc_db.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_db.c
	
sc_debug.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_debug.c
	
sc_esl.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_esl.c
	
sc_event.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_event.c
	
sc_event_fsm.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_event_fsm.c
	
sc_event_lib.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_event_lib.c
	
sc_extensions.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_extensions.c
	
sc_hint.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_hint.c
	
sc_http_api.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_http_api.c
	
sc_httpd.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_httpd.c
	
sc_ini.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_ini.c
	
sc_lib.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_lib.c
	
sc_log_digest.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_log_digest.c
	
sc_publish.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_publish.c
	
sc_res.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_res.c
	
sc_res_mngt.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_res_mngt.c
	
sc_su_mngt.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_su_mngt.c
	
sc_task.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_task.c
	
sc_limit_caller.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_limit_caller.c
	
sc_schedule_task.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_schedule_task.c  

sc_pthread.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/sc_v2/sc_pthread.c  
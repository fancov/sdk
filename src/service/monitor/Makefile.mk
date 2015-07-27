CFILE += $(SROUCE)/service/monitor/mon_lib.c \
	$(SROUCE)/service/monitor/mon_get_cpu_info.c \
	$(SROUCE)/service/monitor/mon_get_mem_info.c \
	$(SROUCE)/service/monitor/mon_get_disk_info.c \
	$(SROUCE)/service/monitor/mon_get_net_info.c \
	$(SROUCE)/service/monitor/mon_get_proc_info.c \
	$(SROUCE)/service/monitor/mon_monitor_and_handle.c \
	$(SROUCE)/service/monitor/mon_notification.c \
	$(SROUCE)/service/monitor/mon_warning_msg_queue.c \
	$(SROUCE)/service/monitor/mon_debug.c \
	$(SROUCE)/service/monitor/mod_dipcc_mon.c
	
C_OBJ_FILE += mon_lib.$(SUFFIX) \
	mon_get_cpu_info.$(SUFFIX) \
	mon_get_mem_info.$(SUFFIX) \
	mon_get_disk_info.$(SUFFIX) \
	mon_get_net_info.$(SUFFIX) \
	mon_get_proc_info.$(SUFFIX) \
	mon_monitor_and_handle.$(SUFFIX) \
	mon_notification.$(SUFFIX) \
	mon_warning_msg_queue.$(SUFFIX) \
	mon_debug.$(SUFFIX) \
	mod_dipcc_mon.$(SUFFIX)
	
mon_lib.$(SUFFIX):
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_lib.c
mon_get_cpu_info.$(SUFFIX):
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_get_cpu_info.c
mon_get_mem_info.$(SUFFIX):
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_get_mem_info.c
mon_get_disk_info.$(SUFFIX):
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_get_disk_info.c
mon_get_net_info.$(SUFFIX):
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_get_net_info.c
mon_get_proc_info.$(SUFFIX):
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_get_proc_info.c
mon_monitor_and_handle.$(SUFFIX):
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_monitor_and_handle.c
mon_notification.$(SUFFIX):
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_notification.c
mon_warning_msg_queue.$(SUFFIX):
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_warning_msg_queue.c
mon_debug.$(SUFFIX):
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_debug.c
mod_dipcc_mon.$(SUFFIX):
	$(C_COMPILE) $(SROUCE)/service/monitor/mod_dipcc_mon.c
	
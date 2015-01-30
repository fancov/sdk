C_FILE += $(SROUCE)/service/main.c \
	$(SROUCE)/service/root.c \
	$(SROUCE)/service/telnetd/telnetd.c \
	$(SROUCE)/service/cli/cli_server.c \
	$(SROUCE)/service/cli/cmd_set.c  \
	$(SROUCE)/service/monitor/mon_string.c  \
	$(SROUCE)/service/monitor/mon_get_mem_info.c  \
	$(SROUCE)/service/monitor/mon_get_cpu_info.c  \
	$(SROUCE)/service/monitor/mon_get_disk_info.c  \
	$(SROUCE)/service/monitor/mon_get_net_info.c  \
	$(SROUCE)/service/monitor/mon_get_proc_info.c  \
	$(SROUCE)/service/monitor/mon_warning_msg_queue.c  \
	$(SROUCE)/service/monitor/mon_notification.c  \
	$(SROUCE)/service/monitor/mon_monitor_and_handle.c  \
	$(SROUCE)/service/monitor/mon_lib.c 
	

C_OBJ_FILE += main.$(SUFFIX) \
	root.$(SUFFIX) \
	telnetd.$(SUFFIX) \
	cli_server.$(SUFFIX) \
	cmd_set.$(SUFFIX) \
	mon_string.$(SUFFIX) \
	mon_get_mem_info.$(SUFFIX) \
	mon_get_cpu_info.$(SUFFIX) \
	mon_get_disk_info.$(SUFFIX) \
	mon_get_net_info.$(SUFFIX) \
	mon_get_proc_info.$(SUFFIX) \
	mon_warning_msg_queue.$(SUFFIX) \
	mon_notification.$(SUFFIX) \
	mon_monitor_and_handle.$(SUFFIX) \
	mon_lib.$(SUFFIX)

main.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/main.c

root.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/root.c
	
telnetd.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/telnetd/telnetd.c
	
cli_server.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/cli/cli_server.c

cmd_set.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/cli/cmd_set.c
	
mon_string.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_string.c
	
mon_get_mem_info.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_get_mem_info.c
	
mon_get_cpu_info.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_get_cpu_info.c
	
mon_get_disk_info.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_get_disk_info.c
	
mon_get_net_info.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_get_net_info.c
	
mon_get_proc_info.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_get_proc_info.c
	
mon_warning_msg_queue.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_warning_msg_queue.c
	
mon_notification.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_notification.c
	
mon_monitor_and_handle.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_monitor_and_handle.c
	
mon_lib.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/monitor/mon_lib.c
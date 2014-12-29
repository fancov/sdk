C_FILE += $(SROUCE)/service/main.c \
			$(SROUCE)/service/root.c \
			$(SROUCE)/service/telnetd/telnetd.c \
			$(SROUCE)/service/cli/cli_server.c \
			$(SROUCE)/service/cli/cmd_set.c 

C_OBJ_FILE += main.$(SUFFIX) \
				root.$(SUFFIX) \
				telnetd.$(SUFFIX) \
				cli_server.$(SUFFIX) \
				cmd_set.$(SUFFIX)


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
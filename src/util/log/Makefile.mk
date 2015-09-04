C_FILE += $(SROUCE)/util/log/log_console.c \
			$(SROUCE)/util/log/log_cli.c \
			$(SROUCE)/util/log/log_db.c \
			$(SROUCE)/util/log/log_cmd.c \
            $(SROUCE)/util/log/dos_log.c
			

C_OBJ_FILE += log_console.$(SUFFIX) \
			log_cli.$(SUFFIX) \
			log_db.$(SUFFIX) \
			log_cmd.$(SUFFIX) \
            dos_log.$(SUFFIX)

log_console.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/log/log_console.c

log_cli.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/log/log_cli.c

log_db.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/log/log_db.c
	
log_cmd.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/log/log_cmd.c

dos_log.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/log/dos_log.c

C_FILE += $(SROUCE)/util/config/config_api.c \
			$(SROUCE)/util/config/dos_config.c \
			$(SROUCE)/util/config/config_hb_srv.c \
            $(SROUCE)/util/config/dos_db_config.c \
            $(SROUCE)/util/config/config_warn_config.c
			

C_OBJ_FILE += config_api.$(SUFFIX) \
			dos_config.$(SUFFIX) \
			config_hb_srv.$(SUFFIX) \
            dos_db_config.$(SUFFIX) \
            config_warn_config.$(SUFFIX)

config_api.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/config/config_api.c
	
dos_config.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/config/dos_config.c

config_hb_srv.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/config/config_hb_srv.c

dos_db_config.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/config/dos_db_config.c

config_warn_config.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/config/config_warn_config.c	

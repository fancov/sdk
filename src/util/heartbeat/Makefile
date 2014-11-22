C_FILE += $(SROUCE)/util/heartbeat/hb_client.c \
			$(SROUCE)/util/heartbeat/hb_server.c \
			$(SROUCE)/util/heartbeat/hb_lib.c 

C_OBJ_FILE += hb_client.$(SUFFIX) \
				hb_server.$(SUFFIX) \
				hb_lib.$(SUFFIX)

hb_client.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/heartbeat/hb_client.c
	
hb_server.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/heartbeat/hb_server.c
	
hb_lib.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/heartbeat/hb_lib.c

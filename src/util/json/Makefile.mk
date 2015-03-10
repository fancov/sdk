C_FILE += $(SROUCE)/util/json/json.c 			

C_OBJ_FILE += json.$(SUFFIX)

json.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/json/json.c


C_FILE += $(SROUCE)/service/mc/mc_ini.c \
			$(SROUCE)/service/mc/mc_debug.c

C_OBJ_FILE += mc_ini.$(SUFFIX) \
				mc_debug.$(SUFFIX)


mc_ini.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/mc/mc_ini.c

mc_debug.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/mc/mc_debug.c

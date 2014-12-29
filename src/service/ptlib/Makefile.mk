C_FILE += $(SROUCE)/service/ptlib/dos_sqlite3.c \
			$(SROUCE)/service/ptlib/msg_public.c 

C_OBJ_FILE += dos_sqlite3.$(SUFFIX) \
				msg_public.$(SUFFIX)


dos_sqlite3.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/ptlib/dos_sqlite3.c  -lsqlite3

msg_public.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/ptlib/msg_public.c

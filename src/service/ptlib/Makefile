C_FILE += $(SROUCE)/service/ptlib/dos_sqlite3.c \
			$(SROUCE)/service/ptlib/pt.c \
			$(SROUCE)/service/ptlib/md5.c \
			$(SROUCE)/service/ptlib/des.c 

C_OBJ_FILE += dos_sqlite3.$(SUFFIX) \
				pt.$(SUFFIX) \
				md5.$(SUFFIX) \
				des.$(SUFFIX)


dos_sqlite3.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/ptlib/dos_sqlite3.c  -lsqlite3

pt.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/ptlib/pt.c

md5.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/ptlib/md5.c

des.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/ptlib/des.c

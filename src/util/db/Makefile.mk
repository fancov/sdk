C_FILE += $(SROUCE)/util/db/dos_db.c \
			$(SROUCE)/util/db/driver/db_mysql.c \
			$(SROUCE)/util/db/driver/db_sqlite.c

C_OBJ_FILE += dos_db.$(SUFFIX) \
				db_mysql.$(SUFFIX) \
				db_sqlite.$(SUFFIX)

dos_db.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/db/dos_db.c
	
db_mysql.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/db/driver/db_mysql.c
	
db_sqlite.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/db/driver/db_sqlite.c

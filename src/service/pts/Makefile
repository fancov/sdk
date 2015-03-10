C_FILE += $(SROUCE)/service/pts/pts.c \
			$(SROUCE)/service/pts/pts_msg.c \
			$(SROUCE)/service/pts/pts_web.c \
			$(SROUCE)/service/pts/pts_telnet.c \
			$(SROUCE)/service/pts/pts_goahead.c 

C_OBJ_FILE += pts.$(SUFFIX) \
				pts_msg.$(SUFFIX) \
				pts_web.$(SUFFIX) \
				pts_telnet.$(SUFFIX) \
				pts_goahead.$(SUFFIX)

pts.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/pts/pts.c

pts_msg.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/pts/pts_msg.c

pts_web.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/pts/pts_web.c

pts_telnet.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/pts/pts_telnet.c
	
pts_goahead.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/pts/pts_goahead.c
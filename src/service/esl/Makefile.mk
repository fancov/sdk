C_FILE += $(SROUCE)/service/esl/esl.c \
	$(SROUCE)/service/esl/esl_buffer.c \
	$(SROUCE)/service/esl/esl_config.c \
	$(SROUCE)/service/esl/esl_event.c \
	$(SROUCE)/service/esl/esl_json.c  \
	$(SROUCE)/service/esl/esl_threadmutex.c
	

C_OBJ_FILE += esl.$(SUFFIX) \
	esl_buffer.$(SUFFIX) \
	esl_config.$(SUFFIX) \
	esl_event.$(SUFFIX) \
	esl_json.$(SUFFIX) \
	esl_threadmutex.$(SUFFIX) 

esl.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/esl/esl.c

esl_buffer.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/esl/esl_buffer.c
	
esl_config.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/esl/esl_config.c
	
esl_event.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/esl/esl_event.c

esl_json.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/esl/esl_json.c
	
esl_threadmutex.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/service/esl/esl_threadmutex.c

CFILE += $(SROUCE)/util/http_api/http_api.c \
	$(SROUCE)/util/http_api/mongoose.c \
	$(SROUCE)/util/http_api/http_api_reg.c
	
C_OBJ_FILE += http_api.$(SUFFIX) \
	mongoose.$(SUFFIX) \
	http_api_reg.$(SUFFIX)

http_api.$(SUFFIX):
	$(C_COMPILE) $(SROUCE)/util/http_api/http_api.c

mongoose.$(SUFFIX):
	$(C_COMPILE) $(SROUCE)/util/http_api/mongoose.c
	
http_api_reg.$(SUFFIX):
	$(C_COMPILE) $(SROUCE)/util/http_api/http_api_reg.c
	
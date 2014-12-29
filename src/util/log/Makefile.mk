CXX_FILE += $(SROUCE)/util/log/_log_console.cpp \
			$(SROUCE)/util/log/_log_file.cpp \
			$(SROUCE)/util/log/_log_cli.cpp \
			$(SROUCE)/util/log/_log_db.cpp \
			$(SROUCE)/util/log/dos_log.cpp \
			$(SROUCE)/util/log/log_cmd.cpp
			

CXX_OBJ_FILE += _log_console.$(SUFFIX) \
			_log_file.$(SUFFIX) \
			_log_cli.$(SUFFIX) \
			_log_db.$(SUFFIX) \
			dos_log.$(SUFFIX) \
			log_cmd.$(SUFFIX)

_log_console.$(SUFFIX) :
	$(CXX_COMPILE) $(SROUCE)/util/log/_log_console.cpp
	
_log_file.$(SUFFIX) :
	$(CXX_COMPILE) $(SROUCE)/util/log/_log_file.cpp

_log_cli.$(SUFFIX) :
	$(CXX_COMPILE) $(SROUCE)/util/log/_log_cli.cpp

_log_db.$(SUFFIX) :
	$(CXX_COMPILE) $(SROUCE)/util/log/_log_db.cpp

dos_log.$(SUFFIX) :
	$(CXX_COMPILE) $(SROUCE)/util/log/dos_log.cpp
	
log_cmd.$(SUFFIX) :
	$(CXX_COMPILE) $(SROUCE)/util/log/log_cmd.cpp


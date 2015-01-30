########################################################################
#                         Define the Complie                           # 
########################################################################
TARGET					= pts

########################################################################
#                      Define some                           # 
########################################################################
PROJECT_ROOT			= ../..
DOS_LIBS				= -L$(PROJECT_ROOT)/libs
SROUCE					= $(PROJECT_ROOT)/src
DOS_INCLUDES			= -I$(SROUCE)/include

SUFFIX					= o

LIBS					= -lpthread -lmysqlclient -lmxml -lsqlite3

MYSQL_HEADERS			= -I/usr/include/mysql
MYSQL_LIBS				= -L/usr/lib64/mysql



########################################################################
#                      Collect other makefile                          # 
########################################################################
include $(SROUCE)/util/tools/Makefile.mk
include $(SROUCE)/util/log/Makefile.mk
include $(SROUCE)/util/memory/Makefile.mk
include $(SROUCE)/util/debug/Makefile.mk
include $(SROUCE)/util/config/Makefile.mk
include $(SROUCE)/util/timer/Makefile.mk
include $(SROUCE)/util/hash/Makefile.mk
include $(SROUCE)/util/list/Makefile.mk
include $(SROUCE)/util/heartbeat/Makefile.mk
include $(SROUCE)/util/db/Makefile.mk
include $(SROUCE)/cfg/Makefile.mk
# include $(SROUCE)/service/Makefile
include $(SROUCE)/service/pts/Makefile.mk
include $(SROUCE)/service/ptlib/Makefile.mk


########################################################################
#                              All Files                               # 
########################################################################
ALL_OBJS	= $(C_OBJ_FILE) $(CXX_OBJ_FILE) 
ALL_SRCS	= $(C_FILE) $(CXX_FILE)
ALL_HEADERS	= $(DOS_INCLUDES) $(MYSQL_HEADERS)
ALL_LIBS	= $(MYSQL_LIBS) $(DOS_LIBS) $(LIBS) 

########################################################################
#                         Define the Complie                           # 
########################################################################
CC						= gcc
CXX						= g++
CFLAGS					= -Wall -rdynamic -g -ggdb -fsigned-char
DEFINES					= -DDIPCC_PTS -DDEBUG_VERSION

C_COMPILE				= $(CC) -c $(CFLAGS) $(DEFINES) $(ALL_HEADERS) 
CXX_COMPILE				= $(CXX) -c $(CFLAGS) $(DEFINES) $(ALL_HEADERS)
C_LINKS					= $(CXX) -o $@


########################################################################
#                               Target                                 # 
######################################################################## 
dep :
	$(CC) -MM $(ALL_SRCS) $(ALL_HEADERS) $(DEFINES) > $(TARGET).dep

all : $(ALL_OBJS)
	$(C_LINKS) -o $(TARGET)d $(ALL_OBJS) $(CFLAGS) $(DEFINES) $(ALL_HEADERS) $(ALL_LIBS)

clean :
	rm -f *.o
	rm -f $(TARGET)d

include $(TARGET).dep

#ifndef _MON_DEF_H_
#define _MON_DEF_H_

#include "mon_lib.h"

VOID mon_show_cpu(U32 ulIndex);
VOID mon_show_mem(U32 ulIndex);
VOID mon_show_disk(U32 ulIndex);
VOID mon_show_netcard(U32 ulIndex);
VOID mon_show_process(U32 ulIndex);

#endif

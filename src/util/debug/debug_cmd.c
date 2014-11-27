#include <dos.h>
#include <sys/un.h>

#if INCLUDE_DEBUG_CLI

extern S32 cli_cmd_mem(U32 ulIndex, S32 argc, S8 **argv);
extern S32 cli_set_log_level(U32 ulIndex, S32 argc, S8 **argv);
#if INCLUDE_BH_SERVER
extern S32 bh_process_list(U32 ulIndex, S32 argc, S8 **argv);
#endif

COMMAND_ST g_stCommandSet[] = {
    {NULL, "memory",        "Show memory usage",               cli_cmd_mem},
    {NULL, "debug",         "Set the log level",               cli_set_log_level},
#if INCLUDE_BH_SERVER
    {NULL, "process",       "Show processes reggister in",     bh_process_list}
#endif
};

COMMAND_GROUP_ST g_stCommandRootGrp[] = {
    {NULL,  g_stCommandSet,   sizeof(g_stCommandSet)/sizeof(COMMAND_ST)}
};

COMMAND_ST * debug_cli_cmd_find(S32 argc, S8 **argv)
{
    U32 ulGrpLoop, ulCmdLoop, ulGrpSize;
    COMMAND_GROUP_ST *pstGrpTmp;
    COMMAND_ST       *pstCmdTmp;

    if (!argv || 0 == argc)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    ulGrpSize = sizeof(g_stCommandRootGrp)/sizeof(COMMAND_GROUP_ST);

    for (ulGrpLoop=0; ulGrpLoop<ulGrpSize; ulGrpLoop++)
    {
        pstGrpTmp = &g_stCommandRootGrp[ulGrpLoop];
        for (ulCmdLoop=0; ulCmdLoop<pstGrpTmp->ulSize; ulCmdLoop++)
        {
            pstCmdTmp = &pstGrpTmp->pstCmdSet[ulCmdLoop];
            if (dos_stricmp(pstCmdTmp->pszCommand, argv[0]) == 0)
            {
                return pstCmdTmp;
            }
        }
    }

    return NULL;
}

#endif


#include <dos.h>
#include <sys/un.h>

#if INCLUDE_DEBUG_CLI

extern S32 cli_cmd_mem(U32 ulIndex, S32 argc, S8 **argv);
extern S32 cli_set_log_level(U32 ulIndex, S32 argc, S8 **argv);
#if INCLUDE_BH_SERVER
extern S32 bh_process_list(U32 ulIndex, S32 argc, S8 **argv);
#endif
#if INCLUDE_SERVICE_TIMER
extern S32 cli_show_timer(U32 ulIndex, S32 argc, S8 **argv);
#endif
#if INCLUDE_SERVICE_BS
extern S32 bs_command_proc(U32 ulIndex, S32 argc, S8 **argv);
extern S32 bs_update_test(U32 ulIndex, S32 argc, S8 **argv);

#endif

#if INCLUDE_CC_SC
extern S32 cli_cc_process(U32 ulIndex, S32 argc, S8 **argv);
#endif


COMMAND_ST g_stCommandSet[] = {
    {NULL, "assert",        "Show assert informationa",        dos_assert_print},
#if INCLUDE_SERVICE_BS
    {NULL, "bs",            "BS command",                      bs_command_proc},
    /*bs?web????*/ 
    {NULL, "bst",           "bs test",                         bs_update_test},
#endif
    {NULL, "debug",         "Set the log level",               cli_set_log_level},
#if INCLUDE_MEMORY_MNGT
    {NULL, "memory",        "Show memory usage",               cli_cmd_mem},
#endif
#if INCLUDE_BH_SERVER
    {NULL, "process",       "Show processes reggister in",     bh_process_list},
#endif
#if INCLUDE_SERVICE_TIMER
    {NULL, "timer",         "Show timer usage",                cli_show_timer},
#endif
#if INCLUDE_CC_SC
    {NULL, "cc",            "Debug CC mod",                    cli_cc_process},
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


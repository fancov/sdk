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

#if INCLUDE_RES_MONITOR
extern S32 mon_command_proc(U32 ulIndex, S32 argc, S8 ** argv);
#endif

#if INCLUDE_PTS
extern S32 pts_send_command_to_ptc(U32 ulIndex, S32 argc, S8 **argv);
extern S32 pts_telnet_cmd(U32 ulIndex, S32 argc, S8 **argv);
extern S32 pts_printf_web_msg(U32 ulIndex, S32 argc, S8 **argv);
extern S32 pts_printf_recv_cache_msg(U32 ulIndex, S32 argc, S8 **argv);
extern S32 pts_printf_send_cache_msg(U32 ulIndex, S32 argc, S8 **argv);
extern S32 pts_trace_debug_ptc(U32 ulIndex, S32 argc, S8 **argv);
#endif

#if INCLUDE_PTC
extern S32 ptc_printf_telnet_msg(U32 ulIndex, S32 argc, S8 **argv);
extern S32 ptc_printf_web_msg(U32 ulIndex, S32 argc, S8 **argv);
extern S32 ptc_printf_recv_cache_msg(U32 ulIndex, S32 argc, S8 **argv);
extern S32 ptc_printf_send_cache_msg(U32 ulIndex, S32 argc, S8 **argv);
#endif

#if INCLUDE_SERVICE_MC
extern S32 mc_cmd_set(U32 ulIndex, S32 argc, S8 **argv);
#endif

COMMAND_ST g_stCommandSet[] = {
    {NULL, "assert",        "Show assert informationa",        dos_assert_print},
#if INCLUDE_SERVICE_BS
    {NULL, "bs",            "BS command",                      bs_command_proc},
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

#if INCLUDE_PTS
    {NULL, "ptc",           "send cmd to PTC",                  pts_send_command_to_ptc},
    {NULL, "web",           "printf web CB",                    pts_printf_web_msg},
    {NULL, "recv",          "printf recv CB",                   pts_printf_recv_cache_msg},
    {NULL, "send",          "printf send CB",                   pts_printf_send_cache_msg},
    {NULL, "trace",         "trace debug a PTC",                pts_trace_debug_ptc},
    {NULL, "telnet",        "",                                 pts_telnet_cmd},
#endif

#if INCLUDE_PTC
    {NULL, "telnet",        "printf telnet CB",                 ptc_printf_telnet_msg},
    {NULL, "web",           "printf web CB",                    ptc_printf_web_msg},
    {NULL, "recv",          "printf recv CB",                   ptc_printf_recv_cache_msg},
    {NULL, "send",          "printf send CB",                   ptc_printf_send_cache_msg},
#endif

#if INCLUDE_RES_MONITOR
    {NULL, "mon",           "Monitor debug",                   mon_command_proc},
#endif

#if INCLUDE_SERVICE_MC
    {NULL, "mc",            "Media convert",                   mc_cmd_set},
#endif

    {NULL, NULL,            "",                               NULL}
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


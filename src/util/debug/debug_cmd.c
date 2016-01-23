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
extern S32 sc_cc_show_agent_stat(U32 ulIndex, S32 argc, S8 **argv);
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

#if INCLUDE_CC_SC
COMMAND_ST g_stCommandSCStat[] = {
    {NULL, "agent",         "Show agent stat info",           sc_cc_show_agent_stat},
    {NULL, NULL,            "",                               NULL}
};

COMMAND_GROUP_ST g_stCommandSCStatGroup[] = {
    {NULL,  g_stCommandSCStat,   sizeof(g_stCommandSCStat)/sizeof(COMMAND_ST)},
    {NULL,  NULL,                0}
};
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
    {NULL,                   "cc",            "Debug CC mod",  cli_cc_process},
    {g_stCommandSCStatGroup, "stat",          "SC cmd group",  NULL},
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
    {NULL,  g_stCommandSet,   sizeof(g_stCommandSet)/sizeof(COMMAND_ST)},
    {NULL,  NULL,             0}
};

COMMAND_ST * debug_cli_cmd_find(COMMAND_GROUP_ST *pstCmdGrp, S32 argc, S8 **argv)
{
    U32 ulGrpLoop, ulCmdLoop;
    COMMAND_GROUP_ST *pstGrpTmp;
    COMMAND_ST       *pstCmdTmp;

    if (!argv || 0 == argc)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    if (!pstCmdGrp)
    {
        pstCmdGrp = g_stCommandRootGrp;
    }

    for (ulGrpLoop=0
        ; pstCmdGrp && pstCmdGrp[ulGrpLoop].pstCmdSet && pstCmdGrp[ulGrpLoop].pstCmdSet > 0
        ; ulGrpLoop++, pstCmdGrp++)
    {
        pstGrpTmp = &pstCmdGrp[ulGrpLoop];
        for (ulCmdLoop=0; ulCmdLoop<pstGrpTmp->ulSize; ulCmdLoop++)
        {
            pstCmdTmp = &pstGrpTmp->pstCmdSet[ulCmdLoop];

            if (dos_stricmp(pstCmdTmp->pszCommand, argv[0]) == 0)
            {
                if (pstCmdTmp->pstGroup)
                {
                    return debug_cli_cmd_find(pstCmdTmp->pstGroup, argc-1, argv+1);
                }
                else
                {
                    return pstCmdTmp;
                }
            }
        }
    }

    return NULL;
}

#endif


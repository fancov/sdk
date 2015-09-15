#include <dos.h>

#include "mon_def.h"

#include "mon_get_mem_info.h"
#include "mon_get_cpu_info.h"
#include "mon_get_disk_info.h"
#include "mon_get_net_info.h"
#include "mon_get_proc_info.h"

extern MON_SYS_MEM_DATA_S * g_pstMem;
extern MON_CPU_RSLT_S     * g_pstCpuRslt;
extern MON_SYS_PART_DATA_S * g_pastPartition[MAX_PARTITION_COUNT];
extern MON_NET_CARD_PARAM_S * g_pastNet[MAX_NETCARD_CNT];
extern MON_PROC_STATUS_S * g_pastProc[MAX_PROC_CNT];

extern S32 g_ulNetCnt;
extern U32 g_ulPartCnt;

#define MON_TRACE_BUFF_LEN  1024

S32 mon_command_proc(U32 ulIndex, S32 argc, S8 **argv)
{
    S8  szBuff[1024] = {0};

    if (3 != argc)
    {
        dos_snprintf(szBuff, sizeof(szBuff), "\r\nYou should input 3 params,but you input %u params.\r\n", argc);
        cli_out_string(ulIndex, szBuff);
        goto help;
    }

    if (0 != dos_stricmp(argv[1], "show"))
    {
        dos_snprintf(szBuff, sizeof(szBuff), "\r\nThe param 1 should be \"show\", but your param is \"%s\".\r\n", argv[1]);
        cli_out_string(ulIndex, szBuff);
        goto help;
    }

    if (0 == dos_stricmp(argv[2], "memory"))
    {
        mon_show_mem(ulIndex);
    }
    else if (0 == dos_stricmp(argv[2], "cpu"))
    {
        mon_show_cpu(ulIndex);
    }
    else if (0 == dos_stricmp(argv[2], "disk"))
    {
        mon_show_disk(ulIndex);
    }
    else if (0 == dos_stricmp(argv[2], "net"))
    {
        mon_show_netcard(ulIndex);
    }
    else if (0 == dos_stricmp(argv[2], "process"))
    {
        mon_show_process(ulIndex);
    }
    else if (0 == dos_stricmp(argv[2], "all"))
    {
        mon_show_mem(ulIndex);
        mon_show_cpu(ulIndex);
        mon_show_disk(ulIndex);
        mon_show_netcard(ulIndex);
        mon_show_process(ulIndex);
    }
    else
    {
        dos_snprintf(szBuff, sizeof(szBuff), "\r\nYour param 2 is \"%s\",it is not supported.\r\n", argv[2]);
        cli_out_string(ulIndex, szBuff);
        goto help;
    }

    dos_snprintf(szBuff, sizeof(szBuff), "\r\nList %s information(s) finished.", argv[2]);
    cli_out_string(ulIndex, szBuff);

    return DOS_SUCC;

help:
    cli_out_string(ulIndex, "\r\nHelp:\r\n  monitord mon show memory|cpu|disk|net|process|all\r\n");
    return DOS_FAIL;
}

VOID mon_show_mem(U32 ulIndex)
{
    S8 szBuff[1024] = {0};

    dos_snprintf(szBuff, sizeof(szBuff), "\r\nList System Memory Information.");
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n+--------------------+-------------------+--------------+---------------+------------------+-----------------+--------------+------------+");
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n|Total Memory(KBytes)|Free Memory(KBytes)|Cached(KBytes)|Buffers(KBytes)|Total Swap(KBytes)|Free Swap(KBytes)|Memory Rate(%)|Swap Rate(%%)|");
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n+--------------------+-------------------+--------------+---------------+------------------+-----------------+--------------+------------+");
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n|%20u|%19u|%14u|%15u|%18u|%17u|%14u|%12u|"
                    , g_pstMem->ulPhysicalMemTotalBytes
                    , g_pstMem->ulPhysicalMemFreeBytes
                    , g_pstMem->ulCached
                    , g_pstMem->ulBuffers
                    , g_pstMem->ulSwapTotalBytes
                    , g_pstMem->ulSwapFreeBytes
                    , g_pstMem->ulPhysicalMemUsageRate
                    , g_pstMem->ulSwapUsageRate);
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n+--------------------+-------------------+--------------+---------------+------------------+-----------------+--------------+------------+\r\n");
    cli_out_string(ulIndex, szBuff);
}

VOID mon_show_cpu(U32 ulIndex)
{
    S8 szBuff[1024] = {0};


    dos_snprintf(szBuff, sizeof(szBuff), "\r\nList System CPU Information.");
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n+----------------+-------------+---------------+----------------+");
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n|Total CPU Avg(%%)|5s CPU Avg(%%)|1min CPU Avg(%%)|10min CPU Avg(%%)|");
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n+----------------+-------------+---------------+----------------+");
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n|%16u|%13u|%15u|%16u|"
                    , g_pstCpuRslt->ulCPUUsageRate
                    , g_pstCpuRslt->ulCPU5sUsageRate
                    , g_pstCpuRslt->ulCPU1minUsageRate
                    , g_pstCpuRslt->ulCPU10minUsageRate);
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n+----------------+-------------+---------------+----------------+\r\n");
    cli_out_string(ulIndex, szBuff);
}

VOID mon_show_disk(U32 ulIndex)
{
    S8  szBuff[1024] = {0};
    U32 ulLoop = 0;

    dos_snprintf(szBuff, sizeof(szBuff), "\r\nList System disk Information.");
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n+--------------------------------+------------------+-------------+-------------------+");
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n|         Partition Name         |Avail Disk(KBytes)|Usage Rate(%%)|     Serial NO     |");
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n+--------------------------------+------------------+-------------+-------------------+");
    cli_out_string(ulIndex, szBuff);

    for (ulLoop = 0; ulLoop < g_ulPartCnt; ++ulLoop)
    {
        dos_snprintf(szBuff, sizeof(szBuff), "\r\n|%-32s|%18u|%13u|%-19s|"
                        , g_pastPartition[ulLoop]->szPartitionName
                        , g_pastPartition[ulLoop]->uLPartitionAvailBytes
                        , g_pastPartition[ulLoop]->uLPartitionUsageRate
                        , g_pastPartition[ulLoop]->szDiskSerialNo);
        cli_out_string(ulIndex, szBuff);
    }

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n+--------------------------------+------------------+-------------+-------------------+");
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n| Total Disk KBytes:%12u;  Total Disk Usage Rate:%3u%%                         |"
                    , mon_get_total_disk_bytes()
                    , mon_get_total_disk_usage_rate());
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n+-------------------------------------------------------------------------------------+\r\n");
    cli_out_string(ulIndex, szBuff);
}

VOID mon_show_netcard(U32 ulIndex)
{
    S8  szBuff[1024] = {0};
    U32 ulLoop = 0;

    dos_snprintf(szBuff, sizeof(szBuff), "\r\nList Netcard Information.");
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n+---------+---------------------+--------------------+--------------------+--------------------+--------------------+");
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n| Netcard |     Mac Address     |     IP Address     |  Broad IP Address  |      Net Mask      |Transmit Speed(KB/s)|");
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n+---------+---------------------+--------------------+--------------------+--------------------+--------------------+");
    cli_out_string(ulIndex, szBuff);

    for (ulLoop = 0; ulLoop < g_ulNetCnt; ++ulLoop)
    {
        dos_snprintf(szBuff, sizeof(szBuff), "\r\n|%-9s|%-21s|%-20s|%-20s|%-20s|%20u|"
                        , g_pastNet[ulLoop]->szNetDevName
                        , g_pastNet[ulLoop]->szMacAddress
                        , g_pastNet[ulLoop]->szIPAddress
                        , g_pastNet[ulLoop]->szBroadIPAddress
                        , g_pastNet[ulLoop]->szNetMask
                        , g_pastNet[ulLoop]->ulRWSpeed);
        cli_out_string(ulIndex, szBuff);
    }

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n+---------+---------------------+--------------------+--------------------+--------------------+--------------------+\r\n");
    cli_out_string(ulIndex, szBuff);
}


VOID mon_show_process(U32 ulIndex)
{
    S8  szBuff[1024] = {0};
    U32 ulLoop = 0;

    dos_snprintf(szBuff, sizeof(szBuff), "\r\nList Process Information.");
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n+-------+------------+----------------+-------------+----------+---------------+-------------------+---------------+");
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n|  PID  |    Name    | Memory Rate(%%) | CPU Rate(%%) | CPU Time |Open File Count|DB Connection Count| Threads Count |");
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n+-------+------------+----------------+-------------+----------+---------------+-------------------+---------------+");
    cli_out_string(ulIndex, szBuff);

    for (ulLoop = 0; ulLoop < MAX_PROC_CNT; ++ulLoop)
    {
        dos_snprintf(szBuff, sizeof(szBuff), "\r\n|%7d|%-12s|%16.1f|%13.1f|%-10s|%15u|%19u|%15u|"
                        , g_pastProc[ulLoop]->ulProcId
                        , mon_str_get_name(g_pastProc[ulLoop]->szProcName)
                        , g_pastProc[ulLoop]->fMemoryRate
                        , g_pastProc[ulLoop]->fCPURate
                        , g_pastProc[ulLoop]->szProcCPUTime
                        , g_pastProc[ulLoop]->ulOpenFileCnt
                        , g_pastProc[ulLoop]->ulDBConnCnt
                        , g_pastProc[ulLoop]->ulThreadsCnt);
        cli_out_string(ulIndex, szBuff);
    }

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n+-------+------------+----------------+-------------+----------+---------------+-------------------+---------------+\r\n");
    cli_out_string(ulIndex, szBuff);
}


VOID mon_trace(U32 ulTraceTarget, U8 ucTraceLevel, const S8 * szFormat, ...)
{
    va_list         Arg;
    U32             ulTraceTagLen;
    S8              szTraceStr[MON_TRACE_BUFF_LEN];
    BOOL            bIsOutput = DOS_FALSE;

    if (ucTraceLevel >= LOG_LEVEL_INVAILD)
    {
        return;
    }

    /* warning级别以上强制输出 */
    if (ucTraceLevel <= LOG_LEVEL_WARNING)
    {
        bIsOutput = DOS_TRUE;
    }

    if (!bIsOutput)
    {
        bIsOutput = DOS_TRUE;
    }

    if(!bIsOutput)
    {
        return;
    }

    switch(ulTraceTarget)
    {
        case MON_TRACE_MEM:
            dos_strcpy(szTraceStr, "MON-MEM:");
            break;
        case MON_TRACE_CPU:
            dos_strcpy(szTraceStr, "MON-CPU:");
            break;
        case MON_TRACE_DISK:
            dos_strcpy(szTraceStr, "MON-DISK:");
            break;
        case MON_TRACE_NET:
            dos_strcpy(szTraceStr, "MON-NET:");
            break;
        case MON_TRACE_PROCESS:
            dos_strcpy(szTraceStr, "MON-PROCESS:");
            break;
        case MON_TRACE_MH:
            dos_strcpy(szTraceStr, "MON-MH:");
            break;
        case MON_TRACE_LIB:
            dos_strcpy(szTraceStr, "MON-LIB:");
            break;
        case MON_TRACE_NOTIFY:
            dos_strcpy(szTraceStr, "MON-NOTIFY:");
            break;
        case MON_TRACE_PUB:
            dos_strcpy(szTraceStr, "MON-PUB:");
            break;
        case MON_TRACE_WARNING_MSG:
            dos_strcpy(szTraceStr, "MON-WARNING-MSG:");
            break;
        default:
            dos_strcpy(szTraceStr, "MON-BUTT:");
            break;
    }

    ulTraceTagLen = dos_strlen(szTraceStr);

    va_start(Arg, szFormat);
    vsnprintf(szTraceStr + ulTraceTagLen, sizeof(szTraceStr) - ulTraceTagLen, szFormat, Arg);
    va_end(Arg);
    szTraceStr[sizeof(szTraceStr) -1] = '\0';

    dos_log(ucTraceLevel, LOG_TYPE_RUNINFO, szTraceStr);
}



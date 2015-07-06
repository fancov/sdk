/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  config_hb_srv.c
 *
 *  Created on: 2014-11-14
 *      Author: Larry
 *        Todo: 统一配置项管理，由使用者自己维护
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include dos public header files */
#include <dos.h>

#if INCLUDE_BH_SERVER
/* include provate header file */
#include "config_api.h"

#if (INCLUDE_XML_CONFIG)

/* provate macro */
#define HB_CONFIG_FILE_PATH1 "/etc"
#define HB_CONFIG_FILE_PATH2 "../etc"

#define HB_CONFIG_FILE_NAME "hb-srv.xml"


/* global variable */
/* 全局配置句柄 */
static mxml_node_t *g_pstHBSrvCfg = NULL;


/**
 * 函数：config_hb_get_process_list
 * 功能：获取指定index的路径下的进程信息
 * 参数：
 *      U32 ulIndex：xml中process的路径编号
 *      S8 *pszName：输出参数，存放进程名的缓存
 *      U32 ulNameLen：存放进程名缓存的长度
 *      S8 *pszVersion：输出参数，存放进程版本号
 *      U32 ulVerLen：存放进程版本号缓存的长度
 * 返回值：
 *      成功返回0，失败返回－1
 */
S32 config_hb_get_process_list(U32 ulIndex, S8 *pszName, U32 ulNameLen, S8 *pszVersion, U32 ulVerLen)
{
    S8 szXmlPath[256] = { 0 };
    S8 szProcessName[DOS_MAX_PROCESS_NAME_LEN] = { 0 };
    S8 szProcessVersion[DOS_MAX_PROCESS_VER_LEN] = { 0 };
    S8 *pszValue = NULL;

    if (!pszName || 0 == ulNameLen)
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (!pszVersion || 0 == ulVerLen)
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (!g_pstHBSrvCfg)
    {
        DOS_ASSERT(0);
        return -1;
    }

    snprintf(szXmlPath, sizeof(szXmlPath), "config/process/%d", ulIndex);
    pszValue = _config_get_param(g_pstHBSrvCfg, szXmlPath, "name", szProcessName, sizeof(szProcessName));
    if (!pszValue || '\0' == pszValue[0])
    {
        return -1;
    }
    pszValue = _config_get_param(g_pstHBSrvCfg, szXmlPath, "version", szProcessVersion, sizeof(szProcessVersion));

    dos_strncpy(pszName, szProcessName, ulNameLen);
    pszName[ulNameLen - 1] = '\0';

    if (!pszValue || '\0' == pszValue[0])
    {
        dos_strncpy(pszVersion, szProcessVersion, ulVerLen);
        pszVersion[ulVerLen - 1] = '\0';
    }
    else
    {
        pszVersion[0] = '\0';
    }

    return 0;
}


/**
 * 函数：S32 config_hb_init()
 * 功能： 初始化心跳模块的配置
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数初始化配置模块。配置文件的搜索路径顺序为：
 *      1.系统配置文件目录
 *      2."../etc/" 把当前目录看作bin目录，上层目录为当前服务根目录，在服务根目录下面查找
 */
S32 config_hb_init()
{
    S8 szBuffFilename[256];

    dos_printf("%s", "Start init hb-srv config.");

    snprintf(szBuffFilename, sizeof(szBuffFilename)
                , "%s/%s", HB_CONFIG_FILE_PATH1, HB_CONFIG_FILE_NAME);

    if (access(szBuffFilename, R_OK|W_OK) < 0)
    {
        dos_printf("Cannon find the hb-srv.xml file in system etc. (%s)", szBuffFilename);

        snprintf(szBuffFilename, sizeof(szBuffFilename)
                        , "%s/%s", HB_CONFIG_FILE_PATH2, HB_CONFIG_FILE_NAME);
        if (access(szBuffFilename, R_OK|W_OK) < 0)
        {
            dos_printf("Cannon find the hb-srv.xml file in service etc. (%s)", szBuffFilename);
            dos_printf("%s", "Cannon find the hb-srv.xml. Config init failed.");
            DOS_ASSERT(0);
            return -1;
        }
    }

    dos_printf("Use file %s as hb server config file.", szBuffFilename);

    g_pstHBSrvCfg = _config_init(szBuffFilename);
    if (!g_pstHBSrvCfg)
    {
        dos_printf("%s", "Init hb-srv.xml config fail.");
        DOS_ASSERT(0);
        return -1;
    }
    dos_printf("%s", "hb-srv config init successfully.");

    return 0;
}

/**
 * 函数：S32 config_hb_deinit()
 * 功能： 销毁心跳模块配置
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_hb_deinit()
{
    S32 lRet = 0;

    lRet = _config_deinit(g_pstHBSrvCfg);
    g_pstHBSrvCfg = NULL;

    return lRet;
}

/**
 * 函数：S32 config_hb_save()
 * 功能：保存心跳模块配置文件
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_hb_save()
{
    S8 szBuffFilename[256];

    dos_printf("%s", "Start save hb server config.");

    snprintf(szBuffFilename, sizeof(szBuffFilename)
                , "%s/%s", HB_CONFIG_FILE_PATH1, HB_CONFIG_FILE_NAME);

    if (access(szBuffFilename, R_OK|W_OK) < 0)
    {
        dos_printf("Cannon find the hb-srv.xml file in system etc. (%s)", szBuffFilename);

        snprintf(szBuffFilename, sizeof(szBuffFilename)
                        , "%s/%s", HB_CONFIG_FILE_PATH2, HB_CONFIG_FILE_NAME);
        if (access(szBuffFilename, R_OK|W_OK) < 0)
        {
            dos_printf("Cannon find the hb-srv.xml file in service etc. (%s)", szBuffFilename);
            dos_printf("%s", "Cannon find the hb-srv.xml. save failed.");
            DOS_ASSERT(0);
            return -1;
        }
    }

    dos_printf("Save process list to file %s.", szBuffFilename);

    if (_config_save(g_pstHBSrvCfg, szBuffFilename) < 0)
    {
        DOS_ASSERT(0);
        return -1;
    }

    return 0;
}

/**
 * 函数：config_hb_get_process_cfg_cnt
 * 功能：获取配置的进程数量
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_hb_get_process_cfg_cnt()
{
   S32 lCfgProcCnt = 0;
   S8  szCfgProcCnt[4] = {0};
   S8* pszCfgProcCnt = NULL;
   S8  szBuff[32] = {0};
   S32 lRet = 0;

   if(!g_pstHBSrvCfg)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }
   dos_strncpy(szBuff, "config/process", dos_strlen("config/process"));
   szBuff[dos_strlen("config/process")] = '\0';

   pszCfgProcCnt = _config_get_param(g_pstHBSrvCfg, szBuff, "count"
                    , szCfgProcCnt, sizeof(szCfgProcCnt));
   if(!pszCfgProcCnt)
   {
      dos_printf("%s", "get configured process count failure.");
      DOS_ASSERT(0);
      return -1;
   }

   lRet = dos_atol(pszCfgProcCnt, &lCfgProcCnt);
   if(0 != lRet)
   {
      dos_printf("%s","dos_atol failure!");
      DOS_ASSERT(0);
      return -1;
   }

   return lCfgProcCnt;
}


/**
 * 函数：config_hb_get_start_cmd
 * 功能：获取进程的启动命令
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_hb_get_start_cmd(U32 ulIndex, S8 *pszCmd, U32 ulLen)
{
   S8  szProcCmd[1024] = {0};
   S8  szNodePath[32] = {0};
   S8* pszValue = NULL;

   if(!g_pstHBSrvCfg)
   {
      dos_printf("%s", "get start process command failed!");
      DOS_ASSERT(0);
      return -1;
   }
   dos_snprintf(szNodePath, sizeof(szNodePath), "config/process/%u", ulIndex);

   pszValue = _config_get_param(g_pstHBSrvCfg, szNodePath, "startcmd", szProcCmd, sizeof(szProcCmd));
   if(!pszCmd && *pszCmd == '\0')
   {
      dos_printf("%s", "get start process command failed!");
      DOS_ASSERT(0);
      return -1;
   }
   dos_strncpy(pszCmd, pszValue, dos_strlen(pszValue));
   pszCmd[dos_strlen(pszValue)] = '\0';
   return 0;
}

/**
 * 函数：config_hb_threshold_mem
 * 功能：获取内存占用率的阀值
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_hb_threshold_mem(U32* pulMem)
{
   S8  szNodePath[] = "config/threshold/mem";
   S8  szMemThreshold[4] = {0};
   S8* pszMemThreshold = NULL;
   S32 lRet = 0;

   if(!g_pstHBSrvCfg)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }

   pszMemThreshold = _config_get_param(g_pstHBSrvCfg, szNodePath, "rate", szMemThreshold, sizeof(szMemThreshold));
   if(!pszMemThreshold || '\0' == pszMemThreshold[0])
   {
      dos_printf("%s", "get memory threshold value failed.");
      DOS_ASSERT(0);
      return -1;
   }

   lRet = dos_atoul(pszMemThreshold, pulMem);
   if(0 > lRet)
   {
      dos_printf("%s", "dos_atol failed.");
      DOS_ASSERT(0);
      return -1;
   }

   return 0;
}


/**
 * 函数：config_hb_threshold_cpu
 * 功能：获取cpu四个占用率的阀值
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_hb_threshold_cpu(U32* pulAvg, U32* pul5sAvg, U32 *pul1minAvg, U32 *pul10minAvg)
{
   S8  szAvgCPURateThreshold[4] = {0};
   S8  sz5sAvgCPURateThreshold[4] = {0};
   S8  sz1minAvgCPURateThreshold[4] = {0};
   S8  sz10minAvgCPURateThreshold[4] = {0};
   S8  szNodePath[] = "config/threshold/cpu";

   S8* pszAvgCPURateThreshold = NULL;
   S8* psz5sAvgCPURateThreshold = NULL;
   S8* psz1minAvgCPURateThreshold = NULL;
   S8* psz10minAvgCPURateThreshold = NULL;

   S32 lRet = 0;

   if(!g_pstHBSrvCfg)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }

   pszAvgCPURateThreshold = _config_get_param(g_pstHBSrvCfg, szNodePath, "avgRate"
                            , szAvgCPURateThreshold, sizeof(szAvgCPURateThreshold));
   if(!pszAvgCPURateThreshold)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }

   psz5sAvgCPURateThreshold = _config_get_param(g_pstHBSrvCfg, szNodePath, "5sAvgRate"
                                , sz5sAvgCPURateThreshold, sizeof(sz5sAvgCPURateThreshold));
   if(!psz5sAvgCPURateThreshold)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }

   psz1minAvgCPURateThreshold = _config_get_param(g_pstHBSrvCfg, szNodePath, "1minAvgRate"
                                , sz1minAvgCPURateThreshold, sizeof(sz1minAvgCPURateThreshold));
   if(!psz1minAvgCPURateThreshold)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }

   psz10minAvgCPURateThreshold = _config_get_param(g_pstHBSrvCfg, szNodePath, "10minAvgRate"
                                    , sz10minAvgCPURateThreshold, sizeof(sz10minAvgCPURateThreshold));
   if(!psz10minAvgCPURateThreshold)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }

   lRet = dos_atoul(pszAvgCPURateThreshold, pulAvg);
   if(0 > lRet)
   {
      dos_printf("%s", "dos_atol failed.");
      DOS_ASSERT(0);
      return -1;
   }

   lRet = dos_atoul(psz5sAvgCPURateThreshold, pul5sAvg);
   if(0 > lRet)
   {
      dos_printf("%s", "dos_atol failed.");
      DOS_ASSERT(0);
      return -1;
   }

   lRet = dos_atoul(psz1minAvgCPURateThreshold, pul1minAvg);
   if(0 > lRet)
   {
      dos_printf("%s", "dos_atol failed.");
      DOS_ASSERT(0);
      return -1;
   }

   lRet = dos_atoul(psz10minAvgCPURateThreshold, pul10minAvg);
   if(0 > lRet)
   {
      dos_printf("%s", "dos_atol failed.");
      DOS_ASSERT(0);
      return -1;
   }

   return 0;
}

/**
 * 函数：config_hb_threshold_disk
 * 功能：获取硬盘占用率的阀值
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_hb_threshold_disk(U32 *pulPartition, U32* pulDisk)
{
   S8  szNodePath[] = "config/threshold/disk";
   S8  szPartitionRate[4] = {0};
   S8  szDiskRate[4] = {0};

   S8* pszPartitionRate = NULL;
   S8* pszDiskRate = NULL;

   S32 lRet = 0;

   if(!g_pstHBSrvCfg)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }

   pszPartitionRate = _config_get_param(g_pstHBSrvCfg, szNodePath, "partitionRate"
                        , szPartitionRate, sizeof(szPartitionRate));
   if(!pszPartitionRate)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }

   pszDiskRate = _config_get_param(g_pstHBSrvCfg, szNodePath, "diskRate"
                    , szDiskRate, sizeof(szDiskRate));
   if(!pszDiskRate)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }

   lRet = dos_atoul(pszPartitionRate, pulPartition);
   if(0 > lRet)
   {
      dos_printf("%s", "dos_atol failed.");
      DOS_ASSERT(0);
      return -1;
   }

   lRet = dos_atoul(pszDiskRate, pulDisk);
   if(0 > lRet)
   {
      dos_printf("%s", "dos_atol failed.");
      DOS_ASSERT(0);
      return -1;
   }

   return 0;
}

/**
 * 函数：config_hb_threshold_disk
 * 功能：获取进程内存与CPU占用率阀值
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_hb_threshold_proc(U32* pulMem)
{
   S8  szNodePath[] = "config/threshold/proc";
   S8  szProcMemThres[4] = {0};

   S8* pszProcMemThres = NULL;

   S32 lRet = 0;

   if(!g_pstHBSrvCfg)
   {
      dos_printf("%s", ".Init hb-srv.xml config fail!");
      DOS_ASSERT(0);
      return -1;
   }

   pszProcMemThres = _config_get_param(g_pstHBSrvCfg, szNodePath, "memRate"
                        , szProcMemThres, sizeof(szProcMemThres));
   if(!pszProcMemThres)
   {
      dos_printf("%s", ".Init hb-srv.xml config fail!");
      DOS_ASSERT(0);
      return -1;
   }

   lRet = dos_atoul(pszProcMemThres, pulMem);
   if(0 > lRet)
   {
      dos_printf("%s", "dos_atoul failed!");
      DOS_ASSERT(0);
      return -1;
   }

   return 0;
}

/**
 * 函数：config_hb_threshold_bandwidth
 * 功能：获取网络占用最大带宽
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_hb_threshold_bandwidth(U32* pulBandWidth)
{
    S8 szNodePath[] = "config/threshold/net";
    S8 szMaxBandWidth[8] = {0};

    S8* pszMaxBandWidth = NULL;
    S32 lRet = 0;

    if(!g_pstHBSrvCfg)
    {
        dos_printf("%s", ".Init hb-srv.xml config fail!");
        DOS_ASSERT(0);
        return -1;
    }

    pszMaxBandWidth = _config_get_param(g_pstHBSrvCfg, szNodePath, "maxBandWidth"
                        , szMaxBandWidth, sizeof(szMaxBandWidth));
    if (DOS_ADDR_INVALID(pszMaxBandWidth))
    {
        dos_printf("%s", ".Init hb-srv.xml config fail!");
        DOS_ASSERT(0);
        return -1;
    }

    lRet = dos_atoul(pszMaxBandWidth, pulBandWidth);
    if(0 > lRet)
    {
        dos_printf("%s", "dos_atoul failed!");
        DOS_ASSERT(0);
        return -1;
    }

    return 0;
}


#endif //end  #if (INCLUDE_XML_CONFIG)

#endif //end  #if INCLUDE_BH_SERVER

#ifdef __cplusplus
}
#endif /* __cplusplus */


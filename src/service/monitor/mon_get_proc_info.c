#ifdef __cplusplus
extern "C" {
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR
#if INCLUDE_PROC_MONITOR

#include <dirent.h>
#include "mon_get_proc_info.h"
#include "mon_string.h"


extern S8 g_szMonProcessInfo[MAX_PROC_CNT * MAX_BUFF_LENGTH];
extern MON_PROC_STATUS_S * g_pastProc[MAX_PROC_CNT];
extern S32 g_lPidCnt; //实际运行的进程个数

static BOOL  mon_is_pid_valid(S32 lPid);
static S32   mon_get_cpu_mem_time_info(S32 lPid, MON_PROC_STATUS_S * pstProc);
static S32   mon_get_openfile_count(S32 lPid);
static S32   mon_get_threads_count(S32 lPid);
static S32   mon_get_proc_pid_list();
static S32   mon_kill_process(S32 lPid);
static S32   mon_check_all_process();


S32  mon_proc_malloc()
{
   S32 lRows = 0;
   MON_PROC_STATUS_S * pstProc;
   
   pstProc = (MON_PROC_STATUS_S *)dos_dmem_alloc(MAX_PROC_CNT * sizeof(MON_PROC_STATUS_S));
   if(!pstProc)
   {
      logr_cirt("%s:Line %d:mon_proc_malloc|alloc memory failure, pstProc is %p!"
                , dos_get_filename(__FILE__), __LINE__, pstProc);
      return DOS_FAIL;
   }
   memset(pstProc, 0, MAX_PROC_CNT * sizeof(MON_PROC_STATUS_S));
   
   for(lRows = 0; lRows < MAX_PROC_CNT; lRows++)
   {
      g_pastProc[lRows] = &(pstProc[lRows]);
   }
   
   return DOS_SUCC;
}

S32 mon_proc_free()
{
   S32 lRows = 0;
   
   MON_PROC_STATUS_S * pstProc = g_pastProc[0];
   if(!pstProc)
   {
      logr_cirt("%s:Line %d:mon_proc_free|free memory failure,pstProc is %p!"
                , dos_get_filename(__FILE__), __LINE__ , pstProc);
      return DOS_FAIL;
   }
   
   dos_dmem_free(pstProc);
   for(lRows = 0; lRows < MAX_PROC_CNT; lRows++)
   {
      g_pastProc[lRows] = NULL;
   }
   
   return DOS_SUCC;
}

static BOOL mon_is_pid_valid(S32 lPid)
{
   if(lPid > MAX_PID_VALUE || lPid <= MIN_PID_VALUE)
   {
      logr_emerg("%s:Line %d:mon_is_pid_valid|pid %d is invalid!",
                 dos_get_filename(__FILE__), __LINE__, lPid);
      return DOS_FALSE;
   }
   return DOS_TRUE;
}


static S32  mon_get_cpu_mem_time_info(S32 lPid, MON_PROC_STATUS_S * pstProc)
{//未完成
   S8  szPsCmd[MAX_CMD_LENGTH] = {0};
   S8  szProcFile[] = "procinfo"; 
   FILE * fp;
   S8* pszAnalyseRslt[12] = {0};
   U32 ulRet = 0;

   if(DOS_FALSE == mon_is_pid_valid(lPid))
   {
      logr_error("%s:Line %d:mon_get_cpu_mem_time_info|get cpu & mem & time information failure,pid %d is invalid!"     
                    , dos_get_filename(__FILE__), __LINE__, lPid);
      return DOS_FAIL;
   }
   
   if (!pstProc)
   {
      logr_cirt("%s:line %d:mon_get_cpu_mem_time_info|get cpu & mem & time information|failure,pstProc is %p!"
                , dos_get_filename(__FILE__), __LINE__, pstProc);
      return DOS_FAIL;
   }

   dos_snprintf(szPsCmd, MAX_CMD_LENGTH, "ps aux | grep %d > %s", lPid, szProcFile);
   szPsCmd[dos_strlen(szPsCmd)] = '\0';
   system(szPsCmd);

   fp = fopen(szProcFile, "r");
   if (!fp)
   {
      logr_cirt("%s:line %d:mon_get_cpu_mem_time_info|get cpu & mem & time information|failure,file \'%s\' fp is %p!"
                , dos_get_filename(__FILE__), __LINE__, szProcFile, fp);
      return DOS_FAIL;
   }

   fseek(fp, 0, SEEK_SET);
   while (!feof(fp))
   {  
      S8 szLine[MAX_BUFF_LENGTH] = {0};
      S32 lData = 0;
      S32 lRet  = 0;
      
      if (NULL != (fgets(szLine, MAX_BUFF_LENGTH, fp)))
      {
          ulRet = mon_analyse_by_reg_expr(szLine, " \t\n", pszAnalyseRslt, sizeof(pszAnalyseRslt)/sizeof(pszAnalyseRslt[0]));
          if(DOS_SUCC != ulRet)
          {
             logr_error("%s:Line %d:mon_get_cpu_mem_time_info|analyse sentence failure!"
                        , dos_get_filename(__FILE__), __LINE__);
             goto failure;
          }
          
          lRet = dos_atol(pszAnalyseRslt[1], &lData);
          if(0 != lRet)
          {
             logr_error("%s:Line %d:mon_get_cpu_mem_time_info|dos_atol failure!"
                        , dos_get_filename(__FILE__), __LINE__);
             goto failure;
          }
          if(lPid != lData)
          {
             continue;
          }
          pstProc->fCPURate = atof(pszAnalyseRslt[2]);
          pstProc->fMemoryRate = atof(pszAnalyseRslt[3]);
          dos_strcpy(pstProc->szProcCPUTime, pszAnalyseRslt[8]);
          pstProc->szProcCPUTime[dos_strlen(pszAnalyseRslt[8])] = '\0';
          break;
      }
   }
   goto success;

success:
   fclose(fp);
   fp = NULL;
   unlink(szProcFile);  
   return DOS_SUCC;
failure:
   fclose(fp);
   fp = NULL;
   unlink(szProcFile);  
   return DOS_FAIL;
}

static S32  mon_get_openfile_count(S32 lPid)
{
   S8  szLsofCmd[MAX_CMD_LENGTH] = {0};
   S8  szLine[MAX_BUFF_LENGTH] = {0};
   S8  szFileName[] = "filecount";
   S32 lCount = 0;
   FILE * fp;

   if(DOS_FALSE == mon_is_pid_valid(lPid))
   {
      logr_error("%s:Line %d:mon_get_openfile_count|get open file count failure,pid %d is invalid!"
                    , dos_get_filename(__FILE__), __LINE__, lPid);
      return -1;
   }

   dos_snprintf(szLsofCmd, MAX_CMD_LENGTH, "lsof -p %d | wc -l > %s", lPid, szFileName);
   system(szLsofCmd);

   fp = fopen(szFileName, "r");
   fseek(fp, 0, SEEK_SET);
   if (!fp)
   {
      logr_cirt("%s:line %d:mon_get_openfile_count|file \'%s\' open failure,fp is %p!"
                , dos_get_filename(__FILE__), __LINE__, szFileName, fp);
      return -1;
   }

   if (NULL != (fgets(szLine, MAX_BUFF_LENGTH, fp)))
   {
      S32 lRet = 0;
      lRet = dos_atol(szLine, &lCount);
      if(0 != lRet)
      {
         logr_error("%s:Line %d:mon_get_openfile_count|dos_atol failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
         goto failure;
      }
   }
   goto success;

success:
   fclose(fp);
   fp = NULL;
   unlink(szFileName);
   return lCount;
failure:
    fclose(fp);
    fp = NULL;
    unlink(szFileName);
    return -1;
}

static S32   mon_get_db_conn_count(S32 lPid)
{ //未实现
   if(DOS_FALSE == mon_is_pid_valid(lPid))
   {
      logr_error("%s:Line %d:mon_get_db_conn_count|get database connections count failure,process pid %d is invalid!", 
                    dos_get_filename(__FILE__), __LINE__, lPid);
      return -1;
   }
   
   return DOS_SUCC;
}

static S32   mon_get_threads_count(S32 lPid)
{
   S8  szPidFile[MAX_CMD_LENGTH] = {0};
   S8  szLine[MAX_BUFF_LENGTH] = {0};
   S32 lThreadsCount = 0;
   FILE * fp;
   S8* pszAnalyseRslt[2] = {0};

   if(DOS_FALSE == mon_is_pid_valid(lPid))
   {
      logr_error("%s:Line %d:mon_get_threads_count|get threads count failure,pid %d is invalid!"
                    , dos_get_filename(__FILE__), __LINE__, lPid);
      return -1;
   }
   
   dos_snprintf(szPidFile, MAX_CMD_LENGTH, "/proc/%d/status", lPid);

   fp = fopen(szPidFile, "r");
   if (!fp)
   {
      logr_cirt("%s:line %d:mon_get_threads_count|file \'%s\' open failure,fp is %p!"
                , dos_get_filename(__FILE__), __LINE__, szPidFile, fp);
      return -1;
   }

   fseek(fp, 0, SEEK_SET);
   while (!feof(fp))
   {
      memset(szLine, 0, MAX_BUFF_LENGTH * sizeof(S8));
      if (NULL != (fgets(szLine, MAX_BUFF_LENGTH, fp)))
      {
         if (0 == (dos_strncmp(szLine, "Threads", dos_strlen("Threads"))))
         {
            S32 lData = 0;
            U32 ulRet = 0;
            S32 lRet = 0;
            ulRet = mon_analyse_by_reg_expr(szLine, " \t\n", pszAnalyseRslt, sizeof(pszAnalyseRslt)/sizeof(pszAnalyseRslt[0]));
            if(DOS_SUCC != ulRet)
            {
                logr_error("%s:Line %d:mon_get_threads_count|analyse sentence failure"
                            , dos_get_filename(__FILE__), __LINE__);
                goto failure;
            }
            
            lRet = dos_atol(pszAnalyseRslt[1], &lData);
            if(0 != lRet)
            {
               logr_error("%s:Line %d:mon_get_threads_count|dos_atol failure"
                            , dos_get_filename(__FILE__), __LINE__);
            }
            lThreadsCount = lData;
            goto success;
         }
      }
   }
failure:   
   fclose(fp);
   fp = NULL;
   return -1;
success:
   fclose(fp);
   fp = NULL;
   return lThreadsCount;
}


static S32 mon_get_proc_pid_list()
{
   DIR * pstDir;
   struct dirent * pstEntry;
   S8 szCommPidDir[] = "/ttcom/var/run/pid/";
   S8 szDBPidDir[] = "/var/dipcc/";
   g_lPidCnt = 0;
   FILE * fp = NULL;
   
   pstDir = opendir(szCommPidDir);
   if (!pstDir)
   {
      logr_cirt("%s:Line %d:mon_get_proc_pid_list|dir \'%s\' access failed,psrDir is %p!"
                , dos_get_filename(__FILE__), __LINE__, szCommPidDir, pstDir);
      return DOS_FAIL;
   }

   while (NULL != (pstEntry = readdir(pstDir)))
   {
      if (DT_REG == pstEntry->d_type && DOS_TRUE == mon_is_suffix_true(pstEntry->d_name, "pid"))//如果是普通文件
      {
         S8     szProcAllName[64] = {0};
         S8 *   pTemp;
         S8     szLine[8] = {0};
         S8     szAbsFilePath[64] = {0};

         dos_strcat(szAbsFilePath, szCommPidDir);
         dos_strcat(szAbsFilePath, pstEntry->d_name);
         
         fp = fopen(szAbsFilePath, "r");
      
         if (!fp)
         {
            logr_cirt("%s:Line %d:mon_get_proc_pid_list|file \'%s\' access failed,fp is %p!"
                        , dos_get_filename(__FILE__), __LINE__, szAbsFilePath, fp);
            closedir(pstDir);
            return DOS_FAIL;
         }

         fseek(fp, 0, SEEK_SET);
         if (NULL != fgets(szLine, sizeof(szLine), fp))
         {
            S32 lRet = 0;
            if(!g_pastProc[g_lPidCnt])
            {
               logr_cirt("%s:Line %d:mon_get_proc_pid_list|get proc pid list failure,g_pastProc[%d] is %p!"
                            , dos_get_filename(__FILE__),  __LINE__, g_lPidCnt, g_pastProc[g_lPidCnt]);
               goto failure;
            }
         
            lRet = dos_atol(szLine, &(g_pastProc[g_lPidCnt]->lProcId));
            if(0 != lRet)
            {
               logr_error("%s:Line %d:mon_get_proc_pid_list|dos_atol failure,lRet is %d!"
                            , dos_get_filename(__FILE__), __LINE__, lRet);
               goto failure;
            }

            if(mon_is_proc_dead(g_pastProc[g_lPidCnt]->lProcId))
            {//过滤掉不存在的进程
               logr_error("%s:Line %d:mon_get_proc_pid_list|the pid %d is invalid or not exist!"
                            , dos_get_filename(__FILE__), __LINE__, g_pastProc[g_lPidCnt]->lProcId);
               fclose(fp);
               fp = NULL;
               unlink(szAbsFilePath);
               continue;
            }
            
            pTemp = mon_get_proc_name_by_id(g_pastProc[g_lPidCnt]->lProcId, szProcAllName);
            if(!pTemp)
            {
               logr_error("%s:Line %d:mon_get_proc_pid_list|get proc name by pid failure, pTemp is %p!"
                            , dos_get_filename(__FILE__), __LINE__, pTemp);
               goto failure;
            }
            
            dos_strcpy(g_pastProc[g_lPidCnt]->szProcName, pTemp);
            (g_pastProc[g_lPidCnt]->szProcName)[dos_strlen(pTemp)] = '\0';
            ++g_lPidCnt;
         }
         else
         {
            fclose(fp);
            unlink(szAbsFilePath);
            fp = NULL;
         }
         fclose(fp);
         fp = NULL;
      } 
   }
   closedir(pstDir);

   pstDir = opendir(szDBPidDir);
   if (!pstDir)
   {
      logr_cirt("%s:Line %d:mon_get_proc_pid_list|dir \'%s\' access failed,pstDir is %p!"
                , dos_get_filename(__FILE__), __LINE__, szDBPidDir, pstDir);
      return DOS_FAIL;
   }

   while (NULL != (pstEntry = readdir(pstDir)))
   {
      if (DT_REG == pstEntry->d_type && DOS_TRUE == mon_is_suffix_true(pstEntry->d_name, "pid"))
      {
         S8  szLine[8] = {0};
         S8  szProcAllName[64] = {0};
         S8 *pTemp;
         S8  szAbsFilePath[64] = {0};
         
         dos_strcat(szAbsFilePath, szDBPidDir);
         dos_strcat(szAbsFilePath, pstEntry->d_name);
   
         fp = fopen(szAbsFilePath, "r");      
         if (!fp)
         {
            logr_cirt("%s:Line %d:mon_get_proc_pid_list|file \'%s\' access failure,fp is %p!"
                        , dos_get_filename(__FILE__), __LINE__, szAbsFilePath, fp);
            closedir(pstDir);
            return DOS_FAIL;
         }

         fseek(fp, 0, SEEK_SET);
         if (NULL != (fgets(szLine, sizeof(szLine), fp)))
         {
            S32 lRet = 0;
            if(!g_pastProc[g_lPidCnt])
            {
                logr_cirt("%s:Line %d:mon_get_proc_pid_list|get proc pid list failure,g_pastProc[%d] is %p!"
                            , dos_get_filename(__FILE__), __LINE__ ,g_lPidCnt, g_pastProc[g_lPidCnt]);
                goto failure;
            }
            
            lRet = dos_atol(szLine, &(g_pastProc[g_lPidCnt]->lProcId));
            if(0 != lRet)
            {
               logr_error("%s:Line %d:mon_get_proc_pid_list|dos_atol afilure, lRet is %d!"
                            , dos_get_filename(__FILE__), __LINE__, lRet);
               goto failure;
            }

            if(mon_is_proc_dead(g_pastProc[g_lPidCnt]->lProcId))
            {//过滤掉不存在的进程
               logr_error("%s:Line %d:mon_get_proc_pid_list|pid %d is invalid or not exist!"
                            , dos_get_filename(__FILE__), __LINE__
                            , g_pastProc[g_lPidCnt]->lProcId);
               fclose(fp);
               fp = NULL;
               unlink(szAbsFilePath);
               continue;
            }
            
            pTemp = mon_get_proc_name_by_id(g_pastProc[g_lPidCnt]->lProcId, szProcAllName);
            if(!pTemp)
            {
               logr_error("%s:Line %d:mon_get_proc_pid_list|get proc name by id failure,pTemp is %p!"
                            , dos_get_filename(__FILE__), __LINE__, pTemp);
               goto failure;
            }
            
            dos_strcpy(g_pastProc[g_lPidCnt]->szProcName, pTemp);
            (g_pastProc[g_lPidCnt]->szProcName)[dos_strlen(pTemp)] = '\0';
            ++g_lPidCnt;
         }
         else
         {
            fclose(fp);
            unlink(szAbsFilePath);
            fp = NULL;
         }
         fclose(fp);
         fp = NULL;
      } 
   }

   closedir(pstDir);
   return DOS_SUCC;

failure:
   closedir(pstDir);
   fclose(fp);
   fp = NULL;
   return DOS_FAIL;
}


S32 mon_get_process_data()
{
   S32 lRows = 0;
   S32 lRet = 0;

   lRet = mon_get_proc_pid_list();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_get_process_data|get proc pid list failure,lRet == %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      return DOS_FAIL;
   }
   
   for (lRows = 0; lRows < g_lPidCnt; lRows++)
   {
      if(!g_pastProc[lRows])
      {
          logr_cirt("%s:Line %d:mon_get_process_data|get proc data failure,g_pastProc[%d] is %p!"
                    , dos_get_filename(__FILE__), __LINE__, lRows, g_pastProc[lRows]);
         return DOS_FAIL;
      }
      
      lRet = mon_get_cpu_mem_time_info(g_pastProc[lRows]->lProcId, g_pastProc[lRows]);
      if(DOS_SUCC != lRet)
      {
         logr_error("%s:Line %d:mon_get_process_data|get mem & cpu & time failure,lRet == %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
         return DOS_FAIL;
      }

      lRet = mon_get_openfile_count(g_pastProc[lRows]->lProcId);
      if(-1 == lRet)
      {
         logr_error("%s:Line %d:mon_get_process_data|get open file count failure,lRet == %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
         return DOS_FAIL;
      }
      g_pastProc[lRows]->lOpenFileCnt = lRet;
      
      lRet = mon_get_db_conn_count(g_pastProc[lRows]->lProcId);
      if(DOS_FAIL == lRet)
      {
         logr_error("%s:Line %d:mon_get_process_data|get database connection failure,lRet == %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
         return DOS_FAIL;
      }
      g_pastProc[lRows]->lDBConnCnt = lRet;

      lRet = mon_get_threads_count(g_pastProc[lRows]->lProcId);
      if(-1 == lRet)
      {
         logr_error("%s:Line %d:mon_get_process_data|get threads count failure,lRet == %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
         return DOS_FAIL;
      }
      g_pastProc[lRows]->lThreadsCnt = lRet;
   }

   return DOS_SUCC;
}

 
static S32  mon_kill_process(S32 lPid)
{
   S8 szKillCmd[MAX_CMD_LENGTH] = {0};

   dos_snprintf(szKillCmd, MAX_CMD_LENGTH, "kill -9 %d", lPid);
   system(szKillCmd);

   if (DOS_TRUE == mon_is_proc_dead(lPid))
   {
      logr_info("%s:Line %d:mon_kill_process|kill process success,pid %d is killed!"
                , dos_get_filename(__FILE__), __LINE__, lPid);
      return DOS_SUCC;
   }

   return DOS_FAIL;
}

static S32  mon_check_all_process()
{
   S32 lRet = 0;
   U32 ulRows = 0;
   S8  szProcName[32] = {0};
   S8  szProcVersion[16] = {0};
   S8  szProcCmd[1024] = {0};
   S32 lCfgProcCnt = 0;
   
   //获取配置的进程数量
   lCfgProcCnt = config_hb_get_process_cfg_cnt();
   if(0 > lCfgProcCnt)
   {
      logr_error("%s:Line %d:mon_check_all_process|get config process count failure,lCfgProcCnt is %d!"
                   , dos_get_filename(__FILE__), __LINE__, lCfgProcCnt);
      config_hb_deinit();
      return DOS_FAIL;
   }

   if(lCfgProcCnt <= g_lPidCnt)
   {
      logr_info("%s:Line %d:mon_check_all_process|no process lost!"
                 , dos_get_filename(__FILE__), __LINE__);
      return DOS_SUCC;
   }
      
   for (ulRows = 0; ulRows < lCfgProcCnt; ulRows++)
   {
      U32 ulCols = 0;
      S32 bHasStarted = DOS_FALSE;//默认未启动
         
      lRet = config_hb_get_process_list(ulRows, szProcName, sizeof(szProcName)
                 , szProcVersion, sizeof(szProcVersion));
      if(lRet < 0)
      {
         logr_error("%s:Line %d:mon_check_all_process|get process list failure!"
                     , dos_get_filename(__FILE__), __LINE__);
         config_hb_deinit();
         return DOS_FAIL;
      }

      lRet = config_hb_get_start_cmd(ulRows, szProcCmd, sizeof(szProcCmd));
      if(0 > lRet)
      {
         logr_error("%s:Line %d:mon_check_all_process|get start command failure!"
                     , dos_get_filename(__FILE__), __LINE__);
         config_hb_deinit();
         return DOS_FAIL;
      }

      for(ulCols = 0; ulCols < g_lPidCnt; ulCols++)
      {
         S8 * pszPtr = mon_str_get_name(g_pastProc[ulCols]->szProcName);
            
         if(0 == dos_strcmp(szProcName, "monitord"))
         {//当前进程是运行状态，不用查找
            bHasStarted = DOS_TRUE;
            break;
         }
         if(0 == dos_strcmp("monitord", pszPtr))
         {// 当前进程不会是monitord，监控到monitord直接跳过
            continue;
         }
         if(0 == dos_strcmp(pszPtr, szProcName))
         {//找到，则直接
            bHasStarted = DOS_TRUE;
            break;
         }
      }

      if(DOS_FALSE == bHasStarted)
      {
         lRet = system(szProcCmd);
         if(0 > lRet)
         {
            logr_error("%s:Line %d:mon_check_all_process|start process \'%s\' failure!"
                         , dos_get_filename(__FILE__), __LINE__, szProcName);
            return DOS_FAIL;
         }
      }
   }
   return DOS_SUCC;
}

S32 mon_kill_all_monitor_process()
{
    S32 lRows = 0;
    S32 lRet = 0;
    S32 lPid = 0;

    lPid = getpid();
    for(lRows = 0; lRows < g_lPidCnt; lRows++)
    {
       if(lPid == g_pastProc[lRows]->lProcId)
       {
          continue;
       }
       lRet = mon_kill_process(g_pastProc[lRows]->lProcId);
       if(DOS_SUCC != lRet)
       {
          logr_error("%s:Line %d:mon_kill_all_monitor_process|kill all monitor process failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
          return DOS_FAIL;
       }
    }

    return DOS_SUCC;
}

S32 mon_restart_computer()
{
    system("/sbin/reboot");
    return DOS_SUCC;
}

BOOL mon_is_proc_dead(S32 lPid)
{
   S8 szPsCmd[MAX_CMD_LENGTH] = {0};
   S8 szPsFile[] = "procfile";
   S8 szLine[MAX_BUFF_LENGTH] = {0};
   FILE * fp;

   if(DOS_FALSE == mon_is_pid_valid(lPid))
   {
       logr_error("%s:Line %d:mon_is_proc_dead|pid %d is invalid or not exist,lPid = %d!"
                    , dos_get_filename(__FILE__), __LINE__, lPid);
       return DOS_TRUE;
   }

   dos_snprintf(szPsCmd, MAX_CMD_LENGTH, "ps aux | grep %d > %s",
     lPid, szPsFile);
   system(szPsCmd);

   fp = fopen(szPsFile, "r");
   if (!fp)
   {
      logr_cirt("%s:Line %d:mon_is_proc_dead|file \'%s\' open failed,fp is %p!"
                , dos_get_filename(__FILE__), __LINE__, szPsFile, fp);
      return DOS_FAIL;
   }

   fseek(fp, 0, SEEK_SET);
   while (!feof(fp))
   {
      if ((fgets(szLine, MAX_BUFF_LENGTH, fp)) != NULL)
      {
         if (lPid == mon_first_int_from_str(szLine))
         {
            fclose(fp);
            fp = NULL;
            unlink(szPsFile);
            logr_warning("%s:Line %d:mon_is_proc_dead|pid %d is running"
                         , dos_get_filename(__FILE__), __LINE__, lPid);
            return DOS_FALSE;
         }
      }
   }

   fclose(fp);
   fp = NULL;
   unlink(szPsFile);
   logr_info("%s:Line %d:mon_is_proc_dead|pid %d is dead!"
                , dos_get_filename(__FILE__), __LINE__
                , lPid);
   return DOS_TRUE;
}


S32  mon_get_proc_total_cpu_rate()
{
   F64 fTotalCPURate = 0.0;
   S32 lRows = 0;

   for (lRows = 0; lRows < g_lPidCnt; lRows++)
   {
      if(!g_pastProc[lRows])
      {
         logr_cirt("%s:Line %d:mon_get_all_proc_total_cpu_rate|get all proc total CPU rate failure,g_pastProc[%d] is %p!"
                    , dos_get_filename(__FILE__), __LINE__, lRows, g_pastProc[lRows]);
         return -1;
      }
      
      if(DOS_FALSE == mon_is_pid_valid(g_pastProc[lRows]->lProcId))
      {
         logr_error("%s:Line %d:mon_get_all_proc_total_cpu_rate|proc pid %d is not exist or invalid!"
                    , dos_get_filename(__FILE__), __LINE__, g_pastProc[lRows]->lProcId);
         continue;
      }
      fTotalCPURate += g_pastProc[lRows]->fCPURate;
   }

   return (S32)(fTotalCPURate + 0.5);
}


S32  mon_get_proc_total_mem_rate()
{
   F64 fTotalMemRate = 0.0;
   S32 lRows = 0;

   for (lRows = 0; lRows < g_lPidCnt; lRows++)
   {
      if(!g_pastProc[lRows])
      {
         logr_error("%s:Line %d:mon_get_all_proc_total_mem_rate|get total proc memory failure,g_pastProc[%d] is %p!"
                    , dos_get_filename(__FILE__), __LINE__, lRows, g_pastProc[lRows]);
         return -1;
      }
      
      if(DOS_FALSE == mon_is_pid_valid(g_pastProc[lRows]->lProcId))
      {
         logr_error("%s:Line %d:mon_get_all_proc_total_mem_rate|proc pid %d is invalid!"
                    , dos_get_filename(__FILE__), __LINE__, g_pastProc[lRows]->lProcId);
         continue;
      }
      fTotalMemRate += g_pastProc[lRows]->fMemoryRate;
   }

   return (S32)(fTotalMemRate + 0.5);
}


S32  mon_get_process_formatted_info()
{
   S32 lRows = 0;
   S8  szTempBuff[MAX_BUFF_LENGTH] = {0};
   S32 lMemRate = 0;
   S32 lCPURate = 0;
   S32 lRet = 0;

   memset(g_szMonProcessInfo, 0, MAX_PROC_CNT * MAX_BUFF_LENGTH);

   lRet = mon_check_all_process();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_get_process_formatted_info|check all process failure, lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      return DOS_FAIL;
   }

   for (lRows = 0; lRows < g_lPidCnt; lRows++)
   {
      S8 szTempInfo[MAX_BUFF_LENGTH] = {0};

      if(!g_pastProc[lRows])
      {
         logr_error("%s:Line %d:mon_get_process_formatted_info|get proc formatted information failure,g_pastProc[%d] is %p!"
                    , dos_get_filename(__FILE__), __LINE__, lRows, g_pastProc[lRows]);
         return DOS_FAIL;
      }

      if(DOS_FALSE == mon_is_pid_valid(g_pastProc[lRows]->lProcId))
      {
         logr_error("%s:Line %d:mon_get_process_formatted_info|proc pid %d is not exist or invalid!"
                    , dos_get_filename(__FILE__), __LINE__, g_pastProc[lRows]->lProcId);
         continue;
      }

      lMemRate = mon_get_proc_total_mem_rate();
      if(-1 == lMemRate)
      {
         logr_error("%s:Line %d:mon_get_process_formatted_info|get all proc total memory rate failure,lMemRate is %d%%!"
                    , dos_get_filename(__FILE__), __LINE__, lMemRate);
         return DOS_FAIL;
      }

      lCPURate = mon_get_proc_total_cpu_rate();
      if(-1 == lCPURate)
      {
         logr_error("%s:Line %d:mon_get_process_formatted_info|get all proc total CPU rate failure,lCPURate is %d%%!"
                    , dos_get_filename(__FILE__), __LINE__, lCPURate);
         return DOS_FAIL;
      }
      
      dos_snprintf(szTempInfo, MAX_BUFF_LENGTH,
          "Process Name:%s\n" \
          "Process ID:%d\n" \
          "Memory Usage Rate:%.1f%%\n" \
          "CPU Usage Rate:%.1f%%\n" \
          "Total CPU Time:%s\n" \
          "Open file count:%d\n" \
          "Database connections count:%d\n" \
          "Threads count:%d\n",
          g_pastProc[lRows]->szProcName,
          g_pastProc[lRows]->lProcId,
          g_pastProc[lRows]->fMemoryRate,
          g_pastProc[lRows]->fCPURate,
          g_pastProc[lRows]->szProcCPUTime,
          g_pastProc[lRows]->lOpenFileCnt,
          g_pastProc[lRows]->lDBConnCnt,
          g_pastProc[lRows]->lThreadsCnt);
       dos_strcat(g_szMonProcessInfo, szTempInfo);
       dos_strcat(g_szMonProcessInfo, "\n");
   }
   
   dos_snprintf(szTempBuff, MAX_BUFF_LENGTH,
          "Total Memory Usage Rate:%d%%\nTotal CPU Usage Rate:%d%%"
          , lMemRate, lCPURate);
   dos_strcat(g_szMonProcessInfo, szTempBuff);
   
   return DOS_SUCC;
}

#endif //end #if INCLUDE_PROC_MONITOR
#endif //end #if INCLUDE_RES_MONITOR
#endif //end #if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include "mon_string.h"

#ifndef MAX_PID_VALUE 
#define MAX_PID_VALUE 65535
#define MIN_PID_VALUE 0
#endif

static S8* pszAnalyseList = NULL;

/**
 * 功能:为字符串分配内存
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
S32  mon_init_str_array()
{
   pszAnalyseList = (S8 *)dos_dmem_alloc(MAX_TOKEN_CNT * MAX_TOKEN_LEN * sizeof(S8));
   if(!pszAnalyseList)
   {
      logr_error("%s:Line %d:mon_init_str_array|initialize string array failure!"
                    , dos_get_filename(__FILE__), __LINE__);
      return DOS_FAIL;
   }
   return DOS_SUCC;
}

/**
 * 功能:释放为字符串分配的内存
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
S32  mon_deinit_str_array()
{
   if(!pszAnalyseList)
   {
      logr_error("%s:Line %d:mon_deinit_str_array|initialize string array failure!"
                    , dos_get_filename(__FILE__), __LINE__);
      return DOS_FAIL;
   }
   dos_dmem_free(pszAnalyseList);
   pszAnalyseList = NULL;
   return DOS_SUCC;
}

/**
 * 功能:判断pszDest是否为pszSrc的子串
 * 参数集：
 *   参数1:S8 * pszSrc  源字符串
 *   参数2:S8 * pszDest 子字符串
 * 返回值：
 *   是则返回DOS_TRUE，否则返回DOS_FALSE
 */
BOOL mon_is_substr(S8 * pszSrc, S8 * pszDest)
{
   S8 * pszStr = pszSrc;

   if(!pszSrc || !pszDest)
   {
      logr_warning("%s:Line %d:mon_is_substr|pszSrc is %p,pszDest is %p!"
                    , dos_get_filename(__FILE__), __LINE__, pszSrc, pszDest);
      return DOS_FALSE;
   }

   /* 如果pszSrc的长度小于pszDest，那么一定不是子串 */
   if (dos_strlen(pszSrc) < dos_strlen(pszDest))
   {
      logr_warning("%s:Line %d:mon_is_substr|dos_strlen(pszSrc) = %d,dos_strlen(pszDest) = %d!"
                    , dos_get_filename(__FILE__), __LINE__, dos_strlen(pszSrc)
                    , dos_strlen(pszDest));
      return DOS_FALSE;
   }

   for (;pszStr <= pszSrc + dos_strlen(pszSrc) - dos_strlen(pszDest); ++pszStr)
   {
      if (*pszStr == *pszDest)
      {
         if (dos_strncmp(pszStr, pszDest, dos_strlen(pszDest)) == 0)
         {
            return DOS_TRUE;
         }
      }
   }
   
   return DOS_FALSE;
}

/**
 * 功能:判断pszSentence是否以pszStr结尾
 * 参数集：
 *   参数1:S8 * pszSentence  源字符串
 *   参数2:S8 * pszStr       结尾字符串
 * 返回值：
 *   是则返回DOS_TRUE，否则返回DOS_FALSE
 */
BOOL mon_is_ended_with(S8 * pszSentence, const S8 * pszStr)
{
   S8 *pszSrc = NULL, *pszTemp = NULL;

   if(!pszSentence || !pszStr)
   {
      logr_warning("%s:Line %d:mon_is_ended_with|pszSentence is %p,pszStr is %p!"
                    , dos_get_filename(__FILE__), __LINE__, pszSentence, pszStr);
      return DOS_FALSE;
   }

   if (dos_strlen(pszSentence) < dos_strlen(pszStr))
   {
      logr_warning("%s:Line %d:mon_is_ended_with|dos_strlen(pszSentence) = %d,dos_strlen(pszStr) = %d!"
                    , dos_strlen(pszStr), dos_strlen(pszStr));
      return DOS_FALSE;
   }
   pszSrc = pszSentence + dos_strlen(pszSentence) - dos_strlen(pszStr);
   pszTemp = (S8 *)pszStr;

   while (*pszTemp != '\0')
   {
      if (*pszSrc != *pszTemp)
      {
         logr_notice("%s:Line %d:mon_is_ended_with|\'%s\' is not ended with \'%s\'!"
                        , dos_get_filename(__FILE__), __LINE__, pszSrc, pszTemp);
         return DOS_FALSE;
      }
      ++pszSrc;
      ++pszTemp;
   }

   return DOS_TRUE;
}

/**
 * 功能:判断pszFile是否以pszSuffix为文件后缀名
 * 参数集：
 *   参数1:S8 * pszFile   源文件名
 *   参数2:S8 * pszSuffix 文件后缀名
 * 返回值：
 *   是则返回DOS_TRUE，否则返回DOS_FALSE
 */
BOOL mon_is_suffix_true(S8 * pszFile, const S8 * pszSuffix)
{
   S8 * pszFileSrc = NULL;
   S8 * pszSuffixSrc = NULL;

   if(!pszFile || !pszFile)
   {
      logr_warning("%s:Line %d:mon_is_suffix_true|pszFile is %p, pszSuffix is %p!"
                    , dos_get_filename(__FILE__), __LINE__, pszFile, pszSuffix);
      return DOS_FALSE;
   }

   pszFileSrc = pszFile + dos_strlen(pszFile) -1;
   pszSuffixSrc = (S8 *)pszSuffix + dos_strlen(pszSuffix) - 1;
   
   for (; pszSuffixSrc >= pszSuffix; pszSuffixSrc--, pszFileSrc--)
   {
      if (*pszFileSrc != *pszSuffixSrc)
      {
         logr_notice("%s:Line %d:mon_is_suffix_true|the suffix of \'%s\' is not \'%s\'!"
                        ,dos_get_filename(__FILE__), __LINE__, pszFile, pszSuffix);
         return DOS_FALSE;
      }
   }
   
   return DOS_TRUE;
}

/**
 * 功能:获取字符串中的一个整数
 * 参数集：
 *   参数1:S8 * pszStr  含有数字字符串的字符串
 * 返回值：
 *   成功则返回字符串中的的一个数字，失败则返回DOS_FAIL
 */
S32 mon_first_int_from_str(S8 * pszStr)
{
   S32  lData;
   S8   szTail[1024] = {0}; 
   S8 * pszSrc = pszStr;
   S8 * pszTemp = NULL;
   S32  lRet = 0;

   if(!pszStr)
   {
      logr_warning("%s:Line %d:mon_first_int_from_str|pszStr is %p!"
                    , dos_get_filename(__FILE__), __LINE__, pszStr);
      return -1;
   }
   
   while (!(*pszSrc >= '0' && *pszSrc <= '9'))
   {
      ++pszSrc;
   }
   pszTemp = pszSrc;
   while (*pszTemp >= '0' && *pszTemp <= '9')
   {
      pszTemp++;
   }
   dos_strncpy(szTail, pszSrc, pszTemp - pszSrc);
   szTail[pszTemp - pszSrc] = '\0';

   lRet = dos_atol(szTail, &lData);
   if(0 != lRet)
   {
      logr_error("%s:Line %d:mon_first_int_from_str|dos_atol failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      return -1;
   }
   
   return lData;
}

/**
 * 功能:将字符串pszStr按照pszRegExpr规则去分割，并存储到pszRsltList中l
 * 参数集：
 *   参数1:S8 * pszStr       含有数字字符串的字符串
 *   参数2:S8* pszRegExpr    分界字符串
 *   参数3:S8* pszRsltList[] 用于存放字符串的首地址
 *   参数4:U32 ulLen         数组最大长度
 * 返回值：
 *   成功则返回DOS_SUCC，失败则返回DOS_FAIL
 */
U32  mon_analyse_by_reg_expr(S8* pszStr, S8* pszRegExpr, S8* pszRsltList[], U32 ulLen)
{
   U32 ulRows = 0;
   S8* pszToken = NULL;

   if(!pszStr)
   {
      logr_error("%s:Line %d:mon_analyse_by_reg_expr|pszStr is %p!"
                    , dos_get_filename(__FILE__), __LINE__);
      return DOS_FAIL;
   }

   if(!pszRegExpr)
   {
      logr_error("%s:Line %d:mon_analyse_by_reg_expr|pszRegExpr is %p!"
                    , dos_get_filename(__FILE__), __LINE__);
      return DOS_FAIL;
   }

   /* 每次使用前初始化为0 */
   memset(pszAnalyseList, 0, MAX_TOKEN_CNT * MAX_TOKEN_LEN * sizeof(char));
   for(ulRows = 0; ulRows < ulLen; ulRows++)
   {
      /*把字符串首地址分别置于分配内存的相应位置*/
      pszRsltList[ulRows] = pszAnalyseList + ulRows * MAX_TOKEN_LEN;
   }

   for(ulRows = 0; ulRows < ulLen; ulRows++)
   {
      S8 *pszBuff = NULL;
      if(0 == ulRows)
      {
         pszBuff = pszStr;
      }

      pszToken = strtok(pszBuff, pszRegExpr);
      if(!pszToken)
      {
         break;
      }
      dos_strncpy(pszRsltList[ulRows], pszToken, dos_strlen(pszToken));
      pszRsltList[ulRows][dos_strlen(pszToken)] = '\0';
   }
   return DOS_SUCC;
}

/**
 * 功能:生成告警id
 * 参数集：
 *   参数1:U32 ulResType     资源种类
 *   参数2:U32 ulNo          资源编号
 *   参数3:U32 ulErrType     错误类型
 * 返回值：
 *   成功则返回告警id，失败则返回(U32)0xff
 */
U32 mon_generate_warning_id(U32 ulResType, U32 ulNo, U32 ulErrType)
{
   if(ulResType >= (U32)0xff || ulNo >= (U32)0xff
      || ulErrType >= (U32)0xff)
   {
      logr_notice("%s:Line %d: mon_generate_warning_id|ulResType is %s%x,ulNo is %s%x, ulErrType is %s%x"
                    , dos_get_filename(__FILE__), __LINE__, "0x", ulResType, "0x", ulNo, "0x", ulErrType);
      return (U32)0xff;
   }
   /* 第1个8位存储资源类型，第2个8位存储资源编号，第3个8位存储错误编号 */
   return (ulResType << 24) | (ulNo << 16 ) | (ulErrType & 0xffffffff);
}

/**
 * 功能:根据进程id获得进程名
 * 参数集：
 *   参数1:S32 lPid  进程id
 *   参数2:S8 * pszPidName   进程名
 * 返回值：
 *   成功则返回进程名，失败则返回NULL
 */
S8 * mon_get_proc_name_by_id(S32 lPid, S8 * pszPidName)
{
   S8   szPsCmd[64] = {0};
   S8   szMonFile[] = "monstring";
   S8   szLine[1024] = {0};
   S8*  pTokenPtr = NULL;
   FILE * fp;

   if(lPid > MAX_PID_VALUE || lPid <= MIN_PID_VALUE)
   {
      logr_emerg("%s:Line %d:mon_get_proc_name_by_id|pid %d is invalid!"
                    , dos_get_filename(__FILE__), __LINE__, lPid);
      return NULL;
   }

   if(!pszPidName)
   {
      logr_warning("%s:Line %d:mon_get_proc_name_by_id|pszPidName is %p!"
                    , dos_get_filename(__FILE__), __LINE__, pszPidName);
      return NULL;
   }

   dos_snprintf(szPsCmd, sizeof(szPsCmd), "ps -ef | grep %d > %s"
                ,lPid, szMonFile);
   system(szPsCmd);

   fp = fopen(szMonFile, "r");
   if (!fp)
   {
      logr_cirt("%s:Line %d:mon_get_proc_name_by_id|file \'%s\' open failed,fp is %p!"
                , dos_get_filename(__FILE__), __LINE__, szMonFile, fp);
      return NULL;
   }

   fseek(fp, 0, SEEK_SET);

   while(!feof(fp))
   {
      S32 lCols = 0;
      if(NULL != fgets(szLine, sizeof(szLine), fp))
      {
         if(lPid != mon_first_int_from_str(szLine))
         {
            continue;
         }

         pTokenPtr = strtok(szLine, " \t\n");
         while(pTokenPtr)
         {
            pTokenPtr = strtok(NULL, " \t\n");
            if(5 == lCols)
            {
               break;
            }
            ++lCols;
         }

         pTokenPtr = strtok(NULL, "\t");
         *(pTokenPtr + dos_strlen(pTokenPtr) - 1) = '\0';

         fclose(fp);
         fp = NULL;
         unlink(szMonFile);

         return pTokenPtr;
      }
   }

   fclose(fp);
   fp = NULL;
   unlink(szMonFile);

   return NULL;
}

/**
 * 功能:为字符串去头去尾，只留下最简单的名字
 * 参数集：
 *   参数1:S8 * pszCmd   进程启动命令
 * 返回值：
 *   成功则返回去头去尾之后的简单名称，失败则返回NULL
 */
S8 * mon_str_get_name(S8 * pszCmd)
{  
   S8 * pszPtr = pszCmd;
   if(!pszPtr)
   {
      logr_error("%s:Line %d:mon_str_get_cmd|get cmd failure,pszCmd is %p!"
                    , dos_get_filename(__FILE__), __LINE__, pszCmd);
      return NULL;
   }

   /**
    *  找到第一个空格，前面为命令的绝对路径，后面为命令附带的相关参数
    */
   while(*pszPtr != ' ' && *pszPtr != '\0' && *pszPtr != '\t')
   {
      ++pszPtr;
   }
   *pszPtr = '\0';

   /*找到最后一个'/'字符，'/'和' '之间的部分是命令的最简化名称*/
   while(*(pszPtr - 1) != '/' && pszPtr != pszCmd)
   {
      --pszPtr;
   }

   pszCmd = pszPtr; //得到

   return pszCmd;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif


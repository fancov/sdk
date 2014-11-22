/*
 * 定义文件日类
 * 创建人: Larry
 * 创建时间: 2014-10-22
 * 修改历史:
 */

#include <stdio.h>
#include <string.h>
#include <dos/dos_types.h>
#include <syscfg.h>

#if (UTIL_LOG_ENABLE && UTIL_FILE_LOG_ENABLE)

#include "_log_base.h"


 #define MAX_FILENAME_LENGTH 64

class log_file : public log
{
private:
    S8 szFileName[MAX_FILENAME_LENGTH];
    FILE *fd;
public:
    log_file(S8 *_file = NULL);
    ~log_file();
    S32 log_set_filename(S8 *_pszFile);
    S32 log_init();
    S32 log_init(S32 _lArgc, S8 **_pszArgv);
    S32 log_set_level(U32 ulLevel);
    VOID log_write(S8 *_pszTime, S8 *_pszType, S8 *_pszLevel, S8 *_pszMsg, U32 ulLevel, U32 ulType = 0);
};

#endif


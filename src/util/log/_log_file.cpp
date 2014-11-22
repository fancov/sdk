/*
 * 实现文件日类
 * 创建人: Larry
 * 创建时间: 2014-10-22
 * 修改历史:
 */

#if (UTIL_LOG_ENABLE && UTIL_FILE_LOG_ENABLE)

#include "_log_file.h"

log_file::log_file(S8 *_pszFile)
{
    if (_pszFile != NULL)
    {
        strncpy(szFileName, _pszFile, sizeof(szFileName));
        sz_file_name[sizeof(szFileName) - 1] = '\0';
    }
    else
    {
        sz_file_name[0] = '\0';
    }

    fd = NULL;
}

log_file::~log_file()
{
    if (fd)
    {
        fclose(fd);
    }
}

S32 log_file::log_init()
{
    return 0;
}

S32 log_file::log_init(S32 _lArgc, S8 **_pszArgv)
{
    return 0;
}

S32 log_file::log_set_filename(S8 *_pszFile)
{
    return 0;
}

S32 log_file::log_set_level(U32 ulLevel)
{
    return 0;
}

void log_file::log_write(S8 *_pszTime, S8 *_pszType, S8 *_pszLevel, S8 *_pszMsg, U32 _ulLevel, U32 _ulType)
{

}

#endif


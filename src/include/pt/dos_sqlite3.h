#ifndef __DOS_SQLITE_H__
#define __DOS_SQLITE_H__

#ifdef  __cplusplus
extern "C"{
#endif

#include <sqlite3.h>
#include <dos/dos_types.h>

#define SQLITE_DB_PATH_LEN                   256

typedef enum
{
    PT_SLITE3_FAIL = -1,    /* 创建失败 */
    PT_SLITE3_SUCC = 0,     /* 创建成功 */
    PT_SLITE3_EXISTED,      /* 已存在 */
    PT_SLITE3_BUTT

}PT_CREATE_DB_RESULT;

typedef struct tagMySqlite3
{
   sqlite3 *    pHandle;
   S8           pchDbPath[SQLITE_DB_PATH_LEN];
}DOS_SQLITE_ST;

typedef int (*my_sqlite_callback)(void*,int,char**,char**);


S32 dos_sqlite3_create_db(DOS_SQLITE_ST *);
S32 dos_sqlite3_creat_table(DOS_SQLITE_ST, S8 *);
S32 dos_sqlite3_exec(DOS_SQLITE_ST, S8 *);
S32 dos_sqlite3_record_is_exist(DOS_SQLITE_ST, S8 *);
S32 dos_sqlite3_exec_callback(DOS_SQLITE_ST, S8 *, my_sqlite_callback, void *);
S32 dos_sqlite3_close(DOS_SQLITE_ST);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

#endif

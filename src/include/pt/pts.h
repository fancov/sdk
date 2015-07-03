#ifndef __PTS_H__
#define __PTS_H__

#ifdef  __cplusplus
extern "C"{
#endif

#include <dos/dos_types.h>
#include <pt/dos_sqlite3.h>

#define PTS_SQLITE_DB_PATH         "var/pts.db"    /* sqlite 数据库的路径 */
#define PTS_WEB_SERV_PORT_DEFAULT  80          /* 请求proxy的默认端口 */
#define PTS_WEB_SERV_IP_DEFAULT    "127.0.0.1" /* 请求proxy的默认ip */
#define PTS_TEL_SERV_IP_DEFAULT    "127.0.0.1" /* 请求proxy的默认ip */
#define PTS_TEL_SERV_PORT_DEFAULT  23          /* 请求proxy的默认端口 */
#define PTS_SOCKET_CACHE           10 * 1024 * 1024 /* socket 收发缓存大小 */

extern DOS_SQLITE_ST *g_pstMySqlite;

typedef struct tagPtsSqliteGetValueParam
{
    U32 ulLen;
    S8  *szValue;

}PTS_SQLITE_GET_VALUE_PARAM_ST;

S32 pts_main();
U32 pts_create_stream_id();
S32 pts_create_tcp_socket(U16 usTcpPort);
VOID pts_change_remark(S8 *szUrl, U32 ulConnfd);
S32 pts_get_value_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */


#endif
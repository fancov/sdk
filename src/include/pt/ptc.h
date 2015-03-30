#ifndef __PTC_H__
#define __PTC_H__

#ifdef  __cplusplus
extern "C"{
#endif
#if INCLUDE_PTS
#include <pt/dos_sqlite3.h>
#endif
#include <dos/dos_types.h>

#define PTC_SOCKET_CACHE     1024 * 1024            /* socket 收发缓存大小 */
#define PTC_PTS_DOMAIN_SIZE  64                     /* PTS域名长度 */
#define PTC_SQLITE_DB_PATH   "var/ptc.db"    /* sqlite 数据库的路径 */

S32 ptc_main();
S32 ptc_create_socket_proxy(U8 *alServIp, U16 usServPort);
VOID ptc_get_pts_history();
S32 ptc_get_ifr_name_by_ip(S8 *szIp, S32 lSockfd, S8 *szName, U32 ulNameLen);
S32 ptc_get_mac(S8 *szName, S32 lSockfd, S8 *szMac, U32 ulMacLen);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */


#endif
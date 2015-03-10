#ifndef __PTS_WEB_H__
#define __PTS_WEB_H__

#ifdef  __cplusplus
extern "C"{
#endif

#include <dos/dos_types.h>

#define PTS_PTC_LIST_HTML  "<tr>\
<td><INPUT name=\"PtcCheckBox\"  type=\"checkbox\" value=\"%s\"   ></td>\
<td><label id='id_%s'>%s</label></td>\
<td>%s</td>\
<td><input type='text' id='label_%s' value='%s' onblur=\"make_sure('button_%s')\" ></td>\
<td><label>%s</label></td>\
<td><label>%s</label></td>\
<td><label>%s</label></td>\
<td><label>%s</label></td>\
<td><label>%s</label></td>\
</tr>"


#define PTS_PTC_LIST_CONFIG_HTML "<tr>\
<td><INPUT name=\"PtcCheckBox\"  type=\"checkbox\" value=\"%s\"   ></td>\
<td><label id='id_%s'>%s</label></td>\
<td>%s</td>\
<td><input type='text' id='label_%s' value='%s' onblur=\"make_sure('button_%s')\" ></td>\
<td><label>%s</label></td>\
<td><label>%s</label></td>\
<td><label>%s</label></td>\
<td><label>%s</label></td>\
<td><label>%s:%s</label></td>\
<td><label>%s:%s</label></td>\
<td><label>%s</label></td>\
</tr>"

#define PTS_PTC_LIST_UPGRADES_HTML "<tr>\
<td><INPUT name=\"PtcCheckBox\"  type=\"checkbox\" value=\"%s\"   ></td>\
<td><label id='id_%s'>%s</label></td>\
<td>%s</td>\
<td><input type='text' id='label_%s' value='%s' onblur=\"make_sure('button_%s')\" ></td>\
<td><label>%s</label></td>\
<td><label>%s</label></td>\
<td><label>%s</label></td>\
<td><label>%s</label></td>\
<td><label>%s</label></td>\
<td><label>%s</label></td>\
</tr>"

extern list_t  *m_stClinetCBList;

void *pts_recv_msg_from_web(void *arg);
list_t *pts_clinetCB_insert(list_t *psthead, S32 lSockfd, struct sockaddr_in stClientAddr);
PTS_CLIENT_CB_ST* pts_clinetCB_search_by_sockfd(list_t *pstHead, S32 lSockfd);
PTS_CLIENT_CB_ST* pts_clinetCB_search_by_stream(list_t* pstHead, U32 ulStreamID);
list_t *pts_clinetCB_delete_node(list_t* pstHead, PTS_CLIENT_CB_ST* pstNode);
list_t *pts_clinetCB_delete_by_sockfd(list_t* pstHead, S32 lSockfd);
list_t *pts_clinetCB_delete_by_stream(list_t* pstHead, U32 ulStreamID);
void pts_send_msg2web(PT_NEND_RECV_NODE_ST *pstNeedRevNode);
VOID pts_create_web_serv_socket(U32 ulArraySeq);
VOID pts_web_timeout_callback(U64 arg);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

#endif

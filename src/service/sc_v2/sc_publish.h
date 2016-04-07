#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#ifndef __SC_PUBLISH__
#define __SC_PUBLISH__

enum {
    SC_PUB_TYPE_STATUS   = 0,
    SC_PUB_TYPE_MARKER   = 1,
    SC_PUB_TYPE_SIP_XMl  = 2,
    SC_PUB_TYPE_GATEWAY  = 3,

    SC_PUB_TYPE_CLIENT   = 4,

    SC_PUB_TYPE_BUUT,
};

enum {
    SC_PUB_ERR_UNKNOW                = -1,
    SC_PUB_ERR_SUCC                  = 0,
    SC_PUB_ERR_NOT_SUPPORT           = 1,
    SC_PUB_ERR_DATA_NOT_COMPLETE     = 2,
    SC_PUB_ERR_INVALID_DATA          = 3,
    SC_PUB_ERR_EXEC_FAIL             = 4,

    SC_PUB_ERR_BUTT
};

typedef struct tagSCPubFSData{
    U32   ulID;
    U32   ulAction;
}SC_PUB_FS_DATA_ST;

typedef struct tagSCPubMarkerData{
    S8 szNumber[40];
}SC_PUB_MARKER_DATA_ST;


U32 sc_pub_send_msg(S8 *pszURL, S8 *pszData, U32 ulType, VOID *pstData);

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */



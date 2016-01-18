
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#ifndef __SC_LOG_DIGEST_H__
#define __SC_LOG_DIGEST_H__

#define SC_LOG_DIGEST_LEN               512
#define SC_LOG_DIGEST_FILE_MAX_SIZE     10485760    /* 10M */
#define SC_LOG_DIGEST_PATH              "/var/ipcc/digest/log"

U32 sc_log_digest_init();
U32 sc_log_digest_stop();

#endif /* end of __SC_LOG_DIGEST_H__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */



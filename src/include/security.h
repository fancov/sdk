#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#ifndef __SECURITY_DEF_H__
#define __SECURITY_DEF_H__

#if (INCLUDE_OPENSSL_LIB)

#define SECURITY_SUCC          1
#define SECURITY_FAIL          0


/* 定义最大摘要长度 目前最长的摘要为SHA512 */
#define DIGEST_MAX_LEN         64

/* 定义加解密结果最大长度 */
#define CRYPTION_MAX_LEN       1048 * 16
#define CRYPTION_KEY_MAX_LEN   64

typedef enum tagDigestTypes{
    CRYPTION_BLOWFISH       = 0,
    CRYPTION_DES_EDE_ECB    = 1,
    CRYPTION_DES_ECB        = 2,
    CRYPTION_RC4            = 3,
    CRYPTION_AES128_ECB     = 4,
    CRYPTION_AES128_CFB8    = 5,
    CRYPTION_AES192_CFB128  = 6,
    CRYPTION_RC2_CBC        = 7,
    CRYPTION_RC2_CFB64      = 8,
    CRYPTION_CAST5_ECB      = 9,
    CRYPTION_CAST5_OFB      = 10,

    CRYPTION_BUTT,
}CRYPTION_TYPES_EN;

typedef enum tagCryptionTypes{
    DIGEST_SHA          = 0,
    DIGEST_SHA1         = 1,
    DIGEST_DSS          = 2,
    DIGEST_DSS1         = 3,
    DIGEST_SHA384       = 4,
    DIGEST_MD4          = 5,
    DIGEST_SHA224       = 6,
    DIGEST_MDC2         = 7,
    DIGEST_SHA256       = 8,
    DIGEST_MD5          = 9,
    DIGEST_SHA512       = 10,

    DIGEST_BUTT
}DIGEST_TYPES_EN;

typedef enum tagCryptionAction{
    CRYPTION_ACTION_DECODE      = 0,
    CRYPTION_ACTION_ENCODE      = 1,
    CRYPTION_ACTION_BUTT
}CRYPTION_ACTION_EN;


typedef struct tagDigestContext
{
    U32    ulDigestType;

    U8     *aucSrc0;
    U32    ulSrcLen0;
    U8     *aucSrc1;
    U32    ulSrcLen1;
    U8     *aucSrc2;
    U32    ulSrcLen2;
    U8     *aucSrc3;
    U32    ulSrcLen3;

    U8     aucDst[DIGEST_MAX_LEN];
    U32    ulDstLen;

    S8     *pszErrMsg;
    U32    ulMsgLen;
}DIGEST_CONTEXT_ST;

typedef struct tagCryptionContext
{
    U32    ulCryptionType;

    U8     *aucSrc;
    U32    ulSrcLen;

    U8     aucKey0[CRYPTION_KEY_MAX_LEN];
    U32    ulKeyLen0;
    U8     aucKey1[CRYPTION_KEY_MAX_LEN];
    U32    ulKeyLen1;
    U8     aucKey2[CRYPTION_KEY_MAX_LEN];
    U32    ulKeyLen2;
    U8     aucKey3[CRYPTION_KEY_MAX_LEN];
    U32    ulKeyLen3;

    U8     *paucDst;
    U32    ulDstLen;

    S8     *pszErrMsg;
    U32    ulMsgLen;
}CRYPTION_CONTEXT_ST;


U32 digest_calc(DIGEST_CONTEXT_ST *pstCtx, DIGEST_TYPES_EN enType);
U32 cryption_encode(CRYPTION_CONTEXT_ST *pstCtx, CRYPTION_TYPES_EN enType);
U32 cryption_decode(CRYPTION_CONTEXT_ST *pstCtx, CRYPTION_TYPES_EN enType);

#endif /* end of INCLUDE_OPENSSL_LIB */

#if 1
U32 dos_crc32(U8 *ptr, U32 count);
U16 dos_crc16(U8 *ptr, U32 count);
#endif

#endif /* End of __SECURITY_DEF_H__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */


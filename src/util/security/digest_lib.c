
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <security.h>
#include <openssl/evp.h>

U32 digest_calc(DIGEST_CONTEXT_ST *pstCtx, DIGEST_TYPES_EN enType)
{
    S32        lRetVal = 0;
    EVP_MD_CTX *pstMDCtx;

    if (DOS_ADDR_INVALID(pstCtx))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstCtx->aucSrc0) || !pstCtx->ulSrcLen0)
    {
        if (DOS_ADDR_VALID(pstCtx->pszErrMsg) && 0 != pstCtx->ulMsgLen)
        {
            dos_snprintf(pstCtx->pszErrMsg, pstCtx->ulMsgLen, "ERR: Invalid input paramters, Lenght: %u, Addr: %p!", pstCtx->ulSrcLen0, pstCtx->aucSrc0);
        }

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (enType >= DIGEST_BUTT)
    {
        if (DOS_ADDR_VALID(pstCtx->pszErrMsg) && 0 != pstCtx->ulMsgLen)
        {
            dos_snprintf(pstCtx->pszErrMsg, pstCtx->ulMsgLen, "ERR: Invalid digest type : %d!", enType);
        }

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstMDCtx = EVP_MD_CTX_create();
    if (DOS_ADDR_INVALID(pstMDCtx))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (enType)
    {
        case DIGEST_SHA384:
            lRetVal = EVP_DigestInit_ex(pstMDCtx, EVP_sha384(), NULL);
            break;

        case DIGEST_SHA:
            lRetVal = EVP_DigestInit_ex(pstMDCtx, EVP_sha(), NULL);
            break;

        case DIGEST_SHA1:
            lRetVal = EVP_DigestInit_ex(pstMDCtx, EVP_sha1(), NULL);
            break;

        case DIGEST_DSS:
            lRetVal = EVP_DigestInit_ex(pstMDCtx, EVP_dss(), NULL);
            break;

        case DIGEST_DSS1:
            lRetVal = EVP_DigestInit_ex(pstMDCtx, EVP_dss1(), NULL);
            break;

        case DIGEST_MD4:
            lRetVal = EVP_DigestInit_ex(pstMDCtx, EVP_md4(), NULL);
            break;

        case DIGEST_SHA224:
            lRetVal = EVP_DigestInit_ex(pstMDCtx, EVP_sha224(), NULL);
            break;

        case DIGEST_MDC2:
            lRetVal = EVP_DigestInit_ex(pstMDCtx, EVP_mdc2(), NULL);
            break;

        case DIGEST_SHA256:
            lRetVal = EVP_DigestInit_ex(pstMDCtx, EVP_sha256(), NULL);
            break;

        case DIGEST_MD5:
            lRetVal = EVP_DigestInit_ex(pstMDCtx, EVP_md5(), NULL);
            break;
        case DIGEST_SHA512:
            lRetVal = EVP_DigestInit_ex(pstMDCtx, EVP_sha512(), NULL);
            break;


        default:
            lRetVal = SECURITY_FAIL;
            break;
    }

    if (SECURITY_SUCC != lRetVal)
    {
        if (DOS_ADDR_VALID(pstCtx->pszErrMsg) && 0 != pstCtx->ulMsgLen)
        {
            dos_snprintf(pstCtx->pszErrMsg, pstCtx->ulMsgLen, "ERR: Init the digest engine FAIL!");
        }

        goto calc_fail;
    }

    lRetVal = EVP_DigestUpdate(pstMDCtx, pstCtx->aucSrc0, pstCtx->ulSrcLen0);
    if (SECURITY_SUCC != lRetVal)
    {
        DOS_ASSERT(0);
        goto calc_fail;
    }

    if (DOS_ADDR_VALID(pstCtx->aucSrc1) && pstCtx->ulSrcLen1 > 0)
    {
        lRetVal = EVP_DigestUpdate(pstMDCtx, pstCtx->aucSrc1, pstCtx->ulSrcLen1);
        if (SECURITY_SUCC != lRetVal)
        {
            DOS_ASSERT(0);
            goto calc_fail;
        }
    }

    if (DOS_ADDR_VALID(pstCtx->aucSrc2) && pstCtx->ulSrcLen2 > 0)
    {
        lRetVal = EVP_DigestUpdate(pstMDCtx, pstCtx->aucSrc2, pstCtx->ulSrcLen2);
        if (SECURITY_SUCC != lRetVal)
        {
            DOS_ASSERT(0);
            goto calc_fail;
        }
    }

    if (DOS_ADDR_VALID(pstCtx->aucSrc3) && pstCtx->ulSrcLen3 > 0)
    {
        lRetVal = EVP_DigestUpdate(pstMDCtx, pstCtx->aucSrc3, pstCtx->ulSrcLen3);
        if (SECURITY_SUCC != lRetVal)
        {
            DOS_ASSERT(0);
            goto calc_fail;
        }
    }


    lRetVal = EVP_DigestFinal_ex(pstMDCtx, pstCtx->aucDst, &pstCtx->ulDstLen);
    if (SECURITY_SUCC != lRetVal)
    {
        DOS_ASSERT(0);
        goto calc_fail;
    }

    EVP_MD_CTX_cleanup(pstMDCtx);
    EVP_MD_CTX_destroy(pstMDCtx);
    return DOS_SUCC;

calc_fail:


    if (pstMDCtx)
    {
        EVP_MD_CTX_cleanup(pstMDCtx);
        EVP_MD_CTX_destroy(pstMDCtx);
    }

    return DOS_FAIL;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */



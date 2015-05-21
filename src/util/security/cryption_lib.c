#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <security.h>
#include <openssl/evp.h>

#if (INCLUDE_OPENSSL_LIB)

U32 cryption_encode(CRYPTION_CONTEXT_ST *pstCtx, CRYPTION_TYPES_EN enType)
{
    U8 *aucOutBuff = NULL;
    S32 lOutputLen = 0;
    S32 lAfterLen  = 0;
    S32 lRetVal     = SECURITY_FAIL;
    EVP_CIPHER_CTX stCTX;

    if (DOS_ADDR_INVALID(pstCtx))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstCtx->aucSrc) || !pstCtx->ulSrcLen)
    {
        if (DOS_ADDR_VALID(pstCtx->pszErrMsg) && 0 != pstCtx->ulMsgLen)
        {
            dos_snprintf(pstCtx->pszErrMsg, pstCtx->ulMsgLen, "ERR: Invalid input source data.");
        }

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstCtx->aucKey0) || !pstCtx->ulKeyLen0)
    {
        if (DOS_ADDR_VALID(pstCtx->pszErrMsg) && 0 != pstCtx->ulMsgLen)
        {
            dos_snprintf(pstCtx->pszErrMsg, pstCtx->ulMsgLen, "ERR: Invalid input key.");
        }

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    aucOutBuff= dos_dmem_alloc(CRYPTION_MAX_LEN);
    if (DOS_ADDR_INVALID(aucOutBuff))
    {
        if (DOS_ADDR_VALID(pstCtx->pszErrMsg) && 0 != pstCtx->ulMsgLen)
        {
            dos_snprintf(pstCtx->pszErrMsg, pstCtx->ulMsgLen, "ERR: Alloc memory FAIL!");
        }

        DOS_ASSERT(0);
        return DOS_FAIL;
    }


    lRetVal     = SECURITY_FAIL;
    EVP_CIPHER_CTX_init(&stCTX);
    switch (enType)
    {
        case CRYPTION_BLOWFISH:
            lRetVal = EVP_EncryptInit_ex(&stCTX, EVP_bf_cfb64(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_DES_EDE_ECB:
            lRetVal = EVP_EncryptInit_ex(&stCTX, EVP_des_ede_ecb(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_DES_ECB:
            lRetVal = EVP_EncryptInit_ex(&stCTX, EVP_des_ecb(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_RC4:
            lRetVal = EVP_EncryptInit_ex(&stCTX, EVP_rc4(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_AES128_ECB:
            lRetVal = EVP_EncryptInit_ex(&stCTX, EVP_aes_128_ecb(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_AES128_CFB8:
            lRetVal = EVP_EncryptInit_ex(&stCTX, EVP_aes_128_cfb8(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_AES192_CFB128:
            lRetVal = EVP_EncryptInit_ex(&stCTX, EVP_aes_192_cfb128(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_RC2_CBC:
            lRetVal = EVP_EncryptInit_ex(&stCTX, EVP_rc2_cbc(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_RC2_CFB64:
            lRetVal = EVP_EncryptInit_ex(&stCTX, EVP_rc2_cfb64(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_CAST5_ECB:
            lRetVal = EVP_EncryptInit_ex(&stCTX, EVP_cast5_ecb(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_CAST5_OFB:
            lRetVal = EVP_EncryptInit_ex(&stCTX, EVP_camellia_128_ofb(), NULL, pstCtx->aucKey0, NULL);
            break;

        default:
            goto fail;
    }

    if (lRetVal != SECURITY_SUCC)
    {
        if (DOS_ADDR_VALID(pstCtx->pszErrMsg) && 0 != pstCtx->ulMsgLen)
        {
            dos_snprintf(pstCtx->pszErrMsg, pstCtx->ulMsgLen, "ERR: Init encrypt engine FAIL!");
        }

        DOS_ASSERT(0);
        goto fail;
    }

    lRetVal = EVP_EncryptUpdate(&stCTX, aucOutBuff, &lOutputLen, pstCtx->aucSrc, pstCtx->ulSrcLen);
    if (lRetVal != SECURITY_SUCC)
    {
        if (DOS_ADDR_VALID(pstCtx->pszErrMsg) && 0 != pstCtx->ulMsgLen)
        {
            dos_snprintf(pstCtx->pszErrMsg, pstCtx->ulMsgLen, "ERR: Encrypt FAIL!");
        }

        DOS_ASSERT(0);
        goto fail;
    }

    lRetVal = EVP_EncryptFinal_ex(&stCTX, aucOutBuff + lOutputLen, &lAfterLen);
    if (lRetVal != SECURITY_SUCC)
    {
        if (DOS_ADDR_VALID(pstCtx->pszErrMsg) && 0 != pstCtx->ulMsgLen)
        {
            dos_snprintf(pstCtx->pszErrMsg, pstCtx->ulMsgLen, "ERR: Process encrypt result FAIL!");
        }

        DOS_ASSERT(0);
        goto fail;
    }

    //printf("Calc result. Algo: %d, Result: %d, Length: %d\r\n", enType, lRetVal, lOutputLen + lAfterLen);

    lOutputLen += lAfterLen;
    lRetVal = EVP_CIPHER_CTX_cleanup(&stCTX);
    if (lRetVal != SECURITY_SUCC)
    {
        if (DOS_ADDR_VALID(pstCtx->pszErrMsg) && 0 != pstCtx->ulMsgLen)
        {
            dos_snprintf(pstCtx->pszErrMsg, pstCtx->ulMsgLen, "ERR: Something happened will cleanup CRYPTION_CONTEXT_ST!");
        }

        DOS_ASSERT(0);
        goto fail;
    }

    pstCtx->paucDst = aucOutBuff;
    pstCtx->ulDstLen = (U32)lOutputLen;

    return DOS_SUCC;

fail:
    if (aucOutBuff)
    {
        dos_dmem_free(aucOutBuff);
        aucOutBuff = NULL;
    }

    pstCtx->paucDst = NULL;
    pstCtx->ulDstLen = 0;

    lRetVal = EVP_CIPHER_CTX_cleanup(&stCTX);
    return DOS_FAIL;
}


U32 cryption_decode(CRYPTION_CONTEXT_ST *pstCtx, CRYPTION_TYPES_EN enType)
{
    U8 *aucOutBuff = NULL;
    S32 lOutputLen = 0;
    S32 lAfterLen  = 0;
    S32 lRetVal     = SECURITY_FAIL;
    EVP_CIPHER_CTX stCTX;

    if (DOS_ADDR_INVALID(pstCtx))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstCtx->aucSrc) || !pstCtx->ulSrcLen)
    {
        if (DOS_ADDR_VALID(pstCtx->pszErrMsg) && 0 != pstCtx->ulMsgLen)
        {
            dos_snprintf(pstCtx->pszErrMsg, pstCtx->ulMsgLen, "ERR: Invalid input source data.");
        }

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstCtx->aucKey0) || !pstCtx->ulKeyLen0)
    {
        if (DOS_ADDR_VALID(pstCtx->pszErrMsg) && 0 != pstCtx->ulMsgLen)
        {
            dos_snprintf(pstCtx->pszErrMsg, pstCtx->ulMsgLen, "ERR: Invalid input key.");
        }

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    aucOutBuff= dos_dmem_alloc(CRYPTION_MAX_LEN);
    if (DOS_ADDR_INVALID(aucOutBuff))
    {
        if (DOS_ADDR_VALID(pstCtx->pszErrMsg) && 0 != pstCtx->ulMsgLen)
        {
            dos_snprintf(pstCtx->pszErrMsg, pstCtx->ulMsgLen, "ERR: Alloc memory FAIL!");
        }

        DOS_ASSERT(0);
        return DOS_FAIL;
    }


    lRetVal = SECURITY_FAIL;
    EVP_CIPHER_CTX_init(&stCTX);
    switch (enType)
    {
        case CRYPTION_BLOWFISH:
            lRetVal = EVP_DecryptInit_ex(&stCTX, EVP_bf_cfb64(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_DES_EDE_ECB:
            lRetVal = EVP_DecryptInit_ex(&stCTX, EVP_des_ede_ecb(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_DES_ECB:
            lRetVal = EVP_DecryptInit_ex(&stCTX, EVP_des_ecb(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_RC4:
            lRetVal = EVP_DecryptInit_ex(&stCTX, EVP_rc4(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_AES128_ECB:
            lRetVal = EVP_DecryptInit_ex(&stCTX, EVP_aes_128_ecb(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_AES128_CFB8:
            lRetVal = EVP_DecryptInit_ex(&stCTX, EVP_aes_128_cfb8(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_AES192_CFB128:
            lRetVal = EVP_DecryptInit_ex(&stCTX, EVP_aes_192_cfb128(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_RC2_CBC:
            lRetVal = EVP_DecryptInit_ex(&stCTX, EVP_rc2_cbc(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_RC2_CFB64:
            lRetVal = EVP_DecryptInit_ex(&stCTX, EVP_rc2_cfb64(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_CAST5_ECB:
            lRetVal = EVP_DecryptInit_ex(&stCTX, EVP_cast5_ecb(), NULL, pstCtx->aucKey0, NULL);
            break;

        case CRYPTION_CAST5_OFB:
            lRetVal = EVP_DecryptInit_ex(&stCTX, EVP_camellia_128_ofb(), NULL, pstCtx->aucKey0, NULL);
            break;

        default:
            goto fail;
    }

    if (lRetVal != SECURITY_SUCC)
    {
        if (DOS_ADDR_VALID(pstCtx->pszErrMsg) && 0 != pstCtx->ulMsgLen)
        {
            dos_snprintf(pstCtx->pszErrMsg, pstCtx->ulMsgLen, "ERR: Init encrypt engine FAIL!");
        }

        DOS_ASSERT(0);
        goto fail;
    }

    lRetVal = EVP_DecryptUpdate(&stCTX, aucOutBuff, &lOutputLen, pstCtx->aucSrc, pstCtx->ulSrcLen);
    if (lRetVal != SECURITY_SUCC)
    {
        if (DOS_ADDR_VALID(pstCtx->pszErrMsg) && 0 != pstCtx->ulMsgLen)
        {
            dos_snprintf(pstCtx->pszErrMsg, pstCtx->ulMsgLen, "ERR: Encrypt FAIL!");
        }

        DOS_ASSERT(0);
        goto fail;
    }

    lRetVal = EVP_DecryptFinal_ex(&stCTX, aucOutBuff + lOutputLen, &lAfterLen);
    if (lRetVal != SECURITY_SUCC)
    {
        if (DOS_ADDR_VALID(pstCtx->pszErrMsg) && 0 != pstCtx->ulMsgLen)
        {
            dos_snprintf(pstCtx->pszErrMsg, pstCtx->ulMsgLen, "ERR: Process encrypt result FAIL!");
        }

        DOS_ASSERT(0);
        goto fail;
    }

    //printf("Decode result. Algo: %d, Result: %d, Length: %d\r\n", enType, lRetVal, lOutputLen + lAfterLen);

    lOutputLen += lAfterLen;
    lRetVal = EVP_CIPHER_CTX_cleanup(&stCTX);
    if (lRetVal != SECURITY_SUCC)
    {
        if (DOS_ADDR_VALID(pstCtx->pszErrMsg) && 0 != pstCtx->ulMsgLen)
        {
            dos_snprintf(pstCtx->pszErrMsg, pstCtx->ulMsgLen, "ERR: Something happened will cleanup CRYPTION_CONTEXT_ST!");
        }

        DOS_ASSERT(0);
        goto fail;
    }

    pstCtx->paucDst = aucOutBuff;
    pstCtx->ulDstLen = (U32)lOutputLen;

    return DOS_SUCC;

fail:
    if (aucOutBuff)
    {
        dos_dmem_free(aucOutBuff);
        aucOutBuff = NULL;
    }

    pstCtx->paucDst = NULL;
    pstCtx->ulDstLen = 0;

    lRetVal = EVP_CIPHER_CTX_cleanup(&stCTX);
    return DOS_FAIL;
}

#endif /* end of INCLUDE_CRYPTION_LIB */

#ifdef __cplusplus
}
#endif /* __cplusplus */


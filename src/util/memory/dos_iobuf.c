#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

U32 dos_iobuf_init(IO_BUF_CB *pstIOBufCB, U32 ulLength)
{
    if (DOS_ADDR_INVALID(pstIOBufCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstIOBufCB->pszBuffer = NULL;
    pstIOBufCB->ulLength = 0;
    pstIOBufCB->ulSize = 0;

    if (ulLength >= IO_BUF_MAX_SIZE)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (0 != ulLength)
    {
        pstIOBufCB->pszBuffer = dos_dmem_alloc(ulLength);
        if (DOS_ADDR_INVALID(pstIOBufCB->pszBuffer))
        {
            DOS_ASSERT(0);
            return DOS_FAIL;
        }

        pstIOBufCB->ulSize = ulLength;
    }

    return DOS_SUCC;
}

U32 dos_iobuf_free(IO_BUF_CB *pstIOBufCB)
{
    if (DOS_ADDR_INVALID(pstIOBufCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstIOBufCB->pszBuffer)
    {
        dos_dmem_free(pstIOBufCB->pszBuffer);
        pstIOBufCB->pszBuffer = NULL;
    }

    pstIOBufCB->pszBuffer = NULL;
    pstIOBufCB->ulLength = 0;
    pstIOBufCB->ulSize = 0;

    return DOS_SUCC;
}

U32 dos_iobuf_dup(IO_BUF_CB *pstIOBufCBSrc, IO_BUF_CB *pstIOBufCBDst)
{
    if (DOS_ADDR_INVALID(pstIOBufCBSrc) || DOS_ADDR_INVALID(pstIOBufCBDst))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstIOBufCBDst->ulLength = 0;
    pstIOBufCBDst->ulSize = 0;

    if (pstIOBufCBSrc->ulSize)
    {
        pstIOBufCBDst->pszBuffer = dos_dmem_alloc(pstIOBufCBSrc->ulSize);
        if (DOS_ADDR_INVALID(pstIOBufCBDst->pszBuffer))
        {
            DOS_ASSERT(0);
            return DOS_FAIL;
        }

        dos_memcpy(pstIOBufCBDst->pszBuffer, pstIOBufCBSrc->pszBuffer, pstIOBufCBSrc->ulLength);
    }

    pstIOBufCBDst->ulLength = pstIOBufCBSrc->ulLength;
    pstIOBufCBDst->ulSize   = pstIOBufCBSrc->ulSize;

    return DOS_SUCC;
}

U32 dos_iobuf_resize(IO_BUF_CB *pstIOBufCB, U32 ulength)
{
    U8 *pData = NULL;

    if (DOS_ADDR_INVALID(pstIOBufCB)
        || 0 == ulength
        || ulength > IO_BUF_MAX_SIZE)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pData = dos_dmem_alloc(ulength);
    if (DOS_ADDR_INVALID(pData))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstIOBufCB->pszBuffer)
    {
        dos_dmem_free(pstIOBufCB->pszBuffer);
        pstIOBufCB->pszBuffer= NULL;
    }
    pstIOBufCB->pszBuffer = pData;
    pstIOBufCB->ulSize = ulength;
    pstIOBufCB->ulLength = 0;

    return DOS_SUCC;
}

U32 dos_iobuf_append(IO_BUF_CB *pstIOBufCB, const VOID *pData, U32 ulength)
{
    U8 *pDataTmp = NULL;

    if (DOS_ADDR_INVALID(pstIOBufCB)
        || DOS_ADDR_INVALID(pData)
        || 0 == ulength
        || ulength > IO_BUF_MAX_SIZE)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstIOBufCB->ulSize - pstIOBufCB->ulLength < ulength)
    {
        pDataTmp = dos_dmem_alloc(ulength + pstIOBufCB->ulSize);
        if (DOS_ADDR_INVALID(pDataTmp))
        {
            DOS_ASSERT(0);
            return DOS_FAIL;
        }

        pstIOBufCB->ulSize += ulength;

        if (pstIOBufCB->ulLength != 0)
        {
            dos_memcpy(pDataTmp, pstIOBufCB->pszBuffer, pstIOBufCB->ulLength);
        }

        if (pstIOBufCB->pszBuffer)
        {
            dos_dmem_free(pstIOBufCB->pszBuffer);
            pstIOBufCB->pszBuffer = NULL;
        }

        pstIOBufCB->pszBuffer = pDataTmp;
        pDataTmp = NULL;
    }

    dos_memcpy(pstIOBufCB->pszBuffer + pstIOBufCB->ulLength, pData, ulength);
    pstIOBufCB->ulLength += ulength;

    return DOS_SUCC;

}

U32 dos_iobuf_append_size(IO_BUF_CB *pstIOBufCB, U32 ulength)
{
    U8 *pData = NULL;

    if (DOS_ADDR_INVALID(pstIOBufCB)
        || 0 == ulength
        || ulength > IO_BUF_MAX_SIZE)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pData = dos_dmem_alloc(ulength + pstIOBufCB->ulSize);
    if (DOS_ADDR_INVALID(pData))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstIOBufCB->ulLength != 0)
    {
        dos_memcpy(pData, pstIOBufCB->pszBuffer, pstIOBufCB->ulLength);
    }

    if (pstIOBufCB->pszBuffer)
    {
        dos_dmem_free(pstIOBufCB->pszBuffer);
        pstIOBufCB->pszBuffer = NULL;
    }
    pstIOBufCB->pszBuffer = pData;
    pstIOBufCB->ulSize += ulength;

    return DOS_SUCC;
}


U8 *dos_iobuf_get_data(IO_BUF_CB *pstIOBufCB)
{
    if (DOS_ADDR_INVALID(pstIOBufCB))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    return pstIOBufCB->pszBuffer;
}

U32 dos_iobuf_get_length(IO_BUF_CB *pstIOBufCB)
{
    if (DOS_ADDR_INVALID(pstIOBufCB))
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    return pstIOBufCB->ulLength;
}

U32 dos_iobuf_get_size(IO_BUF_CB *pstIOBufCB)
{
    if (DOS_ADDR_INVALID(pstIOBufCB))
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    return pstIOBufCB->ulSize;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_mem.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 内存管理模块公共头文件， 如果没有启动内存管理模块，
 *        就需要使用系统内存管理相关的函数
 *     History:
 */

#ifndef __DOSMEM_H_
#define __DOSMEM_H_

#if INCLUDE_MEMORY_MNGT
    DLLEXPORT U32 _mem_mngt_init();
    DLLEXPORT VOID * _mem_alloc(S8 *pszFileName, U32 ulLine, U32 ulSize, U32 ulFlag);
    DLLEXPORT VOID _mem_free(VOID *p);

    #define dos_mem_mngt_init() _mem_mngt_init()
    #define dos_dmem_alloc(_size) _mem_alloc(__FILE__, __LINE__, (_size), 1)
    #define dos_dmem_free(_ptr) _mem_free((_ptr))

    #define dos_smem_alloc(_size) _mem_alloc(__FILE__, __LINE__, (_size), 0)
    #define dos_smem_free(_ptr) _mem_free((_ptr))

    #define dos_memcpy(_dst, _src, _len) memcpy((VOID *)(_dst), (VOID *)(_src), (_len))
    #define dos_memcmp(_ptr1, _ptr2, _len) memcmp((VOID *)(_ptr1), (VOID *)(_ptr2), (_len))
    #define dos_memzero(_ptr, _len) memset((VOID *)(_ptr), 0, (_len))
#else
    #define dos_dmem_alloc(_size) malloc((_size))
    #define dos_dmem_free(_ptr) free((_ptr))

    #define dos_smem_alloc(_size) malloc((_size))
    #define dos_smem_free(_ptr) free((_ptr))

    #define dos_memcpy(_dst, _src, _len) memcpy((VOID *)(_dst), (VOID *)(_src), (_len))
    #define dos_memcmp(_ptr1, _ptr2, _len) memcmp((VOID *)(_ptr1), (VOID *)(_ptr2), (_len))
    #define dos_memzero(_ptr, _len) memset((VOID *)(_ptr), 0, (_len))
#endif

#define DOS_ADDR_VALID(p) (NULL != (p))
#define DOS_ADDR_INVALID(p) (NULL == (p))

#endif

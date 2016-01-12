/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  memory.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 内存管理模块相关定义
 *     History:
 */

#ifndef __MEMORY_H__
#define __MEMORY_H__

#if INCLUDE_MEMORY_MNGT

/* 定义魔术字 */
#define MEM_CCB_MEGIC           0x002B3CF4
#define MEM_CCB_FREE_MEGIC      0x00EB3CF4

/* 定义内存类型 */
#define MEM_TYPE_STATIC   0x01000000
#define MEM_TYPE_DYNANIC  0x00000000

/* 相关处理过程 */
#define MEM_SET_MAGIC(p)                             \
do {                                                 \
    (p)->ulMemDesc = (p)->ulMemDesc | MEM_CCB_MEGIC; \
}while(0)

#define MEM_SET_TYPE(p, type)                      \
do {                                               \
    (p)->ulMemDesc = (p)->ulMemDesc | (type);      \
}while(0)

#define MEM_SET_FREE_MAGIC(p)                             \
do {                                                 \
    (p)->ulMemDesc = (p)->ulMemDesc | MEM_CCB_FREE_MEGIC; \
}while(0)


#define MEM_CHECK_MAGIC(p) (MEM_CCB_MEGIC == (p)->ulMemDesc)
#define MEM_CHECK_FREE_MAGIC(p) (MEM_CCB_FREE_MEGIC == (p)->ulMemDesc)

typedef struct tagMemInfoNode{
    HASH_NODE_S stNode;   /* hash表节点, 请保持该成员为第一个元素 */
    S8 szFileName[48];    /* 分配内存调用点所在的文件 */
    U32 ulLine;           /* 分配内存调用点所在的行数 */

    U32 ulRef;            /* 当前分配之后还没有释放的内存块数 */
    U32 ulTotalSize;      /* 当前内存分配点分配的内存大小总和, 当前还没有释放的内存 */
}MEM_INFO_NODE_ST;

typedef struct tagMemCCB{
    MEM_INFO_NODE_ST  *pstRefer;   /* 指向文件／行描述节点 */
    U32               ulMemDesc;   /* 高一字节指出该内存的type, 低三字节存储魔术字 */
    U32               ulSize;      /* 分配内存的大小 */
}MEM_CCB_ST;

typedef struct tagGetMemInfoParam
{
    S8      *szInfoBuff;
    U32     ulBuffSize;
    U32     ulBuffLen;

}GET_MEM_INFO_PARAM_ST;

#endif

#endif

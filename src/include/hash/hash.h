/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  hash.h
 *
 *  Created on: 2014-11-3
 *      Author: Larry
 *        Todo: 该文件定义系统提供的hash操作接口
 *     History:
 */

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif /* __cplusplus */
#endif /* __cplusplus */

#ifndef __INC_HASH_PUB_H__
#define __INC_HASH_PUB_H__

#include <hash/hash_list.h>

/*macros*/
#define  MATCH_NOT_FOUND    0
#define  INSERT_PRIORTO     1
#define  INSERT_NEXTTO      2

#define HASH_NODE_S   DLL_NODE_S
#define HASH_BUCKET_S DLL_S


typedef  struct HASH_TABLE_S{
         U32             ulHashSize;       /* Size of hash Table */
         U32             (*pInsertFunc)();  /* Pointer to Hash Insert Func */
         U32              NodeNum;
         HASH_BUCKET_S  HashList[1];       /* Array of Hash buckets */
} HASH_TABLE_S;


#define HASH_BUCKET_INIT   DLL_Init
#define HASH_ADD_NODE      DLL_Add
#define HASH_DELETE_NODE   dll_delete
#define HASH_INSERT_NODE   dll_insert
#define HASH_FIRST_NODE    DLL_First
#define HASH_LAST_NODE     DLL_Last
#define HASH_NEXT_NODE     DLL_Next
#define HASH_INIT_NODE     DLL_Init_Node
#define HASH_SCAN_BUCKET   DLL_Scan

#define HASH_Scan_Bucket(pHashTab,ulHashIndex,pNodePtr,TypeCast) \
        HASH_SCAN_BUCKET(&(pHashTab)->HashList[(ulHashIndex)],(pNodePtr),TypeCast)

#define HASH_Scan_Table(pHashTab,ulHashIndex) \
        for((ulHashIndex) = 0; (ulHashIndex) < (pHashTab)->ulHashSize; (ulHashIndex) = (ulHashIndex) +1)

#define HASH_Get_First_Bucket_Node(pHashTab,ulHashIndex) \
        HASH_FIRST_NODE(&pHashTab->HashList[ulHashIndex])

#define HASH_Get_Next_Bucket_Node(pHashTab,ulHashIndex,pNode) \
        HASH_NEXT_NODE(&pHashTab->HashList[ulHashIndex],pNode)

#define HASH_Bucket_Count(pHashTab,ulHashIndex) \
        DLL_Count(&((pHashTab)->HashList[(ulHashIndex)]))

#define HASH_Bucket_FreeAll(pHashTab,ulHashIndex,fnFree) \
        dll_free_all(&((pHashTab)->HashList[(ulHashIndex)]), fnFree)

#define HASH_Init_Node(pNode) HASH_INIT_NODE(pNode)

/*enums*/

/*error codes*/

/*structures*/

/*extern variables, export*/

/*extern functions, export*/
DLLEXPORT HASH_TABLE_S *hash_create_table (U32 ulHashSize, U32 (*pInsertFunc)());
DLLEXPORT HASH_NODE_S *hash_find_node(HASH_TABLE_S *pHashTab, U32 ulIndex,
    VOID *pKey, S32 (*fnValCmp)(VOID *, HASH_NODE_S *));
DLLEXPORT VOID hash_add_node (HASH_TABLE_S *pHashTab,HASH_NODE_S *pNode,U32 ulHashIndex,U8 *pu1InsertFuncParam);
DLLEXPORT VOID hash_delete_node (HASH_TABLE_S *pHashTab,HASH_NODE_S *pNode,U32 ulHashIndex);
DLLEXPORT VOID hash_delete_table (HASH_TABLE_S *pHashTab,VOID (*pFreeNodeMemFunc)(VOID *));
DLLEXPORT VOID hash_walk_bucket (HASH_TABLE_S *pHashTab, U32 ulHashIndex, VOID (*fnVisit)(HASH_NODE_S *));
DLLEXPORT VOID hash_walk_table (HASH_TABLE_S *pHashTab, VOID *pParam, VOID (*fnVisit)(HASH_NODE_S *, VOID *pParam));
DLLEXPORT VOID hash_free_allbucket (HASH_TABLE_S *pHashTab, VOID  (*pMemFreeFunc)(VOID *));


#endif/*__INC_HASH_PUB_H__*/

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */


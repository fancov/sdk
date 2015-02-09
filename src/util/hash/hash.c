/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  hash.c
 *
 *  Created on: 2014-11-3
 *      Author: Larry
 *        Todo: 该文件提供一个hash库
 *     History:
 */
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif /* __cplusplus */
#endif /* __cplusplus */


/* include dos header files */
#include <dos.h>

/*include own public header file   */

/*include private header file      */

/*private variables*/
/*static U32  ulHashTabCount = 0;*/

/*private functions prototypes*/
#define HASH_MALLOC(ulSize)  malloc(ulSize)
#define HASH_FREE(pData)     free(pData)

VOID hash_free_allbucket (HASH_TABLE_S *pHashTab, VOID  (*pMemFreeFunc)(VOID *));

/*public functions*/
/***************************************************************/
/*  Function Name   : hash_create_table                    */
/*  Description     : This procedure creates a hash table of   */
/*                    size 'ulHashSize' and returns a pointer  */
/*                    to the hash table.  If more than one node*/
/*                    hashes on to the same value, the order in*/
/*                    which the elements are to be placed is   */
/*                    determined by the insertion function.    */
/*                    If it is NULL, the Node is added to the  */
/*                    end of the list. The template for        */
/*                    pInsertFunc Is.                          */
/*                                                             */
/*                    U8 insert_fn ARG_LIST((HASH_NODE_S */
/*                         *pNode, U8 *pInsertFunc_param)); */
/*                    pInsertFunc_param is the parameter to be */
/*                    passed to the insert_fn when it is       */
/*                    invoked by HASH_append_Node. This    */
/*                    procedure is called for each Node in the */
/*                    list corresponding to the hash value. If */
/*                    the current Node is to be inserted before*/
/*                    this pNode,insert_fn should return TRUE. */
/*                    Otherwise FALSE.                         */
/*  Input(s)        : ulHashSize - Hash Table Size             */
/*                    pInsertFunc - Pointer to the insertion   */
/*                                    function                 */
/*                    u1MutexLockFlag - Flag indicating whether*/
/*                         to enable Mutual Exclustion or not  */
/*  Output(s)       : None                                     */
/*  Returns         : Pointer to the Hash Table                */
/***************************************************************/
DLLEXPORT HASH_TABLE_S *hash_create_table (U32 ulHashSize, U32 (*pInsertFunc)())
{
 HASH_TABLE_S *pHashTab;
 U32           ulHashIndex,ulHashMemSize;

 ulHashMemSize = sizeof(HASH_TABLE_S)+(ulHashSize-1)*sizeof(HASH_BUCKET_S);
 pHashTab = (HASH_TABLE_S *)HASH_MALLOC(ulHashMemSize);
 if(pHashTab != (HASH_TABLE_S *)NULL){
    pHashTab->ulHashSize      = ulHashSize;
    pHashTab->pInsertFunc      = pInsertFunc;

    for(ulHashIndex = 0; ulHashIndex < ulHashSize; ulHashIndex++){
        HASH_BUCKET_INIT(&pHashTab->HashList[ulHashIndex]);
    }
 }
 return(pHashTab);
}

/* find SLL node with special key */
DLLEXPORT HASH_NODE_S *hash_find_node(HASH_TABLE_S *pHashTab, U32 ulIndex,
    VOID *pKey, S32 (*fnValCmp)(VOID *, HASH_NODE_S *))
{
    HASH_NODE_S *pNode;

    HASH_Scan_Bucket(pHashTab, ulIndex, pNode, HASH_NODE_S *)
    {
        if (!fnValCmp(pKey, pNode))
            return pNode;
    }

    return NULL;
}

/***************************************************************/
/*  Function Name   : hash_add_node                        */
/*  Description     : This function puts the 'pNode' in hash   */
/*                    table in the hash value specified by     */
/*                    'ulHashIndex'.  It invokes the function  */
/*                    pointed by 'pInsertFunc' with the        */
/*                    specified 'pu1InsertFuncParam' to find   */
/*                    out where the element is to be inserted  */
/*                    in the list                              */
/*  Input(s)        : pHashTab - Pointer to the Hash Table in  */
/*                       which a the node needs to be inserted */
/*                    pNode - Pointer to the node which has to */
/*                              be added                       */
/*                    ulHashIndex - Value of the Hash Index    */
/*                    pu1InsertFuncParam - Parameter which will*/
/*                         be passed to the Insert Routine     */
/*  Output(s)       : None                                     */
/*  Returns         : None                                     */
/***************************************************************/
DLLEXPORT VOID hash_add_node (HASH_TABLE_S *pHashTab, HASH_NODE_S *pNode, U32 ulHashIndex, U8 *pu1InsertFuncParam)
{
 U8 u1Found = DOS_FALSE;
 HASH_NODE_S *pTmpNodePtr;
 HASH_NODE_S *pPrevNodePtr = NULL;
 S32   i4InsertPos = MATCH_NOT_FOUND;

 if(pHashTab->pInsertFunc == NULL){
    HASH_ADD_NODE(&pHashTab->HashList[ulHashIndex],pNode);
 }
 else{
    HASH_Scan_Bucket(pHashTab,ulHashIndex,pTmpNodePtr,HASH_NODE_S *){

       i4InsertPos= (S32)(*pHashTab->pInsertFunc)(pTmpNodePtr,pu1InsertFuncParam);
       if ((i4InsertPos == INSERT_NEXTTO) ||
             (i4InsertPos == INSERT_PRIORTO)) {
          u1Found = DOS_TRUE;
          break;
       }
       pPrevNodePtr = pTmpNodePtr;
    }

    if(u1Found == DOS_FALSE) pTmpNodePtr = HASH_LAST_NODE(&pHashTab->HashList[ulHashIndex]);

    /* The new has node needs to be inserted prior to the current node */
    if (i4InsertPos == INSERT_PRIORTO) {
       if (pPrevNodePtr)
          HASH_INSERT_NODE(&pHashTab->HashList[ulHashIndex], pPrevNodePtr, pNode);
       else  /* Needs to be added as the root/head node */
          HASH_INSERT_NODE(&pHashTab->HashList[ulHashIndex], NULL, pNode);
    }
    else /* Insert next to the current node */
       HASH_INSERT_NODE(&pHashTab->HashList[ulHashIndex], pTmpNodePtr, pNode);
 }
}

/***************************************************************/
/*  Function Name   : hash_delete_node                     */
/*  Description     : This procedure deletes the specified node*/
/*                    'pNode' from the hash table 'pHashTab'   */
/*  Input(s)        : pHashTab - Pointer to the Hash Table     */
/*                    pNode    - Pointer to the node which has */
/*                               to be deleted                 */
/*                    ulHashIndex -  Value of the Hash Index   */
/*  Output(s)       : None                                     */
/*  Returns         : None                                     */
/***************************************************************/
DLLEXPORT VOID hash_delete_node (HASH_TABLE_S *pHashTab, HASH_NODE_S  *pNode, U32 ulHashIndex)
{
 HASH_DELETE_NODE(&pHashTab->HashList[ulHashIndex],pNode);
}

/***************************************************************/
/*  Function Name   : hash_free_allbucket                    */
/*  Description     : This procedure deletes the hash table    */
/*                    'pHashTab'. For each node in the hash    */
/*                    table, it calls 'pMemFreeFun' because the*/
/*                    hash function has not allocated the space*/
/*                    for those nodes. If this pointer is null,*/
/*                    then it calls 'free' system call to      */
/*                    release the space                        */
/*  Input(s)        : pHashTab    - Pointer to the Hash Table  */
/*                    pMemFreeFun - Pointer to the Memory Free */
/*                                  function                   */
/*  Output(s)       : None                                     */
/*  Returns         : None                                     */
/***************************************************************/
DLLEXPORT VOID hash_free_allbucket (HASH_TABLE_S *pHashTab, VOID  (*pMemFreeFunc)(VOID *))
{
 U32           ulHashIndex;

 HASH_Scan_Table(pHashTab,ulHashIndex){
   HASH_Bucket_FreeAll(pHashTab, ulHashIndex, pMemFreeFunc);
 }
}

/***************************************************************/
/*  Function Name   : hash_delete_table                    */
/*  Description     : This procedure deletes the hash table    */
/*                    'pHashTab'. For each node in the hash    */
/*                    table, it calls 'pMemFreeFun' because the*/
/*                    hash function has not allocated the space*/
/*                    for those nodes. If this pointer is null,*/
/*                    then it calls 'free' system call to      */
/*                    release the space                        */
/*  Input(s)        : pHashTab    - Pointer to the Hash Table  */
/*                    pMemFreeFun - Pointer to the Memory Free */
/*                                  function                   */
/*  Output(s)       : None                                     */
/*  Returns         : None                                     */
/***************************************************************/
DLLEXPORT VOID hash_delete_table (HASH_TABLE_S *pHashTab, VOID  (*pMemFreeFunc)(VOID *))
{
    hash_free_allbucket(pHashTab, pMemFreeFunc);
/*  HASH_FREE(pHashTab);*/
}

DLLEXPORT VOID hash_walk_bucket (HASH_TABLE_S *pHashTab, U32 ulHashIndex, VOID (*fnVisit)(HASH_NODE_S *))
{
    HASH_NODE_S *pNode;

    HASH_Scan_Bucket(pHashTab, ulHashIndex, pNode, HASH_NODE_S *)
    {
        fnVisit(pNode);
    }
}

DLLEXPORT VOID hash_walk_table (HASH_TABLE_S *pHashTab, VOID *pParam, VOID (*fnVisit)(HASH_NODE_S *, VOID *pParam))
{
    U32 ulHashIndex;
    HASH_NODE_S *pNode;

    HASH_Scan_Table(pHashTab, ulHashIndex)
    {
        HASH_Scan_Bucket(pHashTab, ulHashIndex, pNode, HASH_NODE_S *)
        {
            fnVisit(pNode, pParam);
        }
    }
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

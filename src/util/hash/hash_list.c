/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  list.c
 *
 *  Created on: 2014-11-3
 *      Author: Larry
 *        Todo: 该文件实现一个双向链表
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

/*private functions prototypes*/

/*public functions*/

/* find DLL node with special key */
DLL_NODE_S *dll_find(DLL_S *pList, VOID *pKey,
    S32 (*fnValCmp)(VOID *, DLL_NODE_S *))
{
    DLL_NODE_S *pNode;

    DLL_Scan(pList, pNode, DLL_NODE_S *)
    {
        if (!fnValCmp(pKey, pNode))
            return pNode;
    }

    return NULL;
}

/***************************************************************/
/*  Function Name   : dll_insert_in_middle                 */
/*  Description     : Inserts the node 'pMid' in between       */
/*                    'pPrev' and 'pNext' and updates the node */
/*                    count of the 'pList                      */
/*  Input(s)        : pList - Pointer to the Doubly Linked List*/
/*                    pPrev - Pointer to previous Node         */
/*                    pMid  - Pointer to Node which needs to   */
/*                             be added                        */
/*                    pNext -  Pointer to the Next Node        */
/*  Output(s)       : None                                     */
/*  Returns         : None                                     */
/***************************************************************/
VOID dll_insert_in_middle (DLL_S  *pList, DLL_NODE_S *pPrev, DLL_NODE_S *pMid, DLL_NODE_S *pNext)
{
    pList->ulCount++;
    pMid->pNext  = pNext;
    pMid->pPrev  = pPrev;
    pPrev->pNext = pMid;
    pNext->pPrev = pMid;
}

/***************************************************************/
/*  Function Name   : dll_delete_in_middle                 */
/*  Description     : Removes the 'pNode' from the 'pList' and */
/*                    updates the pointers appropriately using */
/*                    'pPrev' and 'pNext'. After removing from */
/*                    the list, 'pNode' is initialised and     */
/*                    count of nodes is updated in 'pList'     */
/*  Input(s)        : pList - Pointer to the Linked List       */
/*                    pPrev - Pointer to Previous Node         */
/*                    pNode - Pointer to Node which is to be   */
/*                            removed                          */
/*                    pNext - Pointer to Next Node             */
/*  Output(s)       : None                                     */
/*  Returns         : None                                     */
/***************************************************************/
VOID dll_delete_in_middle (DLL_S  *pList, DLL_NODE_S *pPrev, DLL_NODE_S *pNode, DLL_NODE_S *pNext)
{
    pPrev->pNext = pNext;
    pNext->pPrev = pPrev;
    DLL_Init_Node(pNode);
    (pList)->ulCount--;
}

/***************************************************************/
/*  Function Name   : dll_insert                           */
/*  Description     : Inserts the node 'pNode' as a next node  */
/*                    to the 'pPrev' node and updates the node */
/*                    count of the 'pList'                     */
/*  Input(s)        : pList - Pointer to the Linked List       */
/*                    pPrev - Pointer to the node next to which*/
/*                            the new node needs to be inserted*/
/*                    pNode - Pointer of the node which needs  */
/*                            to be inserted                   */
/*  Output(s)       : None                                     */
/*  Returns         : None                                     */
/***************************************************************/
VOID dll_insert (DLL_S  *pList, DLL_NODE_S *pPrev, DLL_NODE_S *pNode)
{
    if(pPrev == NULL)
    {
        pPrev = &pList->Head;
    }
    dll_insert_in_middle(pList,pPrev,pNode,pPrev->pNext);
}

/* Delete a DLL node */
VOID dll_delete(DLL_S *pList, DLL_NODE_S *pNode)
{
    if (DLL_Is_Node_In_List(pNode))
    {
        dll_delete_in_middle(pList, pNode->pPrev, pNode, pNode->pNext);
    }
}

/***************************************************************/
/*  Function Name   : dll_fetch                              */
/*  Description     : Removes the first node from 'pList' and  */
/*                    returns the same.                        */
/*  Input(s)        : pList - Pointer to the Linked List       */
/*  Output(s)       : None                                     */
/*  Returns         : Pointer to the first Node in the list,   */
/*                       if the list is not empty              */
/*                    NULL, otherwise                          */
/***************************************************************/
DLL_NODE_S *dll_fetch (DLL_S *pList)
{
    DLL_NODE_S *pRetNode;

    if(DLL_Count(pList) == 0)
    {
        return(NULL);
    }

    pRetNode = pList->Head.pNext;
    dll_delete_in_middle(pList,&pList->Head,pList->Head.pNext,pList->Head.pNext->pNext);

    return(pRetNode);
}

/***************************************************************/
/*  Function Name   : dll_free_all                            */
/*  Description     : Removes and initialises all nodes from   */
/*                    the list and the memory is not freed     */
/*  Input(s)        : pList   -   Pointer to the Linked List   */
/*  Output(s)       : None                                     */
/*  Returns         : None                                     */
/***************************************************************/
VOID dll_free_all(DLL_S *pList, VOID (*fnFree)(VOID *))
{
    DLL_NODE_S *pTempNode,*pNode,*pHead;

    for(pHead = &pList->Head,pNode = pList->Head.pNext; pNode != pHead; pNode = pTempNode)
    {
        pTempNode = pNode->pNext;
        fnFree(pNode);
    }

    DLL_Init(pList);
}

S32 dll_walk(DLL_S *pList, VOID (*fnVisit)(DLL_NODE_S *))
{
    DLL_NODE_S *pNode;
    S32 nCnt;

    nCnt = 0;
    DLL_Scan(pList, pNode, DLL_NODE_S *)
    {
        fnVisit(pNode);
        nCnt++;
    }

    return nCnt;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

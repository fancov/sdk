/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  list.h
 *
 *  Created on: 2014-11-3
 *      Author: Larry
 *        Todo: 该文件提供系统双向链表的操作接口
 *     History:
 */

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif /* __cplusplus */
#endif /* __cplusplus */

#ifndef __INC_DLL_PUB_H__
#define __INC_DLL_PUB_H__

/*macros*/
/*************************************************
**** This Macro get the handle value of DLL node ****
**************************************************/
#define DLL_GET_HANDLE(pNode)	((pNode)->pHandle)
/*************************************************
**** This Macro set the handle value of DLL node ****
**************************************************/
#define DLL_SET_HANDLE(pNode, pValue)	((pNode)->pHandle = (VOID *)(pValue))

/***  MACRO Definition  ***/
#define DLL_Init(pList) \
    {\
        (pList)->Head.pNext = &(pList)-> Head; \
        (pList)->Head.pPrev = &(pList)-> Head; \
        (pList)->ulCount   = 0; \
    }

/***** This Macro Initialises The Node *****/
#define DLL_Init_Node(pNode) \
        (pNode)->pPrev = (pNode)->pNext = NULL;

#define DLL_Add(pList,pNode) \
        dll_insert_in_middle((pList),(pList)->Head.pPrev,(pNode),&(pList)->Head)

#define DLL_Count(pList) ((pList)->ulCount)

#define DLL_First(pList) \
        ((DLL_Count((pList)) == 0) ? NULL: (pList)->Head.pNext)

#define DLL_Last(pList) \
        ((DLL_Count((pList)) == 0) ? NULL : (pList)->Head.pPrev)

#define DLL_Next(pList,pNode) \
        (((pNode) == NULL) ? DLL_First(pList) : \
        (((pNode)->pNext == &(pList)->Head) ? NULL : (pNode)->pNext))

#define DLL_Previous(pList,pNode) \
        (((pNode) == NULL) ? DLL_Last(pList) : \
        (((pNode)->pPrev == &(pList)->Head) ? NULL : (pNode)->pPrev))

#define DLL_Is_Node_In_List(pNode) \
        (((pNode)->pNext != NULL) && \
         ((pNode)->pPrev != NULL) && \
         ((pNode)->pNext->pPrev == pNode) && \
         ((pNode)->pPrev->pNext == pNode))

/*********************************************************************
**** This Macro Is Useful For Scanning Through The Entire List    ****
**** 'pNode' Is A Temp. Variable Of Type 'TypeCast'. pList is the ****
**** Pointer To linked_list                                       ****
**********************************************************************/
#define DLL_Scan(pList,pNode,TypeCast) \
        for(pNode = (TypeCast)(DLL_First((pList))); \
            pNode != NULL; \
            pNode = (TypeCast)(DLL_Next((pList),((DLL_NODE_S *)(pNode)))))

/*enums*/

/*error codes*/

/*structures*/
typedef struct dll_node
{
    struct dll_node *pNext; /* Points at the next node in the list */
    struct dll_node*pPrev; /* Points at the previous node in the list */
    VOID  *pHandle;          /* reserved for user */
}DLL_NODE_S;

typedef struct DLL{
    DLL_NODE_S Head;  /**** Header List Node        ****/
    U32      ulCount; /**** Number Of Nodes In List ****/
}DLL_S;

/*extern variables, export*/

/*extern functions, export*/
DLL_NODE_S *dll_find(DLL_S *pList, VOID *pKey,
S32 (*fnValCmp)(VOID *, DLL_NODE_S *));
VOID  dll_insert_in_middle (DLL_S *pList, DLL_NODE_S *pPrev, DLL_NODE_S *pMid, DLL_NODE_S *pNext);
#undef dll_insert
VOID  dll_insert (DLL_S *pList, DLL_NODE_S *pPrev, DLL_NODE_S *pNode);
VOID  dll_delete_in_middle (DLL_S *pList, DLL_NODE_S *pPrev, DLL_NODE_S *pNode, DLL_NODE_S *pNext);
VOID  dll_delete(DLL_S *pList, DLL_NODE_S *pNode);
DLL_NODE_S *dll_fetch (DLL_S *pList);
VOID dll_free_all (DLL_S *pList, VOID (*fnFree)(VOID *));
S32 dll_walk(DLL_S *pList, VOID (*fnVisit)(DLL_NODE_S *));

#endif/*__INC_DLL_PUB_H__*/

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
/*
typedef struct list_s{
    struct list_s * prev;
    struct list_s * next;
}list_t;

typedef struct node_s{
    list_t node;

    void * data;
}node_t;

*/
DLLEXPORT void __list_add(struct list_s * new_node,
                  struct list_s * prev,
                  struct list_s * next)
{
    next->prev = new_node;
    new_node->next = next;
    new_node->prev = prev;
    prev->next = new_node;
}

DLLEXPORT void __list_del(struct list_s *prev, struct list_s *next)
{
    next->prev = prev;
    prev->next = next;
}


DLLEXPORT void dos_list_init( struct list_s *list )
{
    if(!list)
    {
        DOS_ASSERT(0);
        return;
    }
    list->prev = list;
    list->next = list;
}

DLLEXPORT void dos_list_add_head( struct list_s *list, struct list_s *node )
{
    if(!list || !node)
    {
        DOS_ASSERT(0);
        return;
    }
    __list_add(node, list, list->next);
}


DLLEXPORT void dos_list_add_tail( struct list_s *list, struct list_s *node )
{
    if(!list || !node)
    {
        DOS_ASSERT(0);
        return;
    }
    __list_add(node, list->prev, list);
}

DLLEXPORT struct list_s *dos_list_fetch( struct list_s *list )
{
    struct list_s *node = NULL;

    if(!list)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    if( list->next != list )
    {
        node = list->next;        
        __list_del(node->prev, node->next);
        node->next = NULL;
        node->prev = NULL;
    }
    return node;
}

DLLEXPORT void dos_list_del( struct list_s *node )
{
    if(!node)
    {
        DOS_ASSERT(0);
        return;
    }
    
    __list_del(node->prev, node->next);
    node->next = NULL;
    node->prev = NULL;
}

DLLEXPORT struct list_s * dos_list_work(struct list_s *list, struct list_s *node)
{
    if (list == node->next)
    {
        return NULL;
    }

    return node->next;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */



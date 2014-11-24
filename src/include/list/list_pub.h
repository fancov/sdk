#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#ifndef __LIST_H__
#define __LIST_H__


#define list_node_init( node )\
do { \
    (node)->next = NULL; (node)->prev = NULL; \
} while (0)

#define list_is_empty( node ) ( (node)==((node)->next) )

#if OS_64
#define list_entry(ptr, type, member) \
        ((type *)((S8*)(ptr)-(U64)(&((type *)0)->member)))
#elif OS_32
#define list_entry(ptr, type, member) \
        ((type *)((S8*)(ptr)-(U32)(&((type *)0)->member)))
#endif


typedef struct list_s
{
    struct list_s *prev; /*previous element*/
    struct list_s *next; /*next element*/
}list_t;

void list_init( list_t *list );
void list_add_head( list_t *list, list_t *node );
void list_add_tail( list_t *list, list_t *node );
void list_del( list_t *node );
list_t *list_fetch( list_t *list );

#endif/*__LIST_H_*/

#ifdef __cplusplus
}
#endif /* __cplusplus */




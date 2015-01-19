#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#ifndef __LIST_H__
#define __LIST_H__


#define dos_list_node_init( node )\
do { \
    (node)->next = NULL; (node)->prev = NULL; \
} while (0)

#define dos_list_is_empty( node ) ( (node)==((node)->next) )

#if OS_64
#define dos_list_entry(ptr, type, member) \
        ((type *)((S8*)(ptr)-(U64)(&((type *)0)->member)))
#elif OS_32
#define dos_list_entry(ptr, type, member) \
        ((type *)((S8*)(ptr)-(U32)(&((type *)0)->member)))
#endif


typedef struct list_s
{
    struct list_s *prev; /*previous element*/
    struct list_s *next; /*next element*/
}list_t;

DLLEXPORT void dos_list_init( list_t *list );
DLLEXPORT void dos_list_add_head( list_t *list, list_t *node );
DLLEXPORT void dos_list_add_tail( list_t *list, list_t *node );
DLLEXPORT void dos_list_del( list_t *node );
DLLEXPORT list_t *dos_list_fetch( list_t *list );
DLLEXPORT list_t *dos_list_work(struct list_s *list, struct list_s *node);

#endif/*__LIST_H_*/

#ifdef __cplusplus
}
#endif /* __cplusplus */




#include <stdio.h>
#include <stdint.h>
#include "const.h"
#include "list.h"

/* init the list */
inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

inline void __list_add(struct list_head *item, struct list_head *prev, struct list_head *next)
{
    next->prev = item;
    item->next = next;
    item->prev = prev;
    prev->next = item;
}

inline void list_add_tail(struct list_head *item, struct list_head *head)
{
    if(item != NULL)
        __list_add(item, head, head->next);
}

inline void list_add_head(struct list_head *item, struct list_head *head)
{
    if(item != NULL)
        __list_add(item,head->prev, head);
}

inline void __list_del(struct list_head *prev, struct list_head *next)
{
    if(next)
        next->prev = prev;

    if(prev)
        prev->next = next;
}

/* del the list */
inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = NULL;
    entry->prev = NULL;
}

inline void list_del_init(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    INIT_LIST_HEAD(entry);
}

/* list is empty */
inline int list_empty(const struct list_head *head)
{
    return ((head->next == head) || (head->prev == head));
}

inline int list_empty_careful(const struct list_head *head)
{
    return ((head->next == head) && (head->prev == head));
}

/* only get don't del */
inline int list_get_head(struct list_head *head, struct list_head **entry)
{
    if(head == NULL || entry == NULL)
    {
        return -1;
    }

    if(head->next != head)
    {
        *entry = head->next;
        return 0;
    }

    return -2;
}

inline int list_get_tail(struct list_head *head, struct list_head **entry)
{
    if(head == NULL || entry == NULL)
    {
        return -1;
    }

    if(head->prev != head) 
    {
        *entry = head->prev;
        return 0;
    }

    return -2;
}

inline int list_get_head_del(struct list_head *head, struct list_head **entry)
{
    if(list_get_head(head, entry) == 0)
    {
        if(*entry)
        {
            list_del(*entry);
            return 0;
        }
    }

    return -2;
}

inline int list_get_tail_del(struct list_head *head, struct list_head **entry)
{
    if(list_get_tail(head, entry))
    {
        if(*entry)
        {
            list_del(*entry);
            return 0;
        }
    }

    return -2;
}

/* the list num */
inline int list_num(struct list_head *head)
{
    int num = 0;
    struct list_head *pos, *n;
    list_for_each_safe(pos, n, head)
    {
        ++num;
    }

    return num;
}

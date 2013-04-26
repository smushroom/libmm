#ifndef     _PTE_H_
#define     _PTE_H_

#include "debug.h"
#include "page.h"

#define     PTE_TYPE        5
#define     PTE_TYPE_MASK   ((1UL << PTE_TYPE) -1)
#define     PTE_SHIFT       12
//#define     PTE_TYPE_SHIFT(e)  (sizeof(e)*8 - PTE_TYPE)
#define     PTE_FLAGS_MASK      (((-1L) << PTE_TYPE ) & (((1UL) << PTE_SHIFT) -1))

typedef struct 
{
    unsigned long val;
}pte_t;

typedef struct
{
    pte_t ptes[DEFAULT_PAGE_FNS];
}pgd_t;

/* pte_t
 * 0    5         11                                63
 *|--------------------------------------------------|
 *| type|   flags |         index                    | 
 *|--------------------------------------------------|
 * */

static inline unsigned long pgd_index(const pgd_t *pgd, const pte_t *pte)
{
    return (pte - pgd->ptes);
}

static inline unsigned int pte_flags(const pte_t *pte)
{
    //DD("pte flags = 0x%lx", (pte.val & PTE_FLAGS_MASK));
    //DD("flags mask = 0x%lx", PTE_FLAGS_MASK);
    //DD("pte flags = 0x%lx", (pte->val & PTE_FLAGS_MASK));
    return (pte->val & PTE_FLAGS_MASK);
}

static inline unsigned long pte_index(const pte_t *pte)
{
    return (pte->val >>  PTE_SHIFT);
}

static inline unsigned long pte_type(const pte_t *pte)
{
    return ((pte->val & PTE_TYPE_MASK));
}

static inline int empty_pte(const pte_t *pte)
{
    return (pte->val == 0);
}

static inline void make_pte(const unsigned long index, const unsigned long flags, unsigned long type, pte_t *pte)
{
    //DD("make pte index = 0x%lx", index);
    //DD("make pte flags = 0x%lx", flags);
    //DD("make pte type = 0x%lx", type);
    //pte->val = ((index << PTE_SHIFT) | ((flags << PTE_TYPE) & PTE_FLAGS_MASK) | (type >> PTE_TYPE_SHIFT(pte)));
    pte->val = ((index << PTE_SHIFT) | ((flags << PTE_TYPE) & PTE_FLAGS_MASK) | (type & PTE_TYPE_MASK));
    //DD("make_pte pte = 0x%lx", pte->val);
}

pte_t * get_pte(pgd_t *pgd, unsigned long index);
pte_t *_set_pte(pte_t *pte, unsigned long index, unsigned long flags, unsigned int type);
pte_t *set_pte(pgd_t *pgd, unsigned long index, unsigned int type);
pte_t *_set_swp_pte(pte_t *pte, unsigned long index, unsigned long swp_index, unsigned long flags, unsigned int type);
pte_t *set_swp_pte(pgd_t *pgd, unsigned long index, unsigned long swp_index, unsigned int type);

#define     PTE_PRESENT     0x01 
#define     PTE_present(pte)   ((pte_flags(pte) >> PTE_TYPE) & PTE_PRESENT)

#endif

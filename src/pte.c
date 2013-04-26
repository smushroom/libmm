#include <stdio.h>
#include "debug.h"
#include "pte.h"

/*static unsigned int next_pte = 1;*/

pte_t* get_pte(pgd_t *pgd, unsigned long index)
{
    /*unsigned long index = pfn_to_index(address);*/
    DD("index = 0x%lx.", index);
    pte_t *pte = &(pgd->ptes[index]);

    return pte;
}

pte_t * _set_pte(pte_t *pte, unsigned long index, unsigned long flags, unsigned int type)
{
    /*unsigned long index = pfn_to_index(address);*/
    if(!empty_pte(pte))
    {
        return NULL;
    }

    make_pte(index, flags, type,pte);
    
    return pte;
}

pte_t * set_pte(pgd_t *pgd, unsigned long index, unsigned int type)
{
    DD("PTE_PRESENT = 0x%x", PTE_PRESENT);
    DD("index = 0x%lx.", index);
    pte_t *pte = &(pgd->ptes[index]); 

    return _set_pte(pte, index, PTE_PRESENT,type);
}


pte_t * _set_swp_pte(pte_t *pte, unsigned long index, unsigned long swp_index, unsigned long flags, unsigned int type)
{
    /*unsigned long index = pfn_to_index(address);*/
    make_pte(swp_index,flags, type,pte);

    return pte;
}

pte_t * set_swp_pte(pgd_t *pgd, unsigned long index, unsigned long swp_index, unsigned int type)
{
    pte_t *pte = &(pgd->ptes[index]);
    return _set_swp_pte(pte, index, swp_index, 0,type);
}

int put_pte(pte_t *pte)
{
    pte->val = 0;
}

#if 0
int put_pte(pgd_t *pgd, unsigned long address)
{
    int index = pfn_to_index(address);
    pte_t *pte = &(pgd->ptes[index]);

    pte->val = 0;
}
#endif

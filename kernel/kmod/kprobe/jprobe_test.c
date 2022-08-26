/*
 * NOTE: This example is works on x86 and powerpc.
 * Here's a sample kernel module showing the use of kprobes to dump a
 * stack trace and selected registers when _do_fork() is called.
 *
 * For more information on theory of operation of kprobes, see
 * Documentation/kprobes.txt
 *
 * You will see the trace data in /var/log/messages and on the console
 * whenever _do_fork() is invoked to create a new process.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>

#define MAX_SYMBOL_LEN	64
static char symbol[MAX_SYMBOL_LEN] = "shrink_page_list";
module_param_string(symbol, symbol, sizeof(symbol), 0644);

/* For each probe you need to allocate a kprobe structure */

 
static long jdo_fork(unsigned long clone_flags, unsigned long stack_start,
	      unsigned long stack_size, int __user *parent_tidptr,
	      int __user *child_tidptr)
{
	pr_info("jprobe: clone_flags = 0x%lx, stack_start = 0x%lx "
		"stack_size = 0x%lx\n", clone_flags, stack_start, stack_size);
 
	/* Always end with a call to jprobe_return(). */
	jprobe_return();
	return 0;
}
 

static struct jprobe my_jprobe = {
	.entry			= jdo_fork,
	.kp = {
		.symbol_name	= "do_fork",
	},
};

static int __init jprobe_init(void)
{
	int ret;
 
	ret = register_jprobe(&my_jprobe);
	if (ret < 0) {
		printk(KERN_INFO "register_jprobe failed, returned %d\n", ret);
		return -1;
	}
	printk(KERN_INFO "Planted jprobe at %p, handler addr %p\n",
	       my_jprobe.kp.addr, my_jprobe.entry);
	return 0;
}

static void __exit jprobe_exit(void)
{
	unregister_jprobe(&my_jprobe);
	printk(KERN_INFO "jprobe at %p unregistered\n", my_jprobe.kp.addr);
}


module_init(jprobe_init)
module_exit(jprobe_exit)
MODULE_LICENSE("GPL");

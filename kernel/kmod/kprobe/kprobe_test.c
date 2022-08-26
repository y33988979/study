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

#define TEST 1

static int is_machine_kexec = 0;

static int handler_pre_ext_machine_kexec(struct kprobe *p, struct pt_regs *regs)
{
	is_machine_kexec = 1;
	return 0;
}

static int handler_post_ext_machine_kexec(struct kprobe *p, struct pt_regs *regs,
						unsigned long flags)
{
	printk("the is_machine_kexec is %d\n", is_machine_kexec);
	//is_machine_kexec = 0;
	return 0;
}

static int handler_fault_ext_machine_kexec(struct kprobe *p, struct pt_regs *regs,
						int  trapnr)
{
	return 0;
}
#include "handler_ext_condition_module.h"
#include "handler_ext_none_module.h"
DEF_EXT_CONDITION_HANDLE(__flush_dcache_area, is_machine_kexec)
DEF_EXT_CONDITION_HANDLE(flush_icache_range, is_machine_kexec)
#ifdef TEST
DEF_EXT_NONE_HANDLE(kvm_vm_ioctl)
#endif

#include "handler_module.h"

DEF_KP_OBJ(machine_kexec)
DEF_KP_OBJ(__flush_dcache_area)
DEF_KP_OBJ(flush_icache_range)

#ifdef TEST
DEF_KP_OBJ(kvm_vm_ioctl)
#endif
static int __init kprobe_init(void)
{
	register_kprobe_obj(machine_kexec);
	register_kprobe_obj(__flush_dcache_area);
	register_kprobe_obj(flush_icache_range);
#ifdef TEST
	register_kprobe_obj(kvm_vm_ioctl);
#endif
	return 0;
err:
	return -1;
}

static void __exit kprobe_exit(void)
{
	unregister_kprobe_obj(machine_kexec);
	unregister_kprobe_obj(__flush_dcache_area);
	unregister_kprobe_obj(flush_icache_range);
#ifdef TEST
	unregister_kprobe_obj(kvm_vm_ioctl);
#endif
	return;
}

module_init(kprobe_init)
module_exit(kprobe_exit)
MODULE_LICENSE("GPL");

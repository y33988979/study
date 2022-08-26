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

//#define TEST 1
#define KPROBE_KEXEC 1

#ifdef KPROBE_KEXEC 
static int is_machine_kexec = 0;

static int handler_pre_ext_machine_kexec_0(struct kprobe *p, struct pt_regs *regs)
{
	is_machine_kexec = 1;
	return 0;
}

static int handler_post_ext_machine_kexec_0(struct kprobe *p, struct pt_regs *regs,
						unsigned long flags)
{
	printk("the is_machine_kexec is %d\n", is_machine_kexec);
	//is_machine_kexec = 0;
	return 0;
}

static int handler_fault_ext_machine_kexec_0(struct kprobe *p, struct pt_regs *regs,
						int  trapnr)
{
	return 0;
}
#endif

#include "handler_ext_condition_module.h"
#include "handler_ext_none_module.h"

#ifdef KPROBE_KEXEC 
DEF_FUNC_EXT_CONDITION_HANDLE(__flush_dcache_area, is_machine_kexec)
DEF_OBJ_EXT_CONDITION_HANDLE(__flush_dcache_area, is_machine_kexec, 32) 	//dc
DEF_FUNC_EXT_CONDITION_HANDLE(__flush_cache_user_range, is_machine_kexec)
DEF_OBJ_EXT_CONDITION_HANDLE(__flush_cache_user_range, is_machine_kexec, 40) 		//dc
DEF_OBJ_EXT_CONDITION_HANDLE(__flush_cache_user_range, is_machine_kexec, 28) 		//dc
DEF_OBJ_EXT_CONDITION_HANDLE(__flush_cache_user_range, is_machine_kexec, 88) 		//ic
DEF_OBJ_EXT_CONDITION_HANDLE(__flush_cache_user_range, is_machine_kexec, 96) 		//ic
#endif

#ifdef TEST
DEF_FUNC_EXT_NONE_HANDLE(kvm_vm_ioctl)
DEF_OBJ_EXT_NONE_HANDLE(kvm_vm_ioctl, 12)
#endif

#include "handler_module.h"

#ifdef KPROBE_KEXEC 
DEF_KP_FUNC(machine_kexec)
DEF_KP_FUNC(__flush_dcache_area)
DEF_KP_OBJ(__flush_dcache_area, 32)
DEF_KP_FUNC(__flush_cache_user_range)
DEF_KP_OBJ(__flush_cache_user_range, 40)
DEF_KP_OBJ(__flush_cache_user_range, 28)
DEF_KP_OBJ(__flush_cache_user_range, 88)
DEF_KP_OBJ(__flush_cache_user_range, 96)
#endif

#ifdef TEST
DEF_KP_FUNC(kvm_vm_ioctl)
DEF_KP_OBJ(kvm_vm_ioctl, 12)
#endif
static int __init kprobe_init(void)
{
#ifdef KPROBE_KEXEC 
	register_kprobe_func(machine_kexec);
	register_kprobe_func(__flush_dcache_area);
	register_kprobe_obj(__flush_dcache_area, 32);
	register_kprobe_func(__flush_cache_user_range);
	register_kprobe_obj(__flush_cache_user_range, 40);
	register_kprobe_obj(__flush_cache_user_range, 28);
	register_kprobe_obj(__flush_cache_user_range, 88);
	register_kprobe_obj(__flush_cache_user_range, 96);
#endif

#ifdef TEST
	register_kprobe_func(kvm_vm_ioctl);
	register_kprobe_obj(kvm_vm_ioctl, 12);
#endif
	return 0;

ERR_REG_OBJ(__flush_cache_user_range, 96):
	unregister_kprobe_obj(__flush_cache_user_range, 96);
ERR_REG_OBJ(__flush_cache_user_range, 88):
	unregister_kprobe_obj(__flush_cache_user_range, 88);
ERR_REG_OBJ(__flush_cache_user_range, 28):
	unregister_kprobe_obj(__flush_cache_user_range, 28);
ERR_REG_OBJ(__flush_cache_user_range, 40):
	unregister_kprobe_obj(__flush_cache_user_range, 40);
ERR_REG_FUNC(__flush_cache_user_range):
	unregister_kprobe_func(__flush_cache_user_range);
ERR_REG_OBJ(__flush_dcache_area, 32):
	unregister_kprobe_obj(__flush_dcache_area, 32);
ERR_REG_FUNC(__flush_dcache_area):
	unregister_kprobe_func(__flush_dcache_area);
ERR_REG_FUNC(machine_kexec):
	unregister_kprobe_func(machine_kexec);
	return -1;
}

static void __exit kprobe_exit(void)
{
#ifdef KPROBE_KEXEC 
	unregister_kprobe_func(machine_kexec);
	unregister_kprobe_func(__flush_dcache_area);
	unregister_kprobe_obj(__flush_dcache_area, 32);
	unregister_kprobe_func(__flush_cache_user_range);
	unregister_kprobe_obj(__flush_cache_user_range, 40);
	unregister_kprobe_obj(__flush_cache_user_range, 28);
	unregister_kprobe_obj(__flush_cache_user_range, 88);
	unregister_kprobe_obj(__flush_cache_user_range, 96);
#endif

#ifdef TEST
	unregister_kprobe_func(kvm_vm_ioctl);
	unregister_kprobe_obj(kvm_vm_ioctl, 12);
#endif
	return;
}

module_init(kprobe_init)
module_exit(kprobe_exit)
MODULE_LICENSE("GPL");

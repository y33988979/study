#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/kallsyms.h>
#include <asm/ptrace.h>

#define EXPIRE_TIMEDOUT (30 * 1000)

static __always_inline unsigned long reg_sp(void)
{
	u64 sp;

	asm volatile("mov %0, sp" : "=r" (sp));
	return sp;
}

typedef void (*serror_fn)(struct pt_regs *regs, unsigned int esr);

static __init int serror_init(void)
{
        unsigned long sp = reg_sp();
        unsigned long addr;
	serror_fn serror_func;

        printk("serror Module inits.\n");

	addr = kallsyms_lookup_name("do_serror");
	serror_func = (serror_fn)addr;
	if (serror_func)
		serror_func((void*)sp, 0xbf000002);
        return 0;
}

static __exit void serror_exit(void)
{
        printk("serror Module exits.\n");
}

module_init(serror_init);
module_exit(serror_exit);

MODULE_LICENSE("GPL");

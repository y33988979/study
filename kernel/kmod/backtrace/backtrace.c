#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/time.h>

#define EXPIRE_TIMEDOUT (30 * 1000)

static __always_inline unsigned long reg_sp(void)
{
	u64 sp;

	asm volatile("mov %0, sp" : "=r" (sp));
	return sp;
}

#define reg_val(reg) \
({							\
	unsigned long _val;				\
	asm volatile("mov %0, " #reg : "=r" (_val));	\
	_val;						\
})

static noinline int print_backtrace(struct task_struct *tsk)
{
	int i,n;
	u64 sp, fp, lr, addr;

	fp = reg_val(fp);
	lr = reg_val(lr);
	sp = reg_sp();
	//dump_stack();
	printk("%s:%d: sp=0x%llx\n", __func__,__LINE__, sp);
	printk("%s:%d: x29(fp)=0x%llx, 0x30(lr)=0x%llx\n", __func__,__LINE__, fp, lr);
	printk("%s:%d: __builtin_frame_address(0)=0x%llx\n", __func__,__LINE__, __builtin_frame_address(0));

	printk("%s:%d: x29(*fp)=0x%llx\n", __func__,__LINE__, *((u64*)fp));

	addr = fp;
	for (i=0; i<4; i++) {
		printk("lr=%pS\n", (u64*)lr);
		fp = *(u64*)fp;
		lr = *(u64*)(fp + 8);
	}


	return 0;
}

static noinline int func3(int a3, int b3)
{
	int e, f;

	e = ++a3;
	f = ++b3;

	printk("%s:%d: sp=0x%lx\n", __func__,__LINE__, reg_sp());
	print_backtrace(current);
	printk("func3: e+f=%d\n", e+f);
	return e + f;
}

static noinline int func2(int a2, int b2)
{
	int c, d;

	printk("%s:%d: sp=0x%lx\n", __func__,__LINE__, reg_sp());
	c = ++a2;
	d = ++b2;
	return func3(c, d);
}

static noinline int func1(int a1, int b1)
{
	int a, b;

	printk("%s:%d: sp=0x%lx\n", __func__,__LINE__, reg_sp());
	a = ++a1;
	b = ++b1;
	return func2(a, b);
}

static __init int backtrace_init(void)
{
	int n = 0;
	int id = smp_processor_id();

        printk("backtrace Module inits.\n");
	n = func1(id, id);
        printk("n = %d.\n", n);

        return 0;
}

static __exit void backtrace_exit(void)
{
        printk("backtrace Module exits.\n");
}

module_init(backtrace_init);
module_exit(backtrace_exit);

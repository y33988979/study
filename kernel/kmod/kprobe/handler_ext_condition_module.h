#include "handler.h"

#define DEF_EXT_CONDITION_HANDLE(_handler_func_sym, __condition)	\
static int handler_fault_ext_##_handler_func_sym(struct kprobe *p, struct pt_regs *regs, int trapnr) \
{							\
	return __condition ? 0 : -1;			\
}							\
							\
static int handler_pre_ext_##_handler_func_sym(struct kprobe *p, struct pt_regs *regs)	 \
{							\
	return __condition ? 0 : -1;			\
}							\
							\
static int handler_post_ext_##_handler_func_sym(struct kprobe *p, struct pt_regs *regs, unsigned long flags)	\
{							\
	return __condition ? 0 : -1;			\
}

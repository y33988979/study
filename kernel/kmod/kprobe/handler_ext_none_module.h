#include "handler.h"

#define DEF_EXT_NONE_HANDLE(_handler_func_sym)	\
static int handler_fault_ext_##_handler_func_sym(struct kprobe *p, struct pt_regs *regs, int trapnr) \
{							\
	return  0;					\
}							\
							\
static int handler_pre_ext_##_handler_func_sym(struct kprobe *p, struct pt_regs *regs)	 \
{							\
	return  0;					\
}							\
							\
static int handler_post_ext_##_handler_func_sym(struct kprobe *p, struct pt_regs *regs, unsigned long flags)	\
{							\
	return  0;					\
}

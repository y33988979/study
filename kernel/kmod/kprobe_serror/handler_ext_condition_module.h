#include "handler.h"

#define DEF_OBJ_EXT_CONDITION_HANDLE(_handler_func_sym, __condition, _offset)	\
static int handler_fault_ext_##_handler_func_sym##_##_offset(struct kprobe *p, struct pt_regs *regs, int trapnr) \
{							\
	return __condition ? 0 : -1;			\
}							\
							\
static int handler_pre_ext_##_handler_func_sym##_##_offset(struct kprobe *p, struct pt_regs *regs)	 \
{							\
	return __condition ? 0 : -1;			\
}							\
							\
static int handler_post_ext_##_handler_func_sym##_##_offset(struct kprobe *p, struct pt_regs *regs, unsigned long flags)	\
{							\
	return __condition ? 0 : -1;			\
}

#define DEF_FUNC_EXT_CONDITION_HANDLE(_handler_func_sym, __condition)	\
	DEF_OBJ_EXT_CONDITION_HANDLE(_handler_func_sym, __condition, 0)

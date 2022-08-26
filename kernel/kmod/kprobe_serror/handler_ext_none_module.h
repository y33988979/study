#include "handler.h"

#define DEF_OBJ_EXT_NONE_HANDLE(_handler_func_sym, _offset)	\
static int handler_fault_ext_##_handler_func_sym##_##_offset(struct kprobe *p, struct pt_regs *regs, int trapnr) 				      \
{							\
	return  0;					\
}							\
							\
static int handler_pre_ext_##_handler_func_sym##_##_offset(struct kprobe *p, struct pt_regs *regs)	 \
{							\
	return  0;					\
}							\
							\
static int handler_post_ext_##_handler_func_sym##_##_offset(struct kprobe *p, struct pt_regs *regs, unsigned long flags)	\
{							\
	return  0;					\
}


#define DEF_FUNC_EXT_NONE_HANDLE(_handler_func_sym)	 \
	DEF_OBJ_EXT_NONE_HANDLE(_handler_func_sym, 0)

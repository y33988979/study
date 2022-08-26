#include "handler.h"

#define DEF_SYM(_handler_func_sym) 	\
static char sym_##_handler_func_sym[MAX_SYMBOL_LEN] = STR(_handler_func_sym);

#define DEF_HANDLER(_handler_func_sym) \
static int handler_fault_##_handler_func_sym(struct kprobe *p, struct pt_regs *regs, int trapnr) \
{								\
	int ret = 0;						\
	ret = handler_fault_ext_##_handler_func_sym(p, regs, 	\
						trapnr);	\
	if (ret < 0) {						\
		return 0;					\
	}							\
	return handler_fault(p, regs, trapnr);			\
}								\
								\
static int handler_pre_##_handler_func_sym(struct kprobe *p, struct pt_regs *regs)	 \
{								\
	int ret = 0;						\
	ret = handler_pre_ext_##_handler_func_sym(p, regs); 	\
								\
	if (ret < 0) {						\
		return 0;					\
	}							\
	return handler_pre(p, regs);				\
}								\
								\
static void handler_post_##_handler_func_sym(struct kprobe *p, struct pt_regs *regs, unsigned long flags)	\
{								\
	int ret = 0;						\
	ret = handler_post_ext_##_handler_func_sym(p, regs, flags); \
								\
	if (ret < 0) {						\
		return ;					\
	}							\
	handler_post(p, regs, flags);				\
}								\


#define DEF_KP(_handler_func_sym) 						\
static struct kprobe kp_##_handler_func_sym = {			\
	.symbol_name = sym_##_handler_func_sym,			\
	.pre_handler = handler_pre_##_handler_func_sym,		\
	.post_handler = handler_post_##_handler_func_sym,	\
	.fault_handler = handler_fault_##_handler_func_sym,	\
};


#define DEF_KP_OBJ(_handler_func_sym)	\
	DEF_SYM(_handler_func_sym)	\
	DEF_HANDLER(_handler_func_sym)	\
	DEF_KP(_handler_func_sym)

#ifndef __HANDLE_MODULE_H__
#define __HANDLE_MODULE_H__
#define KP_OBJ(sym) kp_##sym

#define register_kprobe_obj(_sym)			\
	do {						\
		int _ret = 0;				\
		_ret = register_kprobe(&KP_OBJ(_sym));	\
		if (_ret < 0) {				\
			pr_err("register_kprobe failed, returned %d\n", _ret);	\
			goto err;						\
		}								\
		pr_info("Planted kprobe at %p\n", KP_OBJ(_sym).addr);	\
	}while(0)

#define unregister_kprobe_obj(_sym) 		\
	do {					\
		unregister_kprobe(&KP_OBJ(_sym));	\
		pr_info("kprobe at %p unregistered\n", KP_OBJ(_sym).addr);	\
	}while(0)

#endif

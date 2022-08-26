#include "handler.h"

#define DEF_SYM(_handler_func_sym, _offset) 	\
static char sym_##_handler_func_sym##_##_offset[MAX_SYMBOL_LEN] = STR(_handler_func_sym);

#define DEF_HANDLER(_handler_func_sym, _offset) \
static int handler_fault_##_handler_func_sym##_##_offset(struct kprobe *p, struct pt_regs *regs, int trapnr) \
{										\
	int ret = 0;								\
	ret = handler_fault_ext_##_handler_func_sym##_##_offset(p, regs, 	\
						trapnr);			\
	if (ret < 0) {								\
		return 0;							\
	}									\
	return handler_fault(p, regs, trapnr);					\
}										\
										\
static int handler_pre_##_handler_func_sym##_##_offset(struct kprobe *p, struct pt_regs *regs)	 \
{										\
	int ret = 0;								\
	ret = handler_pre_ext_##_handler_func_sym##_##_offset(p, regs); 	\
										\
	if (ret < 0) {								\
		return 0;							\
	}									\
	return handler_pre(p, regs);						\
}										\
										\
static void handler_post_##_handler_func_sym##_##_offset(struct kprobe *p, struct pt_regs *regs, unsigned long flags)	\
{										\
	int ret = 0;								\
	ret = handler_post_ext_##_handler_func_sym##_##_offset(p, regs, flags); \
										\
	if (ret < 0) {								\
		return ;							\
	}									\
	handler_post(p, regs, flags);						\
}										\


#define DEF_KP(_handler_func_sym, _offset) 				\
static struct kprobe kp_##_handler_func_sym##_##_offset = {		\
	.symbol_name = sym_##_handler_func_sym##_##_offset,		\
	.pre_handler = handler_pre_##_handler_func_sym##_##_offset,	\
	.post_handler = handler_post_##_handler_func_sym##_##_offset,	\
	.fault_handler = handler_fault_##_handler_func_sym##_##_offset,	\
	.offset = _offset,						\
};


#define DEF_KP_OBJ(_handler_func_sym,_offset)	\
	DEF_SYM(_handler_func_sym,_offset)	\
	DEF_HANDLER(_handler_func_sym,_offset)	\
	DEF_KP(_handler_func_sym,_offset)

#define DEF_KP_FUNC(_handler_func_sym)		\
	DEF_KP_OBJ(_handler_func_sym, 0)


#ifndef __HANDLE_MODULE_H__
#define __HANDLE_MODULE_H__
#define KP_OBJ(sym, offset) kp_##sym##_##offset

#define ERR_REG_OBJ(sym, offset) err_##sym##_##offset
#define ERR_REG_FUNC(sym)  ERR_REG_OBJ(sym, 0)

#define register_kprobe_obj(_sym,_offset)			\
	do {							\
		int _ret = 0;					\
		_ret = register_kprobe(&KP_OBJ(_sym,_offset));	\
		if (_ret < 0) {					\
			pr_err("register_kprobe failed, returned %d\n", _ret);	\
			goto ERR_REG_OBJ(_sym, _offset);			\
		}								\
		pr_info("Planted kprobe at %p\n", KP_OBJ(_sym,_offset).addr);	\
	}while(0)

#define unregister_kprobe_obj(_sym, _offset) 		\
	do {					\
		unregister_kprobe(&KP_OBJ(_sym,_offset));				\
		pr_info("kprobe at %p unregistered\n", KP_OBJ(_sym, _offset).addr);	\
	}while(0)

#define register_kprobe_func(_sym) 	register_kprobe_obj(_sym, 0)
#define unregister_kprobe_func(_sym)	unregister_kprobe_obj(_sym, 0)
#endif

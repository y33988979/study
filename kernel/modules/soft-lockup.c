/** 
 * @file    soft-lockup.c 
 * @author  Yu Chen
 * @date    2020-07-07 
 * @version 0.1 
 * @brief  trigger a soft lockup!
*/
#include <linux/module.h>     /* Needed by all modules */ 
#include <linux/kernel.h>     /* Needed for KERN_INFO */ 
#include <linux/init.h>       /* Needed for the macros */ 

#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/kthread.h>
#include <linux/nmi.h>
#include <linux/kallsyms.h>
#include <linux/stop_machine.h>

#include <asm/current.h>


#define MUITL_CPU_LOCKUP
extern int stop_one_cpu(unsigned int cpu, cpu_stop_fn_t fn, void *arg);

///< The license type -- this affects runtime behavior 
MODULE_LICENSE("GPL"); 
  
///< The author -- visible when you use modinfo 
MODULE_AUTHOR("Yu Chen"); 
  
///< The description -- see modinfo 
MODULE_DESCRIPTION("trigger a soft lockup!"); 
  
///< The version of the module 
MODULE_VERSION("0.1"); 

static int cpu = 1;
static int lockup_cpu[] = {2,3};
static int delay = 1000;

static struct cpu_stop_work cpu_stop_work;
module_param(cpu, int, 0644);
MODULE_PARM_DESC(cpu, "which cpu.");

module_param(delay, int, 0644);
MODULE_PARM_DESC(delay, "delay value in milliseconds.");

static int is_lockup_cpu(int cpu)
{
	int i;

	for(i=0; i<sizeof(lockup_cpu)/sizeof(int); i++) {
		if (cpu == lockup_cpu[i])
			return 1;
	}
	return 0;
}

static int cpu_stop_fn(void *arg)
{
	int i;
	int cpu = raw_smp_processor_id();

	pr_err("cpu%d: preempt_count %d.\n", cpu, preempt_count());
	if (!is_lockup_cpu(cpu)) {
		printk("cpu%d: cpu_stop_fn exit..\n", cpu);
		return 0;
	}

	for (i=0; i<40; i++) {
		mdelay(delay);
	}

	return 0;
}

extern int stop_one_cpu(unsigned int cpu, cpu_stop_fn_t fn, void *arg);
static int lockup_trigger(void)
{
	int i;
	int (*stop_one_cpu)(unsigned int, cpu_stop_fn_t, void *) = NULL;
	int (*stop_one_cpu_nowait)(unsigned int, cpu_stop_fn_t, void *, struct cpu_stop_work*) = NULL;

#ifdef MUITL_CPU_LOCKUP
	stop_one_cpu_nowait = kallsyms_lookup_name("stop_one_cpu_nowait");
	if (stop_one_cpu_nowait) {
		for(i=0; i<sizeof(lockup_cpu)/sizeof(int); i++) {
			stop_one_cpu_nowait(lockup_cpu[i], cpu_stop_fn, NULL, &cpu_stop_work);
		}
	}
#else
	stop_one_cpu = kallsyms_lookup_name("stop_one_cpu");
	if (stop_one_cpu)
		stop_one_cpu(1, cpu_stop_fn, NULL);
#endif
	return 0;
}

static int lockup_trigger_thread(void *args)
{
	struct cpumask lockup_cpumask;

	ssleep(5);
	cpumask_clear(&lockup_cpumask);	
	cpumask_set_cpu(cpu, &lockup_cpumask);	
	stop_machine(cpu_stop_fn, NULL, &lockup_cpumask);
	return 0;
}

static int __init softlockup_init(void) 
{ 
	pr_err("softlockup_init: pid %d lockup cpu%d\n", current->pid, cpu); 
	//kthread_run(lockup_trigger_thread, NULL, "lockup_trigger");
	lockup_trigger();
	return 0; 
} 
  
static void __exit softlockup_exit(void) 
{ 
	printk(KERN_INFO "softlockup_exit\n"); 
} 
  
module_init(softlockup_init); 
module_exit(softlockup_exit);



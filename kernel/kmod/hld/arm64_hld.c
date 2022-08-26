// SPDX-License-Identifier: GPL-2.0
/*
 * Detect hard for aarch64 system
 * 
 */

#define pr_fmt(fmt) "arm64_hld: " fmt

#include <linux/mm.h>
#include <linux/cpu.h>
#include <linux/nmi.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>

#include <uapi/linux/sched/types.h>
#include <asm/irq_regs.h>
#include <linux/kthread.h>
#include <linux/sched/clock.h>

#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

unsigned long __read_mostly hld_watchdog_enabled;
int __read_mostly hld_watchdog_thresh = 10;


/*
 * Should we panic when a hard-lockup occurs:
 */
static unsigned int __read_mostly hardlockup_panic = 0;

/* Global variables, exported for sysctl */

static u64 __read_mostly sample_period;
static DEFINE_MUTEX(hld_watchdog_mutex);

static DEFINE_PER_CPU(unsigned long, hld_watchdog_touch_ts);
static DEFINE_PER_CPU(struct hrtimer, hld_watchdog_hrtimer);
static DEFINE_PER_CPU(unsigned long, hld_hrtimer_interrupts);
//static DEFINE_PER_CPU(bool, soft_hld_watchdog_warn);

DEFINE_PER_CPU(struct work_struct, hld_works);

/*
 * Hard-lockup warnings should be triggered after just a few seconds. Soft-
 * lockups can have false positives under extreme conditions. So we generally
 * want a higher threshold for soft lockups than for hard lockups. So we couple
 * the thresholds with a factor: we make the soft threshold twice the amount of
 * time the hard threshold is.
 */
static int get_hardlockup_thresh(void)
{
	return hld_watchdog_thresh;
}

/*
 * Returns seconds, approximately.  We don't need nanosecond
 * resolution, and we don't need to waste time with a big divide when
 * 2^30ns == 1.074s.
 */
static unsigned long get_timestamp(void)
{
	return local_clock() >> 30LL;  /* 2^30 ~= 10^9 */
}

static void hld_set_sample_period(void)
{
	/*
	 * convert hld_watchdog_thresh from seconds to ns
	 * the divide by 5 is to give hrtimer several chances (two
	 * or three with the current relation between the soft
	 * and hard thresholds) to increment before the
	 * hardlockup detector generates a warning
	 */
	sample_period = get_hardlockup_thresh() * ((u64)NSEC_PER_SEC / 5);
}

/* Commands for resetting the watchdog */
static void __touch_watchdog(void)
{
	__this_cpu_write(hld_watchdog_touch_ts, get_timestamp());
}


static int __is_hardlockup(unsigned long touch_ts)
{
	unsigned long now = get_timestamp();

	if (hld_watchdog_enabled && hld_watchdog_thresh){
		/* Warn about unreasonable delays. */
		if (time_after(now, touch_ts + get_hardlockup_thresh()))
			return now - touch_ts;
	}
	return 0;
}

static void hld_watchdog_interrupt_count(void)
{
	__this_cpu_inc(hld_hrtimer_interrupts);
}

/* watchdog kicker functions */
static enum hrtimer_restart hld_watchdog_timer_fn(struct hrtimer *hrtimer)
{
	//unsigned long touch_ts = __this_cpu_read(hld_watchdog_touch_ts);
	struct pt_regs *regs = get_irq_regs();

	if (!hld_watchdog_enabled)
		return HRTIMER_NORESTART;

	/* touch watchdog and Increase the interrupt count */
	__touch_watchdog();
	hld_watchdog_interrupt_count();

	/* .. and repeat */
	hrtimer_forward_now(hrtimer, ns_to_ktime(sample_period));

	return HRTIMER_RESTART;
}

static void hld_watchdog_enable(struct work_struct *work)
{
	struct hrtimer *hrtimer = this_cpu_ptr(&hld_watchdog_hrtimer);

	/*
	 * Start the timer first to prevent the NMI watchdog triggering
	 * before the timer has a chance to fire.
	 */
	hrtimer_init(hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer->function = hld_watchdog_timer_fn;
	hrtimer_start(hrtimer, ns_to_ktime(sample_period),
		      HRTIMER_MODE_REL_PINNED);

	/* Initialize timestamp */
	__touch_watchdog();
}

static void hld_watchdog_disable(struct work_struct *work)
{
	struct hrtimer *hrtimer = this_cpu_ptr(&hld_watchdog_hrtimer);
	hrtimer_cancel(hrtimer);
}

#define WATCHDOG_TIMER_START	1
#define WATCHDOG_TIMER_STOP	2

static void hld_watchdog_timer_start_stop(int action)
{
	int cpu;
	struct work_struct *work = NULL;

	for_each_possible_cpu(cpu) {
		work = per_cpu_ptr(&hld_works, cpu);
		if (action == WATCHDOG_TIMER_START)
			INIT_WORK(work, hld_watchdog_enable);
		else
			INIT_WORK(work, hld_watchdog_disable);
	}

	for_each_online_cpu(cpu)
		queue_work_on(cpu, system_highpri_wq,
			      per_cpu_ptr(&hld_works, cpu));

	for_each_online_cpu(cpu)
		flush_work(per_cpu_ptr(&hld_works, cpu));

}

static void hld_watchdog_check(void)
{
	int cpu;
	int duration;
	unsigned long touch_ts, now;

	now = get_timestamp();

	for_each_online_cpu(cpu) {
		touch_ts = per_cpu(hld_watchdog_touch_ts, cpu);

		duration = __is_hardlockup(touch_ts);
		if (unlikely(duration)) {
			add_taint(TAINT_AUX, LOCKDEP_STILL_OK);
			pr_warn("CPU##%d: irq off %ds, now: %lus, last_touch: %lus\n", cpu, duration, now, touch_ts);
		}
	}
}

static void hldd_set_prio(unsigned int policy, unsigned int prio)
{
	struct sched_param param = { .sched_priority = prio };
	sched_setscheduler(current, policy, &param);
}

static int hldd(void *args)
{
	set_user_nice(current, MIN_NICE);
	hldd_set_prio(SCHED_FIFO, MAX_RT_PRIO - 5);

	while (!kthread_should_stop()) {
		schedule_timeout_interruptible(5*HZ);
		hld_watchdog_check();
	}
	return 0;
}

static int start_stop_hldd_kthread(void)
{
	static struct task_struct *hldd_thread __read_mostly;
	static DEFINE_MUTEX(hldd_mutex);
	int err = 0;

	mutex_lock(&hldd_mutex);
	if (hld_watchdog_enabled) {
		if (!hldd_thread)
			hldd_thread = kthread_run(hldd, NULL, "hldd");
		if (IS_ERR(hldd_thread)) {
			pr_err("hldd: kthread_run(hldd) failed\n");
			err = PTR_ERR(hldd_thread);
			hldd_thread = NULL;
			goto fail;
		}
		pr_info("hardlockup detect thread[%d] started.\n", hldd_thread->pid);
	} else if (hldd_thread) {
		kthread_stop(hldd_thread);
		pr_info("hardlockup detect thread[%d] stopped.\n", hldd_thread->pid);
		hldd_thread = NULL;
	}
fail:
	mutex_unlock(&hldd_mutex);
	return err;
}


/*
 * Create the watchdog timer infrastructure and hardlockup detect thread.
 */
static __init void hld_watchdog_setup(void)
{
	/*
	 * If sysctl is off and watchdog got disabled on the command line,
	 * nothing to do here.
	 */
	if (!(hld_watchdog_enabled && hld_watchdog_thresh))
		return;

	mutex_lock(&hld_watchdog_mutex);
	if (num_online_cpus() < num_possible_cpus())
		pr_warn("WARNING: num_online_cpus = %d, num_possible_cpus = %d\n",
			 num_online_cpus(), num_possible_cpus());

	hld_set_sample_period();

	hld_watchdog_timer_start_stop(WATCHDOG_TIMER_START);

	mutex_unlock(&hld_watchdog_mutex);
	pr_info("hld watchdog timer start success!\n");

	start_stop_hldd_kthread();
}

/*
 * /proc/hld_status
 */
static int hld_status_show(struct seq_file *m, void *v)
{
	int cpu;
	unsigned long touch_ts, irq_cnt;

	seq_puts(m, "hardlockup detect status:\n");
	seq_puts(m, "cpu irq count:");
	for_each_online_cpu(cpu) {
		irq_cnt = per_cpu(hld_hrtimer_interrupts, cpu);
		seq_printf(m, " %6lu", irq_cnt);
	}
	seq_puts(m, "\n");
	seq_puts(m, "cpu touch ts:");
	for_each_online_cpu(cpu) {
		touch_ts = per_cpu(hld_watchdog_touch_ts, cpu);
		seq_printf(m, " %6lu", touch_ts);
	}
	seq_puts(m, "\n");
	return 0;
}

static ssize_t hld_status_write(struct file *file, const char __user *buf,
 				size_t count, loff_t *offs)
{
	unsigned long val = 0;
	int ret;

	mutex_lock(&hld_watchdog_mutex);
	ret = kstrtoul_from_user(buf, count, 10, &val);
	if (ret)
		goto unlock;

unlock:
	mutex_unlock(&hld_watchdog_mutex);
	return count;
}

static int hld_status_open(struct inode *inode, struct file *file)
{
	return single_open(file, hld_status_show, NULL);
}

static const struct file_operations hld_status_fops = {
	.owner			= THIS_MODULE,
	.open			= hld_status_open,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.write			= hld_status_write,
	.release		= single_release,
};

/*
 * /proc/hld_thresh
 */
static int hld_thresh_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d \n", hld_watchdog_thresh);
	return 0;
}

static void hld_account_reset(void)
{
	int cpu;
	for_each_possible_cpu(cpu) {
		per_cpu(hld_hrtimer_interrupts, cpu) = 0;
		per_cpu(hld_watchdog_touch_ts, cpu) = 0;
	}

}
static ssize_t hld_thresh_write(struct file *file, const char __user *buf,
 				size_t count, loff_t *offs)
{
	unsigned long val = 0;
	int old, ret;

	mutex_lock(&hld_watchdog_mutex);
	old = READ_ONCE(hld_watchdog_thresh);
	ret = kstrtoul_from_user(buf, count, 10, &val);
	if (ret)
		goto unlock;

	WRITE_ONCE(hld_watchdog_thresh, val);
	hld_set_sample_period();

	if (old != 0 && val == 0) {
		hld_watchdog_enabled = 0;
		hld_watchdog_timer_start_stop(WATCHDOG_TIMER_STOP);
		start_stop_hldd_kthread();
		hld_account_reset();
	}

	if (old == 0 && val != 0) {
		hld_watchdog_enabled = 1;
		start_stop_hldd_kthread();
		hld_watchdog_timer_start_stop(WATCHDOG_TIMER_START);
	}
unlock:
	mutex_unlock(&hld_watchdog_mutex);
	return count;
}

static int hld_thresh_open(struct inode *inode, struct file *file)
{
	return single_open(file, hld_thresh_show, NULL);
}

static const struct file_operations hld_thresh_fops = {
	.owner			= THIS_MODULE,
	.open			= hld_thresh_open,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.write			= hld_thresh_write,
	.release		= single_release,
};

static struct proc_dir_entry *hld_thresh_proc;
static struct proc_dir_entry *hld_status_proc;
static __init int hld_proc_create(void)
{
	hld_thresh_proc = proc_create("hld_thresh", S_IWUSR | S_IRUGO, NULL, &hld_thresh_fops);
	hld_status_proc = proc_create("hld_status", S_IWUSR | S_IRUGO, NULL, &hld_status_fops);
	return 0;
}

static __init int hld_proc_remove(void)
{
	if (hld_thresh_proc)
		proc_remove(hld_thresh_proc);
	if (hld_status_proc)
		proc_remove(hld_status_proc);
	return 0;
}

static __init int arm64_hld_watchdog_init(void)
{
	hld_watchdog_enabled = 1;
	hld_watchdog_setup();
	hld_proc_create();
	return 0;
}

static __exit void arm64_hld_watchdog_exit(void)
{
	hld_watchdog_enabled = 0;
	start_stop_hldd_kthread();
	hld_watchdog_timer_start_stop(WATCHDOG_TIMER_STOP);
	hld_proc_remove();
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("chen.yu@easystack.cn");
module_init(arm64_hld_watchdog_init);
module_exit(arm64_hld_watchdog_exit);


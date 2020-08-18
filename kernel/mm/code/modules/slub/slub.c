
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
#include <linux/workqueue.h>

#include <linux/slab.h>

#include <asm/current.h>

#define USE_ALLOC_CALLBACK	1
#if 0
struct kmem_cache {
        struct kmem_cache_cpu __percpu *cpu_slab;
        /* Used for retriving partial slabs etc */
        slab_flags_t flags;
        unsigned long min_partial;
        unsigned int size;      /* The size of an object including meta data */
        unsigned int object_size;/* The size of an object without meta data */
        unsigned int offset;    /* Free pointer offset. */
#ifdef CONFIG_SLUB_CPU_PARTIAL
        /* Number of per cpu partial objects to keep around */
        unsigned int cpu_partial;
#endif
        struct kmem_cache_order_objects oo;

        /* Allocation and freeing of slabs */
        struct kmem_cache_order_objects max;
        struct kmem_cache_order_objects min;
        gfp_t allocflags;       /* gfp flags to use on each alloc */
        int refcount;           /* Refcount for slab cache destroy */
        void (*ctor)(void *); 
        unsigned int inuse;             /* Offset to metadata */
        unsigned int align;             /* Alignment */
        unsigned int red_left_pad;      /* Left redzone padding size */
        const char *name;       /* Name (only for display!) */
        struct list_head list;  /* List of slab caches */
#ifdef CONFIG_SYSFS
        struct kobject kobj;    /* For sysfs */
        struct work_struct kobj_remove_work;
#endif
#ifdef CONFIG_MEMCG
        struct memcg_cache_params memcg_params;
        /* for propagation, maximum size of a stored attr */
        unsigned int max_attr_size;
#ifdef CONFIG_SYSFS
        struct kset *memcg_kset;
#endif
#endif

#ifdef CONFIG_SLAB_FREELIST_HARDENED
        unsigned long random;
#endif

#ifdef CONFIG_NUMA
        /*
 *          * Defragmentation by allocating from a remote node.
 *                   */
        unsigned int remote_node_defrag_ratio;
#endif

#ifdef CONFIG_SLAB_FREELIST_RANDOM
        unsigned int *random_seq;
#endif

#ifdef CONFIG_KASAN
        struct kasan_cache kasan_info;
#endif

        unsigned int useroffset;        /* Usercopy region offset */
        unsigned int usersize;          /* Usercopy region size */

        struct kmem_cache_node *node[MAX_NUMNODES];
};
#endif

struct kmem_cache_node {
        spinlock_t list_lock;

#ifdef CONFIG_SLAB
        struct list_head slabs_partial; /* partial list first, better asm code */
        struct list_head slabs_full;
        struct list_head slabs_free;
        unsigned long total_slabs;      /* length of all slab lists */
        unsigned long free_slabs;       /* length of free slab list only */
        unsigned long free_objects;
        unsigned int free_limit;
        unsigned int colour_next;       /* Per-node cache coloring */
        struct array_cache *shared;     /* shared per node */
        struct alien_cache **alien;     /* on other nodes */
        unsigned long next_reap;        /* updated without locking */
        int free_touched;               /* updated without locking */
#endif

#ifdef CONFIG_SLUB
        unsigned long nr_partial;
        struct list_head partial;
#ifdef CONFIG_SLUB_DEBUG
        atomic_long_t nr_slabs; 
        atomic_long_t total_objects;
        struct list_head full;
#endif
#endif

};

struct slub_flag_str {
	int flag;
	char name[32];
}slub_flags_str[] = {
	{SLAB_CONSISTENCY_CHECKS, "SLAB_CONSISTENCY_CHECKS"},
	{SLAB_RED_ZONE, "SLAB_RED_ZONE"},
	{SLAB_POISON, "SLAB_POISON"},
	{SLAB_HWCACHE_ALIGN, "SLAB_HWCACHE_ALIGN"},
	{SLAB_CACHE_DMA, "SLAB_CACHE_DMA"},
	{SLAB_STORE_USER, "SLAB_STORE_USER"},
	{SLAB_PANIC, "SLAB_PANIC"},
	{SLAB_DESTROY_BY_RCU, "SLAB_DESTROY_BY_RCU"},
	{SLAB_MEM_SPREAD, "SLAB_MEM_SPREAD"},
	{SLAB_TRACE, "SLAB_TRACE"},
	{SLAB_DEBUG_OBJECTS, "SLAB_DEBUG_OBJECTS"},
	{SLAB_NOLEAKTRACE, "SLAB_NOLEAKTRACE"},
	{SLAB_FAILSLAB, "SLAB_FAILSLAB"},
	{SLAB_ACCOUNT, "SLAB_ACCOUNT"},
	//{SLAB_KASAN, "SLAB_KASAN"},
	{SLAB_RECLAIM_ACCOUNT, "SLAB_RECLAIM_ACCOUNT"},
	//{SLAB_DEACTIVATED, "SLAB_DEACTIVATED"}
};

#define OBJ_COUNT	1000
struct foo {
	char name[28];
	int val;
	char pad[210];
}; /*size=256*/

static struct task_struct *worker_task;
static struct task_struct *slub_show_task;
static struct kmem_cache  *test_cachep = NULL;
static struct foo *objs[OBJ_COUNT];
static int curr_obj_idx = 0;

static void slub_free(void);

static void slub_flags_show(struct kmem_cache *s)
{
	int i, n = 0, off = 0;
	char buf[512] = {0};

	for (i=0; i<ARRAY_SIZE(slub_flags_str); i++) {
		if (slub_flags_str[i].flag & s->flags) {
			n = sprintf(buf+off, "%s,", slub_flags_str[i].name);
			off += n;
		}
	}
	pr_err("flags: %s\n", buf);
}

static void slub_cache_show(struct kmem_cache *s)
{
	struct kmem_cache_cpu *c;
	struct kmem_cache_node *n;
	struct page *p;
	int cpu = 0;

	pr_err("------------------------ slub_cache_show start ---------------------\n");
	slub_flags_show(s);
	pr_err("flags: 0x%lx, size: %d, obj_sz: %d, offset %d, inuse %d, min_partial %ld\n",
		s->flags, s->size, s->object_size, s->offset, s->inuse, s->min_partial);
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(s->cpu_slab, cpu);
		pr_err("kmem_cache_cpu%d: freelist %p tid %03d, page %p, partial %p\n",
			cpu, c->freelist, c->tid, c->page, c->partial);
		p = c->page;
		if (p)
			pr_err("cpu%d->page:    freelist %p, inuse %u, objects %u, frozen %u, pages %d, next %p\n",
				cpu, p->freelist, p->inuse, p->objects, p->frozen, p->pages, p->next);
		p = c->partial;
		if (p)
			pr_err("cpu%d->partial: freelist %p, inuse %u, objects %u, frozen %u, pages %d, next %p\n",
				cpu, p->freelist, p->inuse, p->objects, p->frozen, p->pages, p->next);
	}
	
	n = s->node[0];
	if (n)
		pr_err("kmem_cache_node%d: nr_partial %lu, nr_slabs %ld, total_objects %ld\n",0 ,n->nr_partial,n->nr_slabs,n->total_objects);
	pr_err("------------------------ slub_cache_show end   ---------------------\n");
}

static int slub_show_thread(void *args)
{
	if (!test_cachep)
		return 0;

	while (!kthread_should_stop()) {
		slub_cache_show(test_cachep);
		msleep_interruptible(3000);
		set_current_state(TASK_RUNNING);
	}
}

static void *foo_alloc(void)
{
	struct foo *ptr = NULL;

	ptr = kmem_cache_alloc(test_cachep, GFP_KERNEL);
	if (!ptr) {
		pr_err("kmem_cache_alloc failed!\n");
	}
	objs[curr_obj_idx++] = ptr;
	if (curr_obj_idx == OBJ_COUNT) {
		pr_err("[ERR]: alloc objects number has been maximized(%d)!!! free all objects..\n", OBJ_COUNT);
		slub_free();
		curr_obj_idx = 0;
	}
	pr_err("foo_alloc success! ptr %p\n", ptr);
	return ptr;
}

static void *foo_zalloc(void)
{
	struct foo *ptr = foo_alloc();
	if (ptr) {
		memset(ptr, 0, sizeof(struct foo));
	}
	return ptr;
}

#define foo_free(ptr)					\
do {							\
	if (ptr) {					\
		kmem_cache_free(test_cachep, ptr);	\
		pr_err("foo_free ptr %p\n", ptr);	\
		ptr = NULL;				\
	}						\
} while (0)

static void slub_free(void)
{
	int i;

	pr_err("free all slub objects\n");
	for (i=0; i<OBJ_COUNT; i++) {
		if (objs[i]) {
			foo_free(objs[i]);
		}
	}
}

static int slub_test(void *args)
{
	int cnt = 0;
	int i = 0;
	struct foo *ptr = NULL;

	while (!kthread_should_stop()) {
		ptr = kmem_cache_alloc(test_cachep, GFP_KERNEL);
		if (!ptr) {
			pr_err("kmem_cache_alloc failed!\n");
			slub_free();
			break;
		}
		objs[i++] = ptr;
		pr_err("new ptr: %p\n", ptr);
		if (i % 3 == 0) {
			kmem_cache_free(test_cachep, ptr);
			objs[i-1] = NULL;
		}
		if (i == OBJ_COUNT) {
			slub_free();
			i = 0;
		}
		msleep_interruptible(1000);
		set_current_state(TASK_RUNNING);
	}
}

#if USE_ALLOC_CALLBACK
void func_ctor(struct foo *object, struct kmem_cache *cachep, unsigned long flags)
{
	static int n;
	printk(KERN_INFO "ctor fuction, count %d.\n", n++);
	object->val = 1;
}
#else

#define func_ctor NULL

#endif

static int slub_free_some_show(int num)
{
	int i, n = 0;

	for (i=0; i<num; i++) {
		if (i < 0 || i >= OBJ_COUNT)
			continue;
		foo_free(objs[i]);
		slub_cache_show(test_cachep);
		n++;
	}
	pr_err("free %d objects\n", n);
	slub_cache_show(test_cachep);
}

static int kmem_cache_create_stat_show(void)
{
	test_cachep = kmem_cache_create("test_cachep", 
		sizeof(struct foo), 0, SLAB_HWCACHE_ALIGN, func_ctor);
	if (!test_cachep) {
		pr_err("kmem_cache_create failed!\n");
		return -1;
	}
	pr_err("kmem_cache '%s(%p)' create success!\n", test_cachep->name, test_cachep);
	slub_cache_show(test_cachep);
}

static int kmem_cache_alloc_one_show(void)
{
	struct foo *ptr;

	ptr = foo_zalloc();
	if (!ptr)
		return -1;
	pr_err("alloc one object\n");
	slub_cache_show(test_cachep);
}

static int kmem_cache_alloc_some_show(int num)
{
	int i;
	struct foo *ptr;

	for (i=0; i<num; i++) {
		ptr = foo_alloc();
		if (!ptr)
			return -1;
	}
	pr_err("alloc %d objects, curr_obj_idx %d\n", num, curr_obj_idx);
	slub_cache_show(test_cachep);
}

static int run_test(void)
{
        int i;
	struct foo *ptr = NULL;

	ptr = foo_alloc();
	if (!ptr)
		return -1;
	
#if 0
	for (i=0; i<1; i++) {
		worker_task = kthread_run(slub_test,
		NULL, "slub_test", NULL);
	}
	slub_show_task = kthread_run(slub_show_thread,
		NULL, "slub_show", NULL);
#endif
}

static int __init slub_test_init(void) 
{ 
	pr_err("\n******************* slub test start *******************\n");
	pr_err("%s: current cpu %d, pid %d\n", __func__, smp_processor_id(), current->pid); 

	kmem_cache_create_stat_show();
	kmem_cache_alloc_one_show();
	kmem_cache_alloc_some_show(14);

	//slub_free_some_show(10);
	kmem_cache_alloc_some_show(5);

	return 0; 
} 
  
static void __exit slub_test_exit(void) 
{ 
	slub_free();
	if (worker_task)
		kthread_stop(worker_task);
	if (slub_show_task)
		kthread_stop(slub_show_task);
	pr_err("free kmem_cache '%s'\n", test_cachep->name);
	if (test_cachep)
		kmem_cache_destroy(test_cachep);
	test_cachep = NULL;
	pr_err("******************* slub test done *******************\n");
}


module_init(slub_test_init);
module_exit(slub_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yu Chen"); 
MODULE_DESCRIPTION("slub test!"); 
MODULE_VERSION("0.1"); 

#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/time.h>

#define EXPIRE_TIMEDOUT (30 * 1000)

static __init int hirq_off_init(void)
{
        unsigned long lastJif;

        printk("cpu%d: %s Module inits.\n", smp_processor_id(),  __func__);

	local_irq_disable();
        lastJif = jiffies;
        while (1)
        {
                if (jiffies_to_msecs(jiffies - lastJif) > EXPIRE_TIMEDOUT)
                {
                        printk("Expire time out: %d s", EXPIRE_TIMEDOUT);
                        return 0;
                }
        }

        return 0;
}

static __exit void hirq_off_exit(void)
{
        printk("%s Module exits.\n", __func__);
}

module_init(hirq_off_init);
module_exit(hirq_off_exit);

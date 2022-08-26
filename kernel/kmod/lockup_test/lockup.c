#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/time.h>

#define EXPIRE_TIMEDOUT (30 * 1000)

static __init int lockup_init(void)
{
        unsigned long lastJif;

        printk("lockup Module inits.\n");

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

static __exit void lockup_exit(void)
{
        printk("lockup Module exits.\n");
}

module_init(lockup_init);
module_exit(lockup_exit);

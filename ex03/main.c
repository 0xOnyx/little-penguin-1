/* SPDX-License-Identifier: BSD-3-Clause */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>

int do_work(int *my_int)
{
	int x;
	int y;
	int z;

	y = *my_int;

	for (x = 0; x < y; ++x)
		udelay(10);

	if (y < 10) {
		/* That was a long sleep, tell userspace about it */
		pr_info("We slept a long time!");
		z = x * y;
		return z;
	}
	return 1;
}

static int __init my_init(void)
{
	int x;

	x = 10;
	x = do_work(&x);
	return x;
}

static void __exit my_exit(void)
{

}

module_init(my_init);
module_exit(my_exit);
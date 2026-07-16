//
// Created by jdossantos on 5/22/2026.
//

#include <linux/module.h> //module_init, module_exit
#include <linux/kernel.h> //pr_info

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jerdos-s");
MODULE_DESCRIPTION("Hello world module");

static int __init   my_init(void) {
    pr_info("Hello world!\n");
    return 0;
}


static void __exit my_exit(void) {
    pr_info("Cleaning up module.\n");
}

module_init(my_init);
module_exit(my_exit);
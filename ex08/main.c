#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jerdos-s");
MODULE_DESCRIPTION("reverse module");

static ssize_t myfd_read(struct file *fp, char __user *user,
	size_t size, loff_t *offs);
static ssize_t myfd_write(struct file *fp, const char __user *user,
	size_t size, loff_t *offs);

static const struct file_operations myfd_fops = {
	.owner = THIS_MODULE,
	.read = &myfd_read,
	.write = &myfd_write
};

/*
 * Pas de const : misc_register() ecrit dans cette structure
 * (minor dynamique, this_device, chainage dans misc_list).
 */
static struct miscdevice myfd_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "reverse",
	.fops = &myfd_fops
};

static char str[PAGE_SIZE];

static int __init myfd_init(void)
{
	return misc_register(&myfd_device);
}

static void __exit myfd_cleanup(void)
{
	misc_deregister(&myfd_device);
}

static ssize_t myfd_read(struct file *fp, char __user *user,
	size_t size, loff_t *offs)
{
	char *tmp2;
	size_t len;
	size_t i;
	ssize_t res;

	len = strlen(str);
	tmp2 = kmalloc(len + 1, GFP_KERNEL);
	if (!tmp2)
		return -ENOMEM;

	for (i = 0; i < len; i++)
		tmp2[i] = str[len - i - 1];
	tmp2[len] = '\0';

	res = simple_read_from_buffer(user, size, offs, tmp2, len);
	kfree(tmp2);
	return res;
}

static ssize_t myfd_write(struct file *fp, const char __user *user,
	size_t size, loff_t *offs)
{
	ssize_t res;

	res = simple_write_to_buffer(str, PAGE_SIZE - 1, offs, user, size);
	if (res <= 0)
		return res;

	str[*offs] = '\0';
	return res;
}

module_init(myfd_init);
module_exit(myfd_cleanup);
#include <linux/miscdevice.h>
#include <linux/fs.h>

static ssize_t my_read(struct file *fp, char __user *buf, size_t count, loff_t *pod)
{
	const char login[] = "jerdos-s\n";

	return simple_read_from_buffer(buf, count,  pod, login, strlen(login));
}


static ssize_t my_write(struct file *fp, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	const char login[] = "jerdos-s";

	if (count > sizeof(login))
		return -EINVAL;

	if (copy_from_user(tmp, buf, count))
		return -EFAULT;

	tmp[count] = '\0';

	if (count > 0 && tmp[count-1] == '\n')
		tmp[count-1] = '\0';


	if (strcmp(tmp, login) != 0)
		return -EINVAL;

	return count ;
}

static struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.read = my_read,
	.write = my_write,
};

static struct miscdevice my_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "fortytwo",
	.fops = &my_fops,
};



static int __init fortytwo_init(void)
{
	return misc_register(&my_device);
}

static void __exit fortytwo_exit(void)
{
	misc_deregister(&my_device);
}

module_init(fortytwo_init);
module_exit(fortytwo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jerdos-s");
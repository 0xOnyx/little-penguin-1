#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>

#define LOGIN "jerdos-s"

static struct dentry *dir;
static char foo_buf[PAGE_SIZE];
static size_t foo_len;
static DEFINE_MUTEX(foo_lock);

/* --- id --- */
static ssize_t id_read(struct file *fp, char __user *buf,
	size_t count, loff_t *pos)
{
	return simple_read_from_buffer(buf, count, pos,
				       LOGIN "\n", strlen(LOGIN) + 1);
}

static ssize_t id_write(struct file *fp, const char __user *buf,
	size_t count, loff_t *pos)
{
	char tmp[32];

	if (count > strlen(LOGIN) + 1)
		return -EINVAL;
	if (copy_from_user(tmp, buf, count))
		return -EFAULT;
	tmp[count] = '\0';
	if (count > 0 && tmp[count - 1] == '\n')
		tmp[count - 1] = '\0';
	if (strcmp(tmp, LOGIN))
		return -EINVAL;
	return count;
}

static const struct file_operations id_fops = {
	.owner = THIS_MODULE,
	.read = id_read,
	.write = id_write,
};

/* --- jiffies --- */
static ssize_t jiffies_read(struct file *fp, char __user *buf,
			    size_t count, loff_t *pos)
{
	char tmp[32];
	int len;

	len = snprintf(tmp, sizeof(tmp), "%llu\n", (u64)jiffies);
	return simple_read_from_buffer(buf, count, pos, tmp, len);
}

static const struct file_operations jiffies_fops = {
	.owner = THIS_MODULE,
	.read = jiffies_read,
};

/* --- foo --- */
static ssize_t foo_read(struct file *fp, char __user *buf,
	size_t count, loff_t *pos)
{
	ssize_t ret;

	mutex_lock(&foo_lock);
	ret = simple_read_from_buffer(buf, count, pos, foo_buf, foo_len);
	mutex_unlock(&foo_lock);
	return ret;
}

static ssize_t foo_write(struct file *fp, const char __user *buf,
			 size_t count, loff_t *pos)
{
	ssize_t ret;

	if (count > PAGE_SIZE)
		return -EINVAL;
	mutex_lock(&foo_lock);
	ret = simple_write_to_buffer(foo_buf, PAGE_SIZE, pos, buf, count);
	if (ret > 0)
		foo_len = ret;
	mutex_unlock(&foo_lock);
	return ret;
}

static const struct file_operations foo_fops = {
	.owner = THIS_MODULE,
	.read = foo_read,
	.write = foo_write,
};

static int __init fortytwo_init(void)
{
	dir = debugfs_create_dir("fortytwo", NULL);
	if (!dir)
		return -ENOMEM;

	/* Rendre le repertoire globalement lisible et traversable (r-x pour tous) */
	d_inode(dir)->i_mode |= S_IRUGO | S_IXUGO;

	debugfs_create_file("id",	0666, dir, NULL, &id_fops);
	debugfs_create_file("jiffies",	0444, dir, NULL, &jiffies_fops);
	debugfs_create_file("foo",	0644, dir, NULL, &foo_fops);
	return 0;
}

static void __exit fortytwo_exit(void)
{
	debugfs_remove_recursive(dir);
}

module_init(fortytwo_init);
module_exit(fortytwo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jerdos-s");

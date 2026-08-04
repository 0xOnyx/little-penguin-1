// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/nsproxy.h>
#include <linux/mount.h>
#include <linux/path.h>
#include <linux/rbtree.h>
#include <linux/sched/task.h>

/*
 * struct mount et struct mnt_namespace ne sont pas exposes par les
 * headers publics : ils vivent dans fs/mount.h. Le Makefile ajoute
 * -I$(srctree)/fs pour que cet include soit resolu.
 */
#include "mount.h"

#define PROC_NAME "mymounts"

/*
 * Note : parcourir un mount namespace demande normalement de tenir
 * namespace_sem, qui n'est pas exporte aux modules. Cette lecture est
 * donc intrinsequement racy — acceptable pour l'exercice, a proscrire
 * en production (utiliser /proc/self/mountinfo a la place).
 */
static void mymounts_print(struct seq_file *s, struct mount *mnt)
{
	struct path path = {
		.mnt	= &mnt->mnt,
		.dentry	= mnt->mnt.mnt_root,
	};

	seq_printf(s, "%s ", mnt->mnt_devname ? mnt->mnt_devname : "none");
	seq_path(s, &path, " \t\n\\");
	seq_putc(s, '\n');
}

static int mymounts_show(struct seq_file *s, void *v)
{
	struct mnt_namespace *ns = init_task.nsproxy->mnt_ns;
	struct mount *mnt;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 8, 0)
	struct rb_node *node;

	/* Depuis 6.8 les mounts d'un namespace sont dans un rbtree. */
	for (node = rb_first(&ns->mounts); node; node = rb_next(node)) {
		mnt = rb_entry(node, struct mount, mnt_node);
		mymounts_print(s, mnt);
	}
#else
	/* Avant 6.8 : liste chainee intrusive. */
	list_for_each_entry(mnt, &ns->list, mnt_list)
		mymounts_print(s, mnt);
#endif
	return 0;
}

static int mymounts_open(struct inode *inode, struct file *file)
{
	return single_open(file, mymounts_show, NULL);
}

static const struct proc_ops mymounts_ops = {
	.proc_open	= mymounts_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

static int __init mymounts_init(void)
{
	if (!proc_create(PROC_NAME, 0444, NULL, &mymounts_ops))
		return -ENOMEM;

	pr_info("[mymounts] /proc/%s created\n", PROC_NAME);
	return 0;
}

static void __exit mymounts_exit(void)
{
	remove_proc_entry(PROC_NAME, NULL);
	pr_info("[mymounts] /proc/%s removed\n", PROC_NAME);
}

module_init(mymounts_init);
module_exit(mymounts_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jerdos-s");
MODULE_DESCRIPTION("List the system mount points through /proc/mymounts");

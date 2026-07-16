# Exercise 10 — Lister les points de montage via /proc/mymounts

## Objectif

Créer un module kernel qui expose la liste des points de montage du système dans `/proc/mymounts`, au format :

```
root /
sys /sys
proc /proc
run /run
dev /dev
```

Format : `<nom_du_fs> <point_de_montage>`

## Livrables

- Le code source du module (`main.c`)
- Le `Makefile`

## Concepts à connaître

### procfs

**procfs** est un système de fichiers virtuel monté sur `/proc/`. Les fichiers qu'on y crée n'existent que en mémoire — leur contenu est généré dynamiquement par le kernel à chaque lecture.

```c
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

// Créer /proc/mymounts
struct proc_dir_entry *proc_create(const char *name, umode_t mode,
                                    struct proc_dir_entry *parent,
                                    const struct proc_ops *proc_ops);

// Supprimer
void remove_proc_entry(const char *name, struct proc_dir_entry *parent);
```

### seq_file : l'interface pour les fichiers procfs multi-lignes

Pour afficher plusieurs lignes dans un fichier procfs, on utilise **seq_file**. C'est une abstraction qui gère automatiquement la pagination (si le contenu est plus grand que le buffer de lecture).

```c
#include <linux/seq_file.h>

// La fonction d'affichage — appelée pour chaque élément
static int mymounts_show(struct seq_file *m, void *v)
{
    seq_printf(m, "ligne de données\n");
    return 0;
}

// Méthode simple : single_open (idéal quand tout tient en une fois)
static int mymounts_open(struct inode *inode, struct file *file)
{
    return single_open(file, mymounts_show, NULL);
}

static const struct proc_ops mymounts_ops = {
    .proc_open    = mymounts_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};
```

### Parcourir les points de montage

Les points de montage sont dans la liste `init_task.nsproxy->mnt_ns` (le namespace de montage initial). On les parcourt via `list_for_each_entry`.

Mais l'API la plus propre passe par le **namespace de montage** et `struct mount` (défini dans `fs/mount.h`, pas exposé publiquement). Une approche plus simple utilise `iterate_mounts` ou la liste enchaînée via `list_for_each_entry`.

#### Approche avec `list_for_each_entry`

```c
#include <linux/mount.h>
#include <linux/nsproxy.h>
#include <linux/fs.h>

// Dans la fonction show :
struct vfsmount *mnt;
struct mount *m;

// init_task est la tâche init (PID 1)
// Son namespace de montage contient tous les mounts
list_for_each_entry(m, &init_task.nsproxy->mnt_ns->list, mnt_list) {
    seq_printf(s, "%s %s\n",
               m->mnt_devname,
               m->mnt.mnt_root->d_name.name);
}
```

**Attention** : `struct mount` est dans `fs/mount.h` qui n'est pas dans les headers publics. Il faudra peut-être inclure le chemin depuis les sources kernel ou redéfinir partiellement la structure.

#### Alternative : utiliser `kern_path` + `d_path`

Une autre approche consiste à utiliser `iterate_mounts()` si disponible sur ta version du kernel.

### Les structures clés

```c
// struct vfsmount (include/linux/mount.h) — la vue publique d'un mount
struct vfsmount {
    struct dentry *mnt_root;    // racine du filesystem monté
    struct super_block *mnt_sb; // super bloc
    int mnt_flags;
};

// struct mount (fs/mount.h) — la structure interne complète
struct mount {
    struct hlist_node mnt_hash;
    struct mount *mnt_parent;
    struct dentry *mnt_mountpoint;
    struct vfsmount mnt;
    // ...
    struct list_head mnt_list;  // maillon de la liste chainée
    const char *mnt_devname;    // nom du device (ex: "sysfs", "proc")
    // ...
};
```

## Squelette du module

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/nsproxy.h>
#include <linux/mount.h>
#include <linux/sched/task.h>

// fs/mount.h n'est pas un header public — on doit inclure les sources kernel
// ou utiliser une autre approche selon la version

static int mymounts_show(struct seq_file *s, void *v)
{
    struct mount *mnt;

    list_for_each_entry(mnt,
                        &init_task.nsproxy->mnt_ns->list,
                        mnt_list) {
        seq_printf(s, "%s %s\n",
                   mnt->mnt_devname,
                   mnt->mnt.mnt_root->d_name.name);
    }
    return 0;
}

static int mymounts_open(struct inode *inode, struct file *file)
{
    return single_open(file, mymounts_show, NULL);
}

static const struct proc_ops mymounts_ops = {
    .proc_open    = mymounts_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init mymounts_init(void)
{
    if (!proc_create("mymounts", 0, NULL, &mymounts_ops))
        return -ENOMEM;
    pr_info("Hello world!\n");
    return 0;
}

static void __exit mymounts_exit(void)
{
    remove_proc_entry("mymounts", NULL);
    pr_info("Cleaning up module.\n");
}

module_init(mymounts_init);
module_exit(mymounts_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ton_login");
```

## Test

```bash
sudo insmod main.ko

cat /proc/mymounts
# Affiche les points de montage

sudo rmmod main
cat /proc/mymounts   # Doit retourner "No such file or directory"
```

## Notes

- `struct mount` et `fs/mount.h` sont des détails internes du kernel. Si tu compiles contre les sources kernel (via le Makefile module standard), tu peux inclure `"../fs/mount.h"` en chemin relatif depuis les sources.
- Si `list_for_each_entry` avec `mnt_list` ne compile pas, consulte la version du kernel pour voir si la structure a changé.
- Ce module n'est **pas difficile conceptuellement**, mais trouver les bons headers et champs peut être fastidieux — c'est l'objectif de l'exercice.

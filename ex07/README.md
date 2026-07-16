# Exercise 8 — Module debugfs

## Objectif

Modifier le module Hello World (ex2) pour créer un répertoire **debugfs** nommé `fortytwo`, contenant trois fichiers virtuels : `id`, `jiffies`, et `foo`.

## Livrables

- Le code source du module
- Une preuve que le module fonctionne

## Qu'est-ce que debugfs ?

**debugfs** est un système de fichiers virtuel du kernel, monté sur `/sys/kernel/debug/`, conçu pour exposer des informations de debug sans les contraintes de sysfs ou procfs. Sa philosophie : "There are no rules."

Contrairement au misc device (ex6), debugfs ne crée pas de fichier dans `/dev/` mais dans `/sys/kernel/debug/`.

```bash
# Vérifier que debugfs est monté
mount | grep debugfs
# Si non monté :
sudo mount -t debugfs none /sys/kernel/debug/
```

Le kernel doit avoir `CONFIG_DEBUG_FS=y`.

## Les trois fichiers à créer

### `id` — identique à ex6

Comportement : lecture retourne le login, écriture vérifie le login.
Permissions : lisible et écrivable par **tous**.

### `jiffies` — lecture seule

Retourne la valeur actuelle du compteur `jiffies` du kernel.

`jiffies` est un compteur global incrémenté à chaque interruption timer (typiquement 100 ou 1000 fois par seconde selon `HZ`). Il mesure le temps depuis le boot.

Permissions : lisible par **tous**, lecture seule.

### `foo` — stockage avec verrou

- Writable seulement par **root**
- Readable par **tous**
- Stocke jusqu'à une page (`PAGE_SIZE` = 4096 bytes) de données
- La lecture retourne ce qui a été écrit
- Doit être thread-safe (locking)

Permissions : `0644` (root écrit, tous lisent).

## API debugfs

```c
#include <linux/debugfs.h>

// Créer un répertoire
struct dentry *debugfs_create_dir(const char *name, struct dentry *parent);
// parent = NULL → crée à la racine de debugfs

// Créer un fichier avec fops custom
struct dentry *debugfs_create_file(const char *name, umode_t mode,
                                    struct dentry *parent, void *data,
                                    const struct file_operations *fops);

// Créer un fichier u64 read-only (pratique pour jiffies)
void debugfs_create_u64(const char *name, umode_t mode,
                         struct dentry *parent, u64 *value);

// Supprimer récursivement un répertoire et son contenu
void debugfs_remove_recursive(struct dentry *dentry);
```

## Locking pour foo

Plusieurs processus peuvent lire/écrire simultanément. Il faut un **mutex** :

```c
#include <linux/mutex.h>

static DEFINE_MUTEX(foo_lock);

// Dans write :
mutex_lock(&foo_lock);
// ... copier les données
mutex_unlock(&foo_lock);

// Dans read :
mutex_lock(&foo_lock);
// ... lire les données
mutex_unlock(&foo_lock);
```

## Squelette du module

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>

#define LOGIN "ton_login"

static struct dentry *dir;       // /sys/kernel/debug/fortytwo/
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
    if (strcmp(tmp, LOGIN) != 0)
        return -EINVAL;
    return count;
}

static const struct file_operations id_fops = {
    .owner = THIS_MODULE,
    .read  = id_read,
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
    .read  = jiffies_read,
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
    .read  = foo_read,
    .write = foo_write,
};

/* --- init / exit --- */
static int __init fortytwo_init(void)
{
    dir = debugfs_create_dir("fortytwo", NULL);
    if (!dir)
        return -ENOMEM;

    debugfs_create_file("id",      0666, dir, NULL, &id_fops);
    debugfs_create_file("jiffies", 0444, dir, NULL, &jiffies_fops);
    debugfs_create_file("foo",     0644, dir, NULL, &foo_fops);
    return 0;
}

static void __exit fortytwo_exit(void)
{
    debugfs_remove_recursive(dir);
}

module_init(fortytwo_init);
module_exit(fortytwo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ton_login");
```

## Test

```bash
sudo insmod main.ko

# Vérifier les fichiers créés
ls /sys/kernel/debug/fortytwo/

# Test id
cat /sys/kernel/debug/fortytwo/id
echo "ton_login" > /sys/kernel/debug/fortytwo/id     # OK
echo "wrong" > /sys/kernel/debug/fortytwo/id          # EINVAL

# Test jiffies (valeur qui change à chaque lecture)
cat /sys/kernel/debug/fortytwo/jiffies
cat /sys/kernel/debug/fortytwo/jiffies

# Test foo
echo "hello" | sudo tee /sys/kernel/debug/fortytwo/foo
cat /sys/kernel/debug/fortytwo/foo    # → hello

sudo rmmod main
ls /sys/kernel/debug/fortytwo   # Ne doit plus exister
```

## Note sur les permissions du répertoire

Le sujet précise que le répertoire `fortytwo` doit être **globalement lisible**. `debugfs_create_dir` ne permet pas de choisir les permissions. Solution :

```bash
sudo chown -R ton_login /sys/kernel/debug/fortytwo
```

Ou utiliser `debugfs_create_dir` et modifier les permissions ensuite dans le code via `d_inode(dir)->i_mode`.

# Exercise 6 — Misc Character Device Driver (/dev/fortytwo)

## Objectif

Transformer le module Hello World (ex2) en un **driver de périphérique caractère misc**. Ce driver crée `/dev/fortytwo` avec les comportements suivants :

- **Lecture** : retourne ton login étudiant
- **Écriture** : compare l'entrée à ton login — succès si égal, `EINVAL` sinon

## Livrables

- Le code source du module mis à jour
- Une preuve que ça fonctionne

## Concepts à connaître

### Qu'est-ce qu'un misc device ?

Le **misc subsystem** est le moyen le plus simple de créer un **character device** sous Linux. Un character device est un fichier spécial dans `/dev/` qu'on peut lire/écrire comme un fichier normal, mais dont le contenu est généré/consommé par le kernel.

Sans misc, créer un char device implique d'enregistrer un major number, gérer sysfs, etc. Le misc subsystem fait tout ça automatiquement avec un minor number dynamique.

### L'interface misc

```c
#include <linux/miscdevice.h>
#include <linux/fs.h>

static struct miscdevice my_device = {
    .minor = MISC_DYNAMIC_MINOR,  // minor number automatique
    .name  = "fortytwo",          // crée /dev/fortytwo
    .fops  = &my_fops,
};

// Enregistrer au chargement
misc_register(&my_device);

// Désenregistrer au déchargement
misc_deregister(&my_device);
```

### Les opérations fichier (file_operations)

```c
static struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .read  = my_read,
    .write = my_write,
};
```

### Implémenter read

`read` doit copier des données du kernel vers l'userspace. On utilise `simple_read_from_buffer` ou `copy_to_user`.

```c
static ssize_t my_read(struct file *fp, char __user *buf,
                        size_t count, loff_t *pos)
{
    const char login[] = "ton_login\n";
    return simple_read_from_buffer(buf, count, pos,
                                   login, strlen(login));
}
```

`simple_read_from_buffer` gère automatiquement `pos` (l'offset) et retourne 0 en fin de données — ce qui dit au userspace qu'il n'y a plus rien à lire.

### Implémenter write

`write` reçoit des données de l'userspace. On compare avec le login.

```c
static ssize_t my_write(struct file *fp, const char __user *buf,
                         size_t count, loff_t *pos)
{
    char tmp[32];
    const char login[] = "ton_login";

    if (count > sizeof(tmp) - 1)
        return -EINVAL;

    if (copy_from_user(tmp, buf, count))
        return -EFAULT;

    tmp[count] = '\0';

    // Retirer le '\n' éventuel
    if (count > 0 && tmp[count - 1] == '\n')
        tmp[count - 1] = '\0';

    if (strcmp(tmp, login) != 0)
        return -EINVAL;

    return count;
}
```

**Pourquoi `copy_from_user` ?** Le kernel ne peut pas déréférencer directement les pointeurs userspace — c'est une violation de sécurité. `copy_from_user` vérifie que le pointeur est valide et copie les données.

### `__user`

L'annotation `__user` sur un pointeur indique aux outils d'analyse statique (sparse) que ce pointeur pointe vers de la mémoire userspace. Ne jamais le déréférencer directement.

## Structure complète du module

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#define LOGIN "ton_login"

static ssize_t fortytwo_read(struct file *fp, char __user *buf,
                              size_t count, loff_t *pos)
{
    return simple_read_from_buffer(buf, count, pos,
                                   LOGIN "\n", strlen(LOGIN) + 1);
}

static ssize_t fortytwo_write(struct file *fp, const char __user *buf,
                               size_t count, loff_t *pos)
{
    char tmp[32];

    if (count > strlen(LOGIN) + 1)
        return -EINVAL;
    if (copy_from_user(tmp, buf, count))
        return -EFAULT;
    tmp[count] = '\0';
    if (tmp[count - 1] == '\n')
        tmp[count - 1] = '\0';
    if (strcmp(tmp, LOGIN) != 0)
        return -EINVAL;
    return count;
}

static const struct file_operations fortytwo_fops = {
    .owner = THIS_MODULE,
    .read  = fortytwo_read,
    .write = fortytwo_write,
};

static struct miscdevice fortytwo_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "fortytwo",
    .fops  = &fortytwo_fops,
};

static int __init fortytwo_init(void)
{
    return misc_register(&fortytwo_dev);
}

static void __exit fortytwo_exit(void)
{
    misc_deregister(&fortytwo_dev);
}

module_init(fortytwo_init);
module_exit(fortytwo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ton_login");
```

## Test

```bash
# Charger le module
sudo insmod main.ko

# Vérifier la création du device
ls -la /dev/fortytwo

# Tester la lecture
cat /dev/fortytwo
# Attendu : ton_login

# Tester l'écriture valide
echo "ton_login" > /dev/fortytwo
echo $?   # Attendu : 0

# Tester l'écriture invalide
echo "mauvais" > /dev/fortytwo
echo $?   # Attendu : 1 (errno EINVAL)

# Décharger
sudo rmmod main
ls /dev/fortytwo  # Ne doit plus exister
```

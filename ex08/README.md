# Exercise 9 — Corriger le style et le comportement du reverse device

## Objectif

Prendre le fichier C fourni, corriger son **coding style** Linux **ET** corriger son **comportement** (bugs fonctionnels). Le fichier implémente un misc device `/dev/reverse` qui est censé inverser les chaînes écrites.

## Livrable

- Le fichier C corrigé et fonctionnel

## Le fichier original

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>

// Dont have a license, LOL
MODULE_LICENSE("LICENSE");
MODULE_AUTHOR("Louis Solofrizzo <louis@ne02ptzero.me>");
MODULE_DESCRIPTION("Useless module");

static ssize_t myfd_read(struct file *fp, char __user *user,
    size_t size, loff_t *offs);
static ssize_t myfd_write(struct file *fp, const char __user *user,
    size_t size, loff_t *offs);

static struct file_operations myfd_fops = {
    .owner = THIS_MODULE, .read = &myfd_read, .write = &myfd_write
};

static struct miscdevice myfd_device = {
    .minor = MISC_DYNAMIC_MINOR,.name = "reverse",
    .fops = &myfd_fops };

char str[PAGE_SIZE];
char *tmp;

static int __init myfd_init(void) {
    int retval;
    retval = misc_register(&(*(&(myfd_device))));
    return 1;
}

static void __exit myfd_cleanup(void) {
}

ssize_t myfd_read(struct file *fp, char __user *user,
    size_t size, loff_t *offs)
{
    size_t t, i;
    char *tmp2;
    tmp2 = kmalloc(sizeof(char) * PAGE_SIZE * 2, GFP_KERNEL);
    tmp = tmp2;
    for (t = strlen(str) - 1, i = 0; t >= 0; t--, i++) {
        tmp[i] = str[t];
    }
    tmp[i] = 0x0;
    return simple_read_from_buffer(user, size, offs, tmp, i);
}

ssize_t myfd_write(struct file *fp, const char __user *user,
    size_t size, loff_t *offs) {
    ssize_t res;
    res = 0;
    res = simple_write_to_buffer(str, size, offs, user, size) + 1;
    str[size + 1] = 0x0;
    return res;
}

module_init(myfd_init);
module_exit(myfd_cleanup);
```

## Ce que le module est censé faire

Le module crée `/dev/reverse`. Quand on écrit une chaîne, elle est inversée et peut être relue :

```bash
echo "hello" > /dev/reverse
cat /dev/reverse
# attendu : olleh
```

## Bugs fonctionnels à corriger

### Bug 1 — `myfd_init` retourne 1 au lieu de 0

```c
// MAUVAIS — retourner 1 signifie une erreur → le module ne se charge pas
return 1;

// BON
return retval;  // misc_register retourne 0 en cas de succès
```

### Bug 2 — `myfd_cleanup` ne désenregistre pas le device

```c
// MAUVAIS — fuite de ressource
static void __exit myfd_cleanup(void) {
}

// BON
static void __exit myfd_cleanup(void)
{
    misc_deregister(&myfd_device);
}
```

### Bug 3 — Fuite mémoire dans `myfd_read`

`kmalloc` alloue de la mémoire mais `kfree` n'est jamais appelé :

```c
tmp2 = kmalloc(...);
// ... utilisation ...
// kfree(tmp2) MANQUANT
return simple_read_from_buffer(...);
```

De plus, le pointeur global `tmp = tmp2` est un anti-pattern dangereux.

### Bug 4 — Boucle d'inversion incorrecte (`size_t` non signé)

```c
// MAUVAIS — t est size_t (non signé), donc t >= 0 est TOUJOURS vrai
// → boucle infinie / overflow quand t == 0 et on décrémente
for (t = strlen(str) - 1, i = 0; t >= 0; t--, i++) {

// BON — utiliser un type signé, ou reformuler
size_t len = strlen(str);
for (i = 0; i < len; i++) {
    tmp[i] = str[len - 1 - i];
}
```

### Bug 5 — `myfd_write` : `res` vaut `taille + 1`

```c
// MAUVAIS
res = simple_write_to_buffer(...) + 1;  // off-by-one

// BON
res = simple_write_to_buffer(str, PAGE_SIZE, offs, user, size);
```

### Bug 6 — `str[size + 1]` au lieu de `str[size]`

```c
// MAUVAIS — off-by-one, potentiel out-of-bounds
str[size + 1] = 0x0;

// BON
str[size] = '\0';
```

### Bug 7 — `MODULE_LICENSE("LICENSE")` invalide

```c
// MAUVAIS — "LICENSE" n'est pas une licence reconnue
MODULE_LICENSE("LICENSE");

// BON
MODULE_LICENSE("GPL");
```

## Problèmes de coding style à corriger

- Les `static` manquants sur `myfd_read` et `myfd_write`
- Déclarations dans `file_operations` sur plusieurs colonnes non alignées → une par ligne
- Accolades ouvrantes non conformes (idem ex4 : blocs sur même ligne, fonctions sur nouvelle ligne)
- Indentation avec espaces → tabulations
- `const` manquant sur `file_operations` (doit être `const struct file_operations`)
- La déréférence inutile `&(*(&(myfd_device)))` → juste `&myfd_device`
- Variables globales `str` et `tmp` devraient être `static`

## Résumé : ce que fait le module corrigé

1. Charge → crée `/dev/reverse`
2. `write` → stocke la chaîne dans `str`
3. `read` → retourne `str` inversé (sans le `\n` si tu veux être propre)
4. Décharge → supprime `/dev/reverse`, libère les ressources

## Vérification avec checkpatch.pl

```bash
scripts/checkpatch.pl --no-tree -f main.c
# Doit retourner 0 erreurs, 0 warnings
```

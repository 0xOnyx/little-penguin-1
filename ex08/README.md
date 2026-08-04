# Exercise 9 — Corriger le style et le comportement du reverse device

## Objectif

Prendre le fichier C fourni, corriger son **coding style** Linux **ET** corriger son **comportement** (bugs fonctionnels). Le fichier implémente un misc device `/dev/reverse` qui est censé inverser les chaînes écrites.

## Livrable

- Le fichier C corrigé et fonctionnel (`main.c`)

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
int retval;
retval = misc_register(&myfd_device);   // valeur ignorée
return 1;

// BON
return misc_register(&myfd_device);     // 0 en cas de succès, < 0 sinon
```

### Bug 2 — `myfd_cleanup` ne désenregistre pas le device

Le device reste dans `/dev` après `rmmod`, avec des `file_operations` pointant sur du code déchargé : `oops` garanti au premier accès.

```c
// MAUVAIS
static void __exit myfd_cleanup(void) {
}

// BON
static void __exit myfd_cleanup(void)
{
	misc_deregister(&myfd_device);
}
```

### Bug 3 — Boucle d'inversion : `size_t` est non signé

```c
// MAUVAIS
for (t = strlen(str) - 1, i = 0; t >= 0; t--, i++)
```

Deux problèmes distincts :

1. `t` est `size_t`, donc `t >= 0` est **toujours vrai**. Quand `t` atteint 0, le `t--` suivant le fait passer à `SIZE_MAX` : la boucle continue à lire hors de `str` et à écrire hors de `tmp` jusqu'au crash.
2. Si `str` est vide, `strlen(str) - 1` vaut `SIZE_MAX` dès le départ.

```c
// BON — pas de soustraction sur un compteur non signé
size_t len = strlen(str);

for (i = 0; i < len; i++)
	tmp2[i] = str[len - i - 1];
tmp2[len] = '\0';
```

### Bug 4 — Fuite mémoire + pointeur global `tmp`

`kmalloc` alloue mais `kfree` n'est jamais appelé : une fuite de 8 Ko à **chaque** `read`. De plus `tmp = tmp2` publie un pointeur vers la mémoire locale dans un global partagé entre tous les appelants — aucune raison d'exister, `tmp` doit disparaître.

Au passage, `kmalloc` peut échouer : son retour **doit** être testé, sinon c'est un déréférencement de `NULL`.

```c
// BON
tmp2 = kmalloc(len + 1, GFP_KERNEL);
if (!tmp2)
	return -ENOMEM;
```

### Bug 5 — `myfd_write` : `available` vaut `size`, contrôlé par l'utilisateur

```c
// MAUVAIS
res = simple_write_to_buffer(str, size, offs, user, size) + 1;
```

Le 2ᵉ paramètre de `simple_write_to_buffer` est la **taille du buffer destination**, pas la taille de la donnée. Passer `size` revient à dire au noyau « `str` fait autant d'octets que ce que l'utilisateur demande d'écrire » : `write(fd, buf, 100000)` écrase 100 000 octets à partir de `str`. C'est le débordement le plus grave du fichier.

Le `+ 1` est un second bug : `write(2)` doit retourner le nombre d'octets réellement consommés, pas un de plus. Un `+ 1` fait croire à l'appelant qu'il a écrit plus que la taille de son propre buffer.

```c
// BON — la borne est la taille réelle de str, moins la place du '\0'
res = simple_write_to_buffer(str, PAGE_SIZE - 1, offs, user, size);
```

### Bug 6 — `str[size + 1] = 0x0` : écriture d'un octet nul à un offset arbitraire

```c
// MAUVAIS
str[size + 1] = 0x0;

// TOUJOURS MAUVAIS — l'off-by-one est corrigé, le débordement non
str[size] = '\0';
```

`size` est le compteur passé par l'appelant à `write(2)` : il n'est borné par rien. Les deux versions ci-dessus permettent d'écrire un `0` à une distance choisie de `str`, donc de corrompre n'importe quelle donnée du noyau située après ce symbole. Et même avec un appelant honnête, une écriture de `PAGE_SIZE` octets place le terminateur sur `str[PAGE_SIZE]`, un octet trop loin.

Il faut se baser sur ce qui a **réellement** été copié, c'est-à-dire `*offs` après l'appel :

```c
// BON
res = simple_write_to_buffer(str, PAGE_SIZE - 1, offs, user, size);
if (res <= 0)
	return res;
str[*offs] = '\0';
```

Pourquoi c'est sûr : `simple_write_to_buffer` tronque `count` à `available - pos` et fait `*ppos = pos + count`. Avec `available = PAGE_SIZE - 1`, on a donc toujours `*offs <= PAGE_SIZE - 1`, soit un index valide dans un tableau de `PAGE_SIZE` octets. Le test `res <= 0` évite en plus d'écrire le terminateur quand rien n'a été copié (`-EFAULT`, `-EINVAL`, ou buffer plein).

### Bug 7 — `MODULE_LICENSE("LICENSE")` invalide

Une chaîne non reconnue marque le noyau comme *tainted* et prive le module des symboles GPL.

```c
// MAUVAIS
MODULE_LICENSE("LICENSE");

// BON
MODULE_LICENSE("GPL");
```

## Pièges classiques en corrigeant

Deux erreurs faciles à introduire pendant la correction :

### Piège A — `kfree` avant `simple_read_from_buffer`

```c
// MAUVAIS — use-after-free : tmp2 est lu après avoir été libéré
kfree(tmp2);
return simple_read_from_buffer(user, size, offs, tmp2, len);

// BON — libérer après la copie
res = simple_read_from_buffer(user, size, offs, tmp2, len);
kfree(tmp2);
return res;
```

Ajouter le `kfree` manquant (Bug 4) au mauvais endroit remplace une fuite mémoire par une faille bien plus grave. KASAN le signale en `use-after-free read`.

### Piège B — `strlen` sur la mémoire fraîchement allouée

```c
// MAUVAIS — tmp2 sort de kmalloc, son contenu est indéterminé
tmp2 = kmalloc(...);
size_t len = strlen(tmp2);

// BON — la longueur à inverser est celle de la source
size_t len = strlen(str);
```

`kmalloc` ne remet pas la mémoire à zéro (contrairement à `kzalloc`) : `strlen` part lire jusqu'à un `0` arbitraire, hors de l'allocation.

## Problèmes de coding style à corriger

- `static` manquant sur `myfd_read` et `myfd_write` (la définition perd le `static` de la déclaration)
- `static` manquant sur `myfd_fops` : symbole global inutilement exporté
- `const` manquant sur `myfd_fops` et `myfd_device` — et dans cet ordre : `static const`, pas `const static` (checkpatch : *"Move const after static"*)
- Champs d'initialiseur collés sur la même ligne (`.minor = ...,.name = ...`) → un par ligne
- Accolade fermante de `myfd_device` en fin de ligne → sur sa propre ligne
- Accolade ouvrante de fonction sur la même ligne → sur une nouvelle ligne (les blocs, eux, la gardent en fin de ligne)
- Indentation avec des espaces → tabulations
- Variables globales `str` et `tmp` non `static`
- Déréférence inutile `&(*(&(myfd_device)))` → `&myfd_device`
- Variable `retval` assignée puis jamais lue
- Accolades superflues autour d'un corps de boucle d'une seule instruction
- Commentaire `//` → style noyau `/* */`
- `#include <linux/string.h>` manquant alors que `strlen` est utilisé

## Le fichier corrigé

```c
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

static const struct miscdevice myfd_device = {
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
```

## Résumé : ce que fait le module corrigé

1. Charge → crée `/dev/reverse`
2. `write` → stocke la chaîne dans `str`, tronquée à `PAGE_SIZE - 1` octets
3. `read` → retourne `str` inversé
4. Décharge → supprime `/dev/reverse`, libère les ressources

## Vérification

### Coding style

```bash
scripts/checkpatch.pl --no-tree -f main.c
# Doit retourner 0 erreurs, 0 warnings
```

### Comportement

```bash
sudo insmod main.ko
echo $?                       # 0 — si le module ne se charge pas, revoir le Bug 1
ls -l /dev/reverse

echo "hello" > /dev/reverse
cat /dev/reverse              # attendu : \nolleh (voir la remarque ci-dessous)

# le débordement du Bug 5/6 ne doit plus rien casser
dd if=/dev/zero of=/dev/reverse bs=1M count=1 2>/dev/null
dmesg | tail                  # aucun oops, aucun rapport KASAN

sudo rmmod main
ls /dev/reverse               # doit avoir disparu
```

Compiler avec `CONFIG_KASAN=y` et `CONFIG_DEBUG_KMEMLEAK=y` rend les Bugs 3, 4 et le Piège A immédiatement visibles dans `dmesg`.

## Limites connues du fichier corrigé

Deux points hors périmètre de l'exercice, mais à connaître :

1. **Pas de verrouillage.** `str` est partagé par tous les appelants. Un `write` concurrent à un `read` peut faire calculer `strlen` sur une chaîne en cours de modification. Un `mutex` autour des deux fonctions règlerait la question.
2. **Le `\n` du shell est conservé.** `echo "hello" > /dev/reverse` stocke `hello\n`, donc la lecture renvoie `\nolleh`. Pour obtenir exactement `olleh`, il faut retirer le saut de ligne final à l'écriture (`printf` sans `\n` côté test, ou un `strip` explicite dans `myfd_write`).

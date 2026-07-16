# Exercise 4 — Corriger le coding style Linux

## Objectif

Prendre le fichier C fourni et le modifier pour qu'il respecte le **Linux kernel coding style** (défini dans `Documentation/CodingStyle`).

## Livrable

- Le fichier C corrigé (même nom que l'original)

## Le fichier original à corriger

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
int do_work(int *my_int, int retval) {
int x;
int y = *my_int;
int z;
for (x = 0; x < my_int; ++x)
{
udelay(10);
}
if (y < 10)
/* That was a long sleep, tell userspace about it */
pr_info("We slept a long time!");
z = x * y;
return z;
return 1;
}
int my_init(void)
{
int x = 10;
x = do_work(&x, x);
return x;
}
void my_exit(void)
{
}
module_init(my_init);
module_exit(my_exit);
```

## Règles du Linux coding style à appliquer

### 1. Indentation : tabulations de 8 caractères

Chaque niveau d'indentation = 1 vraie tabulation (`\t`), pas des espaces.

```c
// MAUVAIS
int foo(void)
{
    return 0;  // 4 espaces
}

// BON
int foo(void)
{
	return 0;  // 1 tabulation
}
```

### 2. Placement des accolades

Pour les fonctions : accolade ouvrante sur **sa propre ligne**.
Pour les blocs (if, for, while) : accolade ouvrante **sur la même ligne**.

```c
// Fonction
int ma_fonction(void)
{
	...
}

// Bloc if/for
if (condition) {
	...
}

for (x = 0; x < n; x++) {
	...
}
```

### 3. Espace avant la parenthèse des mots-clés

```c
// MAUVAIS
if(x) { }
for(x = 0; ...) { }

// BON
if (x) { }
for (x = 0; ...) { }
```

Pas d'espace pour les appels de fonctions :

```c
// MAUVAIS
do_work (&x, x);

// BON
do_work(&x, x);
```

### 4. Déclarer les variables en haut du bloc

```c
// MAUVAIS
int x;
x = 5;
int y;   // déclaration après une instruction

// BON
int x;
int y;
// ... ensuite les instructions
```

### 5. `__init` et `__exit` pour les fonctions init/exit

```c
static int __init my_init(void) { ... }
static void __exit my_exit(void) { ... }
```

### 6. Les fonctions doivent être `static` si non exportées

### 7. Code mort / code inaccessible

Code après un `return` = code mort. À supprimer.

```c
return z;
return 1;  // JAMAIS ATTEINT — à supprimer
```

### 8. Bug logique dans la boucle for

```c
// MAUVAIS — compare un int* à un int
for (x = 0; x < my_int; ++x)

// BON — utiliser la valeur pointée
for (x = 0; x < y; x++)
```

### 9. Commentaire sur la même ligne que le if

```c
// MAUVAIS
if (y < 10)
/* commentaire */
	pr_info("...");

// BON — le commentaire va avant ou le bloc est explicite
if (y < 10) {
	/* That was a long sleep, tell userspace about it */
	pr_info("We slept a long time!");
}
```

## Résumé des corrections à faire

| Problème | Correction |
|---|---|
| Pas d'indentation | Ajouter tabulations |
| Accolades `for` sur nouvelle ligne | Les mettre sur la même ligne |
| `x < my_int` compare ptr et int | `x < y` |
| `return 1;` après `return z;` | Supprimer |
| `my_init` et `my_exit` non `static` | Ajouter `static` |
| Pas de `__init` / `__exit` | Les ajouter |
| Commentaire mal placé par rapport au if | Le déplacer dans le bloc |

## Outil utile : checkpatch.pl

Le kernel fournit un script pour vérifier le style :

```bash
# Depuis la racine du repo kernel Linux
scripts/checkpatch.pl --no-tree -f ton_fichier.c
```

Si `checkpatch.pl` ne retourne aucune erreur, le style est correct.

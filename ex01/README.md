# Exercise 2 — Hello World Kernel Module

## Objective

Écrire un module kernel qui affiche un message dans `dmesg` au chargement et au déchargement.

Comportement attendu :

```bash
$ sudo insmod main.ko
$ dmesg | tail -1
[Wed May 13 12:59:18 2015] Hello world!

$ sudo rmmod main.ko
$ dmesg | tail -1
[Wed May 13 12:59:24 2015] Cleaning up module.
```

## Livrables

- `main.c` — le code source du module
- `Makefile` — pour compiler le module

## Concepts à connaître

### Ce qu'est un module kernel

Un module kernel (`.ko`) est un bout de code qui tourne dans le **kernel space** — pas dans le userspace comme un programme normal. Il peut être chargé/déchargé à chaud sans rebooter.

### Les deux fonctions obligatoires

```c
// Appelée au chargement : sudo insmod main.ko
static int __init my_init(void) { ... }

// Appelée au déchargement : sudo rmmod main.ko
static void __exit my_exit(void) { ... }

module_init(my_init);
module_exit(my_exit);
```

`__init` et `__exit` sont des attributs qui indiquent au kernel que ces fonctions ne sont utiles qu'une fois — il peut libérer leur mémoire ensuite.

### Afficher dans dmesg

```c
pr_info("Hello world!\n");
```

`pr_info` est l'équivalent kernel de `printf`. Le message apparaît dans `dmesg`.

### Les headers nécessaires

```c
#include <linux/module.h>   // module_init, module_exit
#include <linux/kernel.h>   // pr_info
```

### Les macros de métadonnées

```c
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ton_login");
MODULE_DESCRIPTION("Hello world module");
```

`MODULE_LICENSE("GPL")` est important — sans licence GPL, certaines fonctions kernel ne sont pas accessibles.

## Structure des fichiers

```
ex2/
├── main.c
└── Makefile
```

## Le Makefile

Un Makefile pour module kernel a une structure particulière :

```makefile
obj-m += main.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

- `-C /lib/modules/$(uname -r)/build` : utilise le système de build du kernel actuellement en cours d'exécution
- `M=$(PWD)` : indique que les sources du module sont dans le répertoire courant
- `obj-m += main.o` : déclare que `main.c` doit être compilé en module (`.ko`)

## Compilation et test

```bash
# Compiler
make

# Charger le module
sudo insmod main.ko

# Vérifier le message
dmesg | tail -1

# Décharger le module
sudo rmmod main

# Vérifier le message de nettoyage
dmesg | tail -1

# Nettoyer les fichiers compilés
make clean
```

## Compatibilité de version (indice du sujet)

Le sujet précise que le module doit compiler sur **n'importe quel système**. Cela implique de ne pas utiliser de fonctions dépréciées ou spécifiques à une version trop récente du kernel. Reste sur les APIs stables : `pr_info`, `module_init`, `module_exit`.

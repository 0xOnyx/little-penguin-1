# Exercise 5 — Chargement automatique au branchement d'un clavier USB

## Objectif

Modifier le module Hello World (ex2) pour qu'il se charge **automatiquement** lorsqu'un clavier USB est branché, via les outils hotplug du userspace (udev, systemd, etc.).

## Livrables

- Un fichier de règles udev (ex: `42.rules`)
- Le code source mis à jour du module
- Une preuve que le module se charge automatiquement

## Concepts à connaître

### Comment Linux détecte le matériel USB

Quand un périphérique USB est branché :
1. Le kernel détecte le périphérique et émet un événement **uevent**
2. `udev` (ou `mdev`/`systemd`) reçoit l'événement
3. `udev` consulte ses règles et peut déclencher le chargement d'un module

### La table USB d'un module : `MODULE_DEVICE_TABLE`

Pour qu'un module soit **automatiquement** chargé, il doit déclarer les périphériques qu'il supporte via une table de correspondance.

```c
#include <linux/usb.h>

// Déclarer les périphériques USB supportés
static const struct usb_device_id my_table[] = {
    { USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID,
                         USB_INTERFACE_SUBCLASS_BOOT,
                         USB_INTERFACE_PROTOCOL_KEYBOARD) },
    {}  // terminateur
};
MODULE_DEVICE_TABLE(usb, my_table);
```

- `USB_INTERFACE_CLASS_HID` = 3 (Human Interface Device)
- `USB_INTERFACE_SUBCLASS_BOOT` = 1 (boot interface)
- `USB_INTERFACE_PROTOCOL_KEYBOARD` = 1 (clavier)

**Ce que fait `MODULE_DEVICE_TABLE`** : embed la table dans le `.ko` de façon à ce que `depmod` puisse la lire et créer les fichiers `modules.alias`. `udev` consulte ces alias pour savoir quel module charger.

### depmod et modules.alias

**À quoi sert `depmod -a` ?**

`depmod` (*dependency* + *module*) analyse **tous** les modules `.ko` installés dans
`/lib/modules/$(uname -r)/` et reconstruit la base de données des dépendances et des
alias. L'option `-a` (*all*) traite l'ensemble des modules (au lieu d'un seul passé en
argument). Il faut être root car il écrit dans `/lib/modules/...`.

Pour chaque module, il lit :
- les **symboles** exportés / requis → il en déduit **qui dépend de qui** ;
- les **tables de périphériques** publiées via `MODULE_DEVICE_TABLE(...)` → il en extrait
  les alias périphérique → module.

Il (re)génère alors ces fichiers d'index :

| Fichier | Contenu |
|---|---|
| `modules.dep` | Graphe des dépendances (pour charger A, charger d'abord B, C…) |
| `modules.alias` | **Alias périphérique → module** (c'est là qu'atterrit notre `usb_device_id`) |
| `modules.symbols` | Correspondance symbole → module qui le fournit |
| `*.bin` | Versions binaires indexées, lues par `modprobe`/le kernel |

**Pourquoi c'est indispensable ici :**
- `modprobe` s'appuie sur `modules.dep` pour charger les dépendances dans le bon ordre
  (contrairement à `insmod`, qui charge un seul `.ko` sans rien vérifier) ;
- l'auto-chargement au branchement repose sur `modules.alias` : sans un `depmod` à jour,
  l'alias de notre module n'y figure pas et `udev` ne saura pas quel module charger.

On lance donc `sudo depmod -a` **après chaque installation/copie** d'un nouveau module.

```bash
# Après avoir installé le module, régénérer la base de données
sudo depmod -a

# Vérifier que l'alias est bien présent
cat /lib/modules/$(uname -r)/modules.alias | grep ton_module
```

### Règles udev

Un fichier de règles udev (`/etc/udev/rules.d/42.rules`) peut forcer le chargement d'un module :

```
ACTION=="add", SUBSYSTEM=="usb", ATTR{idVendor}=="????", RUN+="/sbin/modprobe ton_module"
```

Mais avec `MODULE_DEVICE_TABLE`, c'est **automatique** — udev reconnaît le périphérique et charge le module via les alias. La règle udev n'est qu'un filet de sécurité.

## Étapes

### 1. Modifier main.c pour ajouter la table USB

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>

static const struct usb_device_id my_usb_table[] = {
    { USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID,
                         USB_INTERFACE_SUBCLASS_BOOT,
                         USB_INTERFACE_PROTOCOL_KEYBOARD) },
    {}
};
MODULE_DEVICE_TABLE(usb, my_usb_table);

static int __init my_init(void)
{
    pr_info("Hello world!\n");
    return 0;
}

static void __exit my_exit(void)
{
    pr_info("Cleaning up module.\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ton_login");
MODULE_DESCRIPTION("USB keyboard hotplug module");
```

### 2. Compiler et installer le module

```bash
make
sudo cp main.ko /lib/modules/$(uname -r)/kernel/drivers/
sudo depmod -a
```

### 3. Créer la règle udev (optionnel mais recommandé)

```bash
# /etc/udev/rules.d/42.rules
ACTION=="add", SUBSYSTEM=="usb", ATTR{bInterfaceProtocol}=="01", RUN+="/sbin/modprobe main"

```

```bash
sudo udevadm control --reload-rules
```

### 4. Tester

```bash
# Brancher un clavier USB, puis vérifier
dmesg | tail -5
lsmod | grep main
```

## Structure des fichiers

```
ex5/
├── main.c
├── Makefile
└── 42.rules
```

## Notes

- Pour la preuve, un screenshot ou une copie de `dmesg` après avoir branché le clavier suffit.
- Lire **Chapter 14** de *Linux Device Drivers, 3rd Edition* (disponible gratuitement en ligne) — c'est la référence pour comprendre le hotplug USB.
- `USB_INTERFACE_CLASS_HID = 3`, `USB_INTERFACE_SUBCLASS_BOOT = 1`, `USB_INTERFACE_PROTOCOL_KEYBOARD = 1` sont définis dans `<linux/hid.h>` — il faut donc ajouter `#include <linux/hid.h>`, sinon la compilation échoue avec `undeclared here`.

# Exercise 3 — Modifier EXTRAVERSION et soumettre un patch

## Objectif

Modifier le `Makefile` du kernel Linux pour que la version du kernel en cours d'exécution contienne `-thor_kernel` dans sa chaîne de version.

Résultat attendu après reboot :

```bash
uname -r
# Exemple : 6.x.y-thor_kernel
```

## Livrables

- `boot.log` — log de démarrage du nouveau kernel
- Un **patch** du Makefile original, au format Linux (`git format-patch`)

## Pourquoi pas `make menuconfig` ?

`make menuconfig` permet de modifier `CONFIG_LOCALVERSION` dans `.config` — c'est un champ différent. Les deux s'ajoutent à la version finale, mais le sujet demande spécifiquement de modifier `EXTRAVERSION` dans le `Makefile`. De plus, le livrable est un **patch du Makefile** généré avec `git format-patch` : menuconfig modifie `.config`, pas le `Makefile`, donc ça ne produirait pas le bon patch.

| | `EXTRAVERSION` | `CONFIG_LOCALVERSION` |
|---|---|---|
| Où ? | `Makefile` du kernel | `.config` |
| Modifié par | Éditeur de texte / sed | menuconfig |
| Versionné par git | Oui → patchable | Non (fichier ignoré) |

## Concepts à connaître

### Le champ EXTRAVERSION dans le Makefile du kernel

Au sommet du `Makefile` du kernel, on trouve :

```makefile
VERSION = 6
PATCHLEVEL = X
SUBLEVEL = Y
EXTRAVERSION =
NAME = ...
```

`EXTRAVERSION` est concaténé à la version pour former le nom complet du kernel. En le mettant à `-thor_kernel`, `uname -r` retournera `6.x.y-thor_kernel`.

### Le format de patch Linux (SubmittingPatches)

Le kernel Linux a des conventions strictes pour les patches. Un patch valide :

1. Est généré avec `git format-patch`
2. A un message de commit structuré : une ligne de résumé courte, puis une description
3. Respecte le coding style (lignes < 80 chars dans le message, etc.)
4. Contient une `Signed-off-by:` line

```
Subject: [PATCH] kbuild: set EXTRAVERSION to -thor_kernel

Set EXTRAVERSION to -thor_kernel to identify this custom build.

Signed-off-by: Ton Nom <ton@email.com>
```

## Étapes

### 1. Se placer dans le repo kernel cloné en ex1

```bash
cd linux/   # le repo cloné lors de l'ex1
```

### 2. Modifier le Makefile

```bash
# Trouver la ligne EXTRAVERSION
grep "^EXTRAVERSION" Makefile
# Résultat : EXTRAVERSION =

# Modifier
sed -i 's/^EXTRAVERSION =.*/EXTRAVERSION = -thor_kernel/' Makefile

# Vérifier
grep "^EXTRAVERSION" Makefile
# Résultat attendu : EXTRAVERSION = -thor_kernel
```

### 3. Générer le patch

```bash
git add Makefile
git commit -s -m "kbuild: set EXTRAVERSION to -thor_kernel"
git format-patch HEAD~1
# Génère : 0001-kbuild-set-EXTRAVERSION-to-thor_kernel.patch
```

Le flag `-s` ajoute automatiquement la ligne `Signed-off-by:`.

### 4. Recompiler et rebooter

```bash
make -j$(nproc)
sudo make modules_install
sudo make install
sudo reboot
```

### 5. Vérifier et capturer le boot log

```bash
uname -r
# Doit afficher quelque chose contenant "thor_kernel"

dmesg > boot.log
```

## Structure des fichiers à rendre

```
ex3/
├── boot.log
└── 0001-kbuild-set-EXTRAVERSION-to-thor_kernel.patch
```

## Notes

- Ne pas modifier `VERSION`, `PATCHLEVEL` ou `SUBLEVEL` — seulement `EXTRAVERSION`.
- La `Signed-off-by:` line est **obligatoire** selon la doc `Documentation/SubmittingPatches` du kernel. Elle certifie que tu as le droit de soumettre ce code (Developer's Certificate of Origin).
- `git format-patch` produit un fichier `.patch` prêt à être envoyé par email à une mailing list — c'est le workflow réel du développement kernel.

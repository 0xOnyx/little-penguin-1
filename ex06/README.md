# Exercise 7 — Compiler et booter le kernel linux-next

## Objectif

Télécharger le kernel **linux-next** (version de développement intégrant les patches en attente de fusion), le compiler, l'installer et booter dessus.

## Livrable

- `boot.log` — log de démarrage du kernel linux-next

## C'est quoi linux-next ?

Le kernel Linux a un processus de développement bien défini :

```
Développeur → Subsystem tree → linux-next → Linus's tree → Release
```

- **linux-next** est un arbre d'intégration **quotidien** qui regroupe les branches de tous les mainteneurs de sous-systèmes. Il représente ce qui sera dans le prochain cycle de merge.
- Il est maintenu par Stephen Rothwell.
- Il est **instable par nature** — c'est son rôle : détecter les conflits et regressions avant qu'ils n'arrivent dans le kernel de Linus.

Pour en savoir plus : `Documentation/development-process/` dans les sources du kernel.

## Étapes

### 1. Cloner linux-next

```bash
# Option A — clone complet (lent, ~5 GB)
git clone https://git.kernel.org/pub/scm/linux/kernel/git/next/linux-next.git
cd linux-next

# Option B — clone léger avec tags
git clone --single-branch -b master \
    https://git.kernel.org/pub/scm/linux/kernel/git/next/linux-next.git
cd linux-next
```

### 2. Pointer sur le tag du jour

```bash
# Lister les tags disponibles (format next-YYYYMMDD)
git tag -l "next-*" | tail -5

# Checkout sur le dernier tag
git checkout next-$(date +%Y%m%d)
# Exemple : git checkout next-20240515

# Si le tag du jour n'existe pas encore, prendre le plus récent
git checkout $(git tag -l "next-*" | sort | tail -1)
```

### 3. Configurer

Comme pour ex1 :

```bash
cp /boot/config-$(uname -r) .config
make olddefconfig
```

### 4. Compiler et installer

```bash
make -j$(nproc)
sudo make modules_install
sudo make install
sudo reboot
```

### 5. Vérifier et capturer le boot log

```bash
uname -r
# Le nom contiendra "next" ou le tag du jour

dmesg > boot.log
```

## Notes

- linux-next peut avoir des bugs — c'est normal. Si le boot échoue, essaie un tag antérieur.
- Garde le répertoire linux-next : il sera réutilisé dans un exercice suivant (ex8).
- Le kernel linux-next est identifiable via `uname -r` qui affichera quelque chose comme `6.10.0-next-20240515`.

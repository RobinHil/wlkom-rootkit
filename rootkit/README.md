# rootkit - Module kernel wlkom

Module noyau Linux (`wlkom.ko`) écrit en C qui constitue la partie "victime" du projet. Une fois chargé, il se dissimule, installe sa persistance, et se connecte au programme attaquant.

## Architecture

```
src/
├── wlkom.c                    # Point d'entrée du module (init/exit)
├── connection.c               # kthread TCP + protocole de commandes
├── connection.h
├── persistence.c              # Auto-installation du module au démarrage
├── persistence.h
└── hide-rootkit/
    ├── hide_rootkit.c         # Retrait de la liste des modules (lsmod)
    ├── hide_rootkit.h
    ├── syscall_hook.c         # Hook ftrace sur read() pour masquer des lignes
    └── syscall_hook.h
```

## Fonctionnement

### Chargement (`wlkom_init`)

Au `insmod`, quatre choses se produisent dans l'ordre :

1. **Dissimulation** - `wlkom_hide_module()` retire le module de la liste kernel : `lsmod` et `/proc/modules` ne le voient plus.
2. **Hook syscall** - `wlkom_syscall_hook_init()` installe un hook ftrace sur `read()` pour masquer les lignes correspondant à des patterns dans des fichiers ciblés. Par défaut, masque `wlkom` dans `/etc/modules`.
3. **Persistance** - `wlkom_persist()` copie `wlkom.ko` dans `/lib/modules/$(uname -r)/kernel/drivers/misc/`, ajoute `wlkom` à `/etc/modules` et exécute `depmod`. Le module survivra aux redémarrages.
4. **Connexion** - un kthread `wlkom_connection` est lancé. Il tente de se connecter en TCP à l'IP/port configurés (défaut : `192.168.50.1:4444`). En cas d'échec ou de déconnexion, il retente toutes les 3 secondes.

### Protocole de commandes

Une fois connecté, le rootkit entre en boucle de lecture. Les commandes reçues sont traitées ainsi :

| Commande | Auth requise | Réponse |
| :------- | :----------: | :------ |
| `PING` | Non | `PONG` |
| `AUTH <mdp>` | Non | `OK authenticated` / `ERR bad password` |
| `HELP` | Non | `OK commands: PING AUTH HELP INFO EXEC QUIT` |
| `INFO` | Oui | `sysname=<s> release=<r> machine=<m>` (champs utsname) |
| `EXEC <cmd>` | Oui | sortie de la commande + `EXIT:<code> PWD:<path>` |
| `QUIT` | Non | `BYE` (ferme la session, reconnexion dans 3s) |
| `HIDE_ADD <pattern> <path>` | Oui | `Wlkom_response : hide line correctly added` |
| `HIDE_DEL <pattern> <path>` | Oui | `Wlkom_response : hide line correctly deleted` / `ERR pattern not found` |
| `HIDE_INFO` | Oui | liste des patterns actifs + `END` |
| `HIDE_HELP` | Oui | aide des commandes HIDE_* |
| Inconnue | Oui | `ERR unknown command` |

L'authentification est vérifiée par hash djb2 (`WLKOM_PASSWORD_HASH = 0x10a572ef`, correspond au mot de passe `wlkom`).

Toutes les communications sont XORées avec la clé partagée (`WLKOM_XOR_KEY`, défaut `"wlkom"`).

## Compilation

Depuis la VM victime (Ubuntu Focal avec headers kernel installés) :

```bash
make module
```

## Chargement

```bash
sudo insmod wlkom.ko
```

La persistance est automatiquement installée. Pour les lancements suivants (après reboot), le module se charge tout seul.

## Vérification

Le module se cachant immédiatement au chargement, `lsmod | grep wlkom` ne retourne rien. Utiliser `dmesg` :

```bash
dmesg | grep wlkom
```

Logs attendus au premier chargement :

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

## Désinstallation complète

Le module étant caché, `rmmod wlkom` échoue. Il faut redémarrer, puis supprimer manuellement la persistance :

```bash
sudo sed -i '/^wlkom$/d' /etc/modules
sudo rm /lib/modules/$(uname -r)/kernel/drivers/misc/wlkom.ko
sudo depmod
```

## Documentation

```bash
# Depuis la racine du projet (génère la doc MkDocs dans docs/build/)
make docs
```

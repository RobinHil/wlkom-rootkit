# VM Attaquante

Machine virtuelle QEMU hébergeant le programme de contrôle du rootkit.

## Caractéristiques

| Paramètre | Valeur |
| :-------- | :----- |
| Distribution | Rocky Linux 10 |
| Image | `Rocky-10-GenericCloud-Base.latest.x86_64.qcow2` |
| RAM | 2 Go |
| CPU | 2 vCPUs |
| Disque | 20 Go |
| IP | `192.168.50.1/24` |
| Accès | SSH uniquement (clé générée dans `build/id_ed25519`) |
| Port SSH hôte | `2222` (forwarded vers le port 22 de la VM) |

## Réseau

La VM utilise deux interfaces :

- **Réseau privé** : socket multicast QEMU (`mcast=230.0.0.1:1234`) partagé avec la VM victime. Forme un réseau privé `192.168.50.0/24` invisible depuis l'hôte. Le programme attaquant écoute les connexions rootkit sur `0.0.0.0:4444`.
- **Réseau SSH** : port forwarding `127.0.0.1:2222` → VM:22. Permet la connexion SSH depuis la machine hôte.

## Démarrage

Depuis la racine du projet :

```sh
make attacking

# Ou directement
make -C vms/attacking start
```

Le script lance QEMU en **arrière-plan** et affiche les informations SSH dans le terminal.

## Ce que fait le script

**Une seule fois (premier téléchargement) :**

1. Télécharge l'image Rocky Linux 10 depuis `dl.rockylinux.org`
2. Vérifie le checksum SHA256
3. Redimensionne l'image à 20 Go (`qemu-img resize`)
4. Installe `python3-pip` et `make` via `virt-customize`

**À chaque lancement :**

5. Génère une clé SSH (`build/id_ed25519`) si elle n'existe pas
6. Copie `attacking_program/` dans `/home/operator/attacking_program/`
7. Lance `make glob-install` pour (ré)installer le programme attaquant globalement (commande `rootkit` disponible partout)
8. Génère l'ISO cloud-init si elle n'existe pas
9. Démarre la VM QEMU en arrière-plan et affiche les credentials SSH

## Se connecter à la VM

Après le démarrage (attendre ~30 s). **Aucun mot de passe SSH n'est demandé** : la connexion se fait uniquement par la clé privée générée dans `build/id_ed25519` :

```sh
# Méthode recommandée
ssh -F vms/attacking/build/ssh.conf attacking

# Méthode explicite
ssh -i vms/attacking/build/id_ed25519 -p 2222 operator@127.0.0.1
```

## Prérequis hôte

```sh
sudo pacman -Sy curl coreutils openssl cdrtools qemu-img guestfs-tools qemu-full awk sed openssh
```

## Utilisation dans la VM

Une fois connecté en SSH, lancer le programme attaquant :

```sh
rootkit
```

**Premier lancement** : le programme configure interactivement les credentials (mot de passe + clé XOR), stockés dans `~/.config/wlkom/auth`.

**Lancements suivants** : seul le mot de passe est demandé.

## Reset

Pour repartir d'une image propre (supprimer `build/` et retélécharger) :

```sh
make -C vms/attacking reset
```

Après un reset, les credentials sont à reconfigurer au premier lancement de `rootkit`.

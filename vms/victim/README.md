# VM Victime

Machine virtuelle QEMU simulant le système Linux compromis, sur lequel tourne le module kernel rootkit.

## Caractéristiques

| Paramètre | Valeur |
| :-------- | :----- |
| Distribution | Ubuntu Focal (20.04 LTS) |
| Image | `focal-server-cloudimg-amd64.img` |
| RAM | 2 Go |
| CPU | 2 vCPUs |
| Disque | 20 Go |
| IP | `192.168.50.2/24` |
| Login par défaut | `victim` / `victim` |
| Mode d'affichage | Fenêtre QEMU (mode graphique) |

## Réseau

La VM utilise un **socket multicast QEMU** (`mcast=230.0.0.1:1234`) partagé avec la VM attaquante. Cela forme un réseau privé `192.168.50.0/24` entièrement invisible depuis l'hôte.

Le rootkit (`wlkom.ko`) initie les connexions sortantes vers `192.168.50.1:4444`.

## Démarrage

Depuis la racine du projet :

```sh
make victim

# Ou directement
make -C vms/victim start
```

## Ce que fait le script au premier lancement

1. Télécharge l'image Ubuntu Focal depuis `cloud-images.ubuntu.com`
2. Vérifie le checksum SHA256
3. Redimensionne l'image à 20 Go (`qemu-img resize`)
4. Configure le clavier en AZERTY et installe `gcc` et `make` via `virt-customize`
5. Copie le dossier `rootkit/` dans `/home/victim/rootkit/`
6. Génère l'ISO cloud-init (paramètres login + réseau) et lance la VM

L'image n'est téléchargée qu'une seule fois. Les lancements suivants réutilisent le fichier existant dans `build/`.

> Pour le module kernel il faut faire un make reset sur la victime puis le recompiler et réinsérer s'il change.

## Prérequis hôte

Les outils suivants doivent être installés sur la machine hôte (Arch Linux) :

```sh
sudo pacman -Sy curl coreutils openssl cdrtools qemu-img guestfs-tools qemu-full awk sed
```

## Utilisation dans la VM

Se connecter avec `victim` / `victim`, puis compiler et charger le rootkit :

```sh
cd /home/victim/rootkit
make module
sudo insmod wlkom.ko
```

Le module s'installe pour persister au redémarrage et se connecte automatiquement au programme attaquant. Voir `rootkit/README.md` pour les détails.

## Vérification du rootkit

Le module se cachant immédiatement au chargement, `lsmod` ne le montrera pas. Utiliser `dmesg` :

```sh
dmesg | grep wlkom
```

## Reset

Pour repartir d'une image propre (supprimer `build/` et retélécharger) :

```sh
# Depuis la racine du projet
make -C vms/victim reset
```

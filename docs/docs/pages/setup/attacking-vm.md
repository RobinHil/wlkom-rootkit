# VM Attaquante

La VM attaquante héberge le programme de contrôle du rootkit. Elle écoute les connexions entrantes du rootkit et permet d'envoyer des commandes.

!!! info "Ordre de lancement"
Lancez la VM attaquante **en premier**, avant la VM victime. Le programme attaquant doit écouter sur le port 4444 avant que le rootkit tente de se connecter.

---

## Choix du système d'exploitation

### Caractéristiques Imposées

Le sujet impose les contraintes suivantes pour la VM attaquante :

| Contrainte | Valeur imposée |
| :--------- | :------------- |
| Type de système | Distribution **Linux**, au libre choix |
| Environnement cible | Doit fonctionner sur le **laptop de l'école** (Arch Linux) |
| Logiciels | **Libres et open source** (*free to use*) |
| Architecture | **Linux amd64** |
| Hyperviseur | **QEMU(KVM)** obligatoire |

### Explication du choix

Nous avons choisi **Rocky Linux 10** pour les raisons suivantes :

- **Image cloud `.qcow2` disponible** : Rocky Linux fournit des images `GenericCloud` au format `.qcow2` directement utilisables avec QEMU, sans installation manuelle. Cela permet d'automatiser entièrement la mise en place de la VM via `cloud-init`.
- **Stabilité** : Rocky Linux est un clone de RHEL (Red Hat Enterprise Linux), conçu pour une stabilité maximale avec un support de 10 ans. Il est pensé pour être déployé en environnement serveur, ce qui est ici pertinent avec l'architecture des machines virtuelles, où l'attacking est assimilée à un serveur sur lequel on se connecte depuis le rootkit et pour accéder à l'attacking program.
- **Libre et open source** : Rocky Linux est un projet open source sponsorisé par la Rocky Enterprise Software Foundation, conforme à l'exigence du sujet.
- **Architecture amd64** : l'image `x86_64` correspond à l'architecture imposée.
- **Python3 disponible** : le programme attaquant est écrit en Python, Rocky Linux 10 inclut Python3 qui est installé par défaut et pip est disponible dans ses dépôts officiels (`dnf install python3-pip`), ce qui simplifie l'installation du programme.

---

## Caractéristiques

| Paramètre        | Valeur                                                           |
| :--------------- | :--------------------------------------------------------------- |
| Distribution     | Rocky Linux 10                                                   |
| Image            | `Rocky-10-GenericCloud-Base.latest.x86_64.qcow2`                 |
| RAM              | 2 Go                                                             |
| CPU              | 2 vCPUs (passthrough host)                                       |
| Disque           | 20 Go                                                            |
| IP               | `192.168.50.1/24`                                                |
| Interface réseau | `ens3`                                                           |
| Utilisateur      | `operator`                                                          |
| Authentification | SSH uniquement (clé `build/id_ed25519`, générée automatiquement) |
| Port SSH hôte    | `2222` (forwarded vers le port 22 de la VM)                      |

---

## Réseau

La VM attaquante utilise deux interfaces réseau :

- **Réseau privé** : socket multicast QEMU sur `230.0.0.1:1234`, adresse `192.168.50.1`. Invisible depuis l'hôte : sert à communiquer avec la VM victime.
- **Réseau SSH** : port forwarding `127.0.0.1:2222` → VM:22. Permet la connexion SSH depuis l'hôte.

---

## Démarrage

```bash
# Depuis la racine du projet
make attacking

# Ou directement
make -C vms/attacking start
```

Le script lance QEMU **en arrière-plan** et affiche dans le terminal les commandes SSH :

```
====== Attacking SSH ======
  ssh -F vms/attacking/build/ssh.conf attacking
  # or:
  ssh -i vms/attacking/build/id_ed25519 -p 2222 operator@127.0.0.1
```

---

## Ce que fait le script à chaque lancement

**Une seule fois (premier téléchargement) :**

1. Télécharge l'image Rocky Linux 10 depuis `dl.rockylinux.org`
2. Vérifie le checksum SHA256
3. Redimensionne l'image à 20 Go
4. Installe `python3-pip` et `make` via `virt-customize`

**À chaque lancement :**

<ol start="5">
  <li>Génère une clé SSH (<code>build/id_ed25519</code>) si elle n'existe pas encore</li>
  <li>Copie le dossier <code>attacking_program/</code> dans <code>/home/operator/attacking_program/</code></li>
  <li>Lance <code>make glob-install</code> pour (ré)installer le programme attaquant</li>
  <li>Génère l'ISO cloud-init si elle n'existe pas</li>
  <li>Démarre la VM QEMU en arrière-plan</li>
</ol>

---

## Se connecter à la VM

Une fois la VM démarrée (attendre ~30 secondes que la VM démarre), connectez-vous via SSH depuis l'hôte.

!!! warning "Pas de mot de passe SSH"
L'authentification par mot de passe est désactivée (`ssh_pwauth: false`). La clé privée générée automatiquement dans `vms/attacking/build/id_ed25519` est le **seul** moyen de se connecter. Aucun mot de passe SSH n'est demandé.

```bash
# Méthode recommandée : utilise le fichier de config SSH généré automatiquement
ssh -F vms/attacking/build/ssh.conf attacking

# Méthode explicite : équivalente
ssh -i vms/attacking/build/id_ed25519 -p 2222 operator@127.0.0.1
```

---

## Programme attaquant

Le programme attaquant est installé et mis à jour automatiquement à chaque lancement. Pour le lancer depuis la VM :

```bash
rootkit
```

**Au premier lancement**, le programme configure interactivement les credentials :

```
First run : setting up credentials.
These will be stored (hashed / encoded) in: /home/operator/.config/wlkom/auth

Choose a password:
Confirm password:
XOR key [default: wlkom]:
```

Choisissez un mot de passe et une clé XOR (appuyez sur Entrée pour garder `wlkom`). Ces informations sont stockées de façon sécurisée dans `~/.config/wlkom/auth` : elles ne sont jamais stockées en clair.

**Aux lancements suivants**, seul le mot de passe est demandé.

!!! tip "Clé XOR"
La clé XOR doit correspondre à celle du module kernel. La valeur par défaut `wlkom` est utilisée des deux côtés. Pour changer la clé après le premier lancement, voir [Chiffrement → Changer la clé XOR](../features/cipher.md#changer-la-cle-xor).

Voir [Interface CLI](../usage/cli.md) pour le détail de l'interface.

---

## Reset

Pour repartir d'une image propre (supprime `build/` et relance depuis zéro) :

```bash
make -C vms/attacking reset
```

!!! warning "Credentials après reset"
Un reset supprime la clé SSH et les données de la VM. Les credentials (`~/.config/wlkom/auth`) doivent être reconfigurés au premier lancement de `rootkit`.

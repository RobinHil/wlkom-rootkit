# Prérequis

Cette page liste tout ce qui doit être installé et configuré sur la **machine hôte** avant de pouvoir lancer les VMs.

---

## Système hôte

Le projet est développé et testé sur **Arch Linux**. Les commandes d'installation ci-dessous utilisent `pacman`.

!!! warning "Virtualisation matérielle requise"
    Le projet nécessite un CPU avec support de la virtualisation matérielle :

    - **Intel** : option `VT-x` (ou `Intel Virtualization Technology`) à activer dans le BIOS/UEFI
    - **AMD** : option `AMD-V` (ou `SVM Mode`) à activer dans le BIOS/UEFI

    Pour vérifier que la virtualisation est bien activée :
    ```bash
    grep -Ec '(vmx|svm)' /proc/cpuinfo
    ```
    Un résultat supérieur à `0` signifie que le CPU supporte KVM et que la virtualisation est activée.

---

## Étape 1 : Installer les outils système

| Outil | Paquet Arch | Utilisation |
| :---- | :---------- | :---------- |
| `qemu-system-x86_64` | `qemu-full` | Émulation des VMs |
| `qemu-img` | `qemu-img` | Création et redimensionnement des images disques |
| `virt-customize` | `guestfs-tools` | Personnalisation des images cloud |
| `mkisofs` | `cdrtools` | Création des ISOs cloud-init |
| `curl` | `curl` | Téléchargement des images cloud |
| `sha256sum` | `coreutils` | Vérification des checksums |
| `openssl` | `openssl` | Hashage des mots de passe cloud-init |
| `awk` | `awk` | Traitement de texte dans les scripts |
| `sed` | `sed` | Substitution dans les fichiers de configuration |
| `ssh-keygen` | `openssh` | Génération de la clé SSH pour la VM attaquante |

Installation en une commande :

```bash
sudo pacman -Sy qemu-full qemu-img guestfs-tools cdrtools curl coreutils openssl awk sed openssh
```

---

## Étape 2 : Activer KVM

KVM (Kernel-based Virtual Machine) est le module qui permet à QEMU d'utiliser l'accélération matérielle. Sans KVM, les VMs tournent en émulation pure et sont inutilisablement lentes.

```bash
sudo modprobe kvm
sudo modprobe kvm_intel   # sur CPU Intel
# ou
sudo modprobe kvm_amd     # sur CPU AMD
```

!!! tip "Chargement automatique au démarrage"
    Ces modules sont généralement chargés automatiquement par le système. Si `/dev/kvm` existe déjà, vous n'avez rien à faire.

Vérifiez que `/dev/kvm` existe et que votre utilisateur peut y accéder :

```bash
ls -la /dev/kvm
```

Si vous obtenez une erreur de permission :

```bash
sudo usermod -aG kvm $USER
```

**Déconnectez-vous puis reconnectez-vous** pour que le changement de groupe soit pris en compte.

---

## Étape 3 : Vérification

```bash
# Vérifie que QEMU est installé
qemu-system-x86_64 --version

# Vérifie qemu-img
qemu-img --version

# Vérifie virt-customize
virt-customize --version

# Vérifie que KVM est accessible
ls /dev/kvm
```

Si toutes ces commandes répondent sans erreur, vous êtes prêt à [lancer les VMs](../setup/attacking-vm.md).

---

## Python (optionnel sur l'hôte)

Le programme attaquant est installé **automatiquement dans la VM attaquante**. Python n'est nécessaire sur l'hôte que si vous souhaitez développer ou tester le programme attaquant en dehors de la VM.

| Prérequis | Version minimale |
| :-------- | :--------------- |
| `python3` | 3.9 |
| `pip3` | - |

```bash
sudo pacman -Sy python python-pip
```

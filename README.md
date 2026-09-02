# wlkom-rootkit

Un rootkit Linux pédagogique, écrit sous forme de **module kernel**, pour apprendre
par la pratique comment un système se laisse prendre.

Un rootkit est un programme qui se dissimule dans le système d'exploitation d'une
machine pour en garder le contrôle à distance, en restant le plus discret possible.
Celui-ci a été construit dans un but d'apprentissage : comprendre la logique interne
de Linux, la mécanique de ses modules, l'interception d'appels système, et par
extension les endroits où une défense se fait contourner.

> [!WARNING]
> Ce code est destiné à l'étude et à l'expérimentation en laboratoire fermé.
> Il s'exécute entre deux machines virtuelles isolées, jamais sur une machine
> réelle ni sur un réseau dont vous n'êtes pas responsable. Charger un module
> kernel de ce type sur un système en service est illégal dans la plupart des
> pays, et vous en êtes seul responsable. Voir [SECURITY.md](SECURITY.md).

## Ce que fait ce projet

Le projet met en scène deux machines virtuelles qui communiquent entre elles :

- **La machine victime** (Ubuntu) héberge le rootkit. Une fois chargé dans son
  kernel, celui-ci rappelle automatiquement la machine attaquante et attend des
  ordres.
- **La machine attaquante** (Rocky Linux) héberge le programme de contrôle.
  L'opérateur y tape des commandes qui sont exécutées sur la victime.

```
Machine victime                     Machine attaquante
┌─────────────────────┐             ┌──────────────────────┐
│  Rootkit (kernel)   │──TCP 4444─▶ │  Programme attaquant │
│  Se connecte seul   │             │  Interface TUI Rich  │
│  Reconnexion auto   │             │  Écoute en attente   │
└─────────────────────┘             └──────────────────────┘
```

Le tout tourne sur une **machine hôte Arch Linux** via QEMU, sur un réseau privé
entre les deux VMs.

## Fonctionnalités

| Fonctionnalité | Description |
| :------------- | :---------- |
| Connexion persistante | Le rootkit se connecte automatiquement et se reconnecte en cas de coupure |
| Persistance au redémarrage | Le rootkit survit aux redémarrages de la machine victime |
| Authentification par mot de passe | Accès protégé au programme attaquant et au rootkit |
| Exécution de commandes | Lancer des programmes sur la victime et récupérer stdout, stderr et code de sortie |
| Dissimulation du module | Le rootkit est invisible à `lsmod` et `/proc/modules` dès le chargement |
| Dissimulation de lignes | Masquage de motifs dans des fichiers ciblés, par interception du syscall `read` |
| Chiffrement XOR | Communications obfusquées avec une clé pré-partagée configurable |

## Prérequis

- Machine hôte **Arch Linux**
- QEMU et outils associés

```sh
sudo pacman -Sy qemu-full qemu-img guestfs-tools cdrtools curl coreutils openssl awk sed openssh
```

## Démarrage rapide

```sh
# 1. Lancer les deux VMs (la VM attaquante télécharge Rocky Linux, la victime Ubuntu)
make start

# 2. Attendre une trentaine de secondes que la VM attaquante démarre, puis s'y
#    connecter. Connexion par clé uniquement, aucun mot de passe SSH n'est demandé.
ssh -F vms/attacking/build/ssh.conf attacking

# Puis lancer le programme de contrôle. Au premier lancement, il demande de
# choisir un mot de passe et une clé XOR.
rootkit

# 3. Dans la VM victime (login : victim / victim), compiler et charger le rootkit
cd /home/victim/rootkit
make module
sudo insmod wlkom.ko

# 4. Dans le programme attaquant, s'authentifier auprès du rootkit
AUTH wlkom          # mot de passe rootkit par défaut
```

Le rootkit se connecte automatiquement et la bordure de l'interface passe au vert.

> [!NOTE]
> Au premier lancement, les images des VMs sont téléchargées automatiquement ; les
> lancements suivants sont immédiats. Le programme attaquant se met à jour tout
> seul sur la machine attaquante. Pour le module kernel, utilisez
> `make -C vms/victim reset` sur la victime, puis recompilez et réinsérez si le
> code a changé.

## Structure du projet

```
wlkom-rootkit/
├── rootkit/              # Module kernel Linux (C), tourne sur la VM victime
│   └── src/
├── attacking_program/    # Programme de contrôle (Python), tourne sur la VM attaquante
├── vms/
│   ├── attacking/        # Configuration et script de lancement de la VM attaquante
│   └── victim/           # Configuration et script de lancement de la VM victime
└── docs/                 # Documentation utilisateur (MkDocs Material)
    └── docs/             # Sources MkDocs (pages markdown)
```

## Documentation complète

La documentation détaillée, avec l'architecture, l'installation pas à pas et la
référence des commandes, est dans `docs/`.

```sh
# Générer la doc statique, dans docs/build/
make docs

# Servir la doc en local avec rechargement automatique, sur http://127.0.0.1:8000
make docs-serve
```

## Contributeurs

Développé par [RobinHil](https://github.com/RobinHil),
[Spaghetto784](https://github.com/Spaghetto784),
[Ulyyysse](https://github.com/Ulyyysse) et
[genmei76](https://github.com/genmei76).

## Licence

Distribué sous licence MIT, voir [LICENSE](LICENSE). Le module kernel déclare
`MODULE_LICENSE("GPL")`, nécessaire pour accéder aux symboles du kernel exportés
en GPL uniquement.

# VM Victime

La VM victime est la machine sur laquelle tourne le rootkit. Elle simule un système Linux compromis.

!!! info "Ordre de lancement"
    Lancez la VM victime **après** la VM attaquante. Le rootkit tentera de se connecter dès son chargement : le programme attaquant doit déjà écouter.

---

## Choix du système d'exploitation

### Caractéristiques Imposées

Le sujet impose les contraintes suivantes pour la VM victime :

| Contrainte | Valeur imposée |
| :--------- | :------------- |
| Type de système | Distribution **Linux**, au libre choix |
| Version du kernel | Supérieure ou égale à **5.0.0** |
| Environnement cible | Doit fonctionner sur le **laptop de l'école** (Arch Linux) |
| Logiciels | **Libres et open source** (*free to use*) |
| Hyperviseur | **QEMU(KVM)** obligatoire |

### Explication du choix

En faisant nos recherches, nous nous sommes aperçus qu'une certaine quantité de rootkits avaient été développés sur **Ubuntu** avec un kernel de version **5.4**. Ce constat nous a orientés vers ce choix pour plusieurs raisons.

**Ubuntu comme cible réaliste :**

- **Documentation abondante** : Ubuntu est l'une des distributions Linux les plus connues et utilisées. Il existe donc une documentation bien plus aboutie sur les rootkits ciblant cette distribution que pour une distribution de niche.
- **Surface d'attaque étendue** : la très forte popularité d'Ubuntu se traduit par une surface d'attaque potentiellement plus grande. Plus une distribution est répandue, plus elle représente une cible attrayante et plus le nombre de machines vulnérables est élevé.
- **Profil d'utilisateur vulnérable** : la simplicité d'Ubuntu et sa forte visibilité attirent des utilisateurs peu expérimentés techniquement. Ces utilisateurs sont plus susceptibles d'installer un rootkit sans s'en rendre compte, et moins enclins à maintenir leur système à jour : ce qui augmente la probabilité de retrouver des environnements sur des versions anciennes et non patchées.
- **Images légères disponibles** : l'existence des images **Ubuntu Server** permet de disposer d'images moins volumineuses, réduisant ainsi le temps de mise en place, et disponibles nativement au format `.img` permettant la mise en place automatisée avec `cloud-init`.

**Kernel 5.4 : un choix délibérément daté :**

- **`kallsyms_lookup_name` accessible sans patch** : à partir du kernel **5.7**, la fonction `kallsyms_lookup_name` a été retirée de l'API exportée aux modules. Sur kernel 5.4, elle est directement disponible, ce qui simplifie grandement l'interception des appels système.
- **Moins de surveillance des modules** : le kernel 5.4 est une version **LTS datant de 2019**. Les mécanismes de contrôle et de détection des modules noyau suspects y sont moins développés que dans les versions plus récentes, ce qui facilite le chargement et la persistance d'un rootkit.

---

## Caractéristiques

| Paramètre | Valeur |
| :-------- | :----- |
| Distribution | Ubuntu Focal (20.04 LTS) |
| Image | `focal-server-cloudimg-amd64.img` |
| RAM | 2 Go |
| CPU | 2 vCPUs (passthrough host) |
| Disque | 20 Go |
| IP | `192.168.50.2/24` |
| Interface réseau | `ens3` |
| Utilisateur | `victim` |
| Mot de passe | `victim` |
| Mode d'affichage | Fenêtre QEMU (mode graphique) |

---

## Réseau

La VM victime et la VM attaquante communiquent via un **socket multicast QEMU** (sans interface réseau virtuelle sur l'hôte) :

```
-netdev socket,id=net0,mcast=230.0.0.1:1234,localaddr=127.0.0.1
```

Les deux VMs partagent le même groupe multicast `230.0.0.1:1234`, formant un réseau privé `192.168.50.0/24` invisible depuis l'hôte.

---

## Démarrage

```bash
# Depuis la racine du projet
make victim

# Ou directement
make -C vms/victim start
```

!!! note "Fenêtre séparée"
    La VM victime s'ouvre dans une **fenêtre QEMU graphique** distincte. Le script tourne en arrière-plan (`&`), ce qui libère votre terminal immédiatement après le lancement.

---

## Ce que fait le script au premier lancement

1. Télécharge l'image Ubuntu Focal depuis `cloud-images.ubuntu.com`
2. Vérifie le checksum SHA256
3. Redimensionne l'image à 20 Go
4. Configure le clavier en AZERTY et installe `gcc` et `make` via `virt-customize`
5. Copie les sources du rootkit dans `/home/victim/rootkit/`
6. Génère l'ISO cloud-init avec les identifiants et la configuration réseau
7. Lance la VM QEMU en arrière-plan

!!! info "Téléchargement unique"
    L'image n'est téléchargée qu'une seule fois. Les lancements suivants réutilisent l'image existante dans `vms/victim/build/`.

---

## Se connecter à la VM

La VM démarre dans la fenêtre QEMU. Connectez-vous avec :

```
login: victim
password: victim
```

---

## Étape suivante

Une fois connectée, compilez et chargez le rootkit :

```bash
cd /home/victim/rootkit
make module
sudo insmod wlkom.ko
```

Voir [Compiler et charger le rootkit](rootkit.md) pour les détails.

---

## Contenu déployé

Le dossier `rootkit/` est copié automatiquement dans `/home/victim/rootkit/` sur la VM. Il contient les sources du module kernel prêtes à être compilées.

---

## Reset

Pour repartir d'une image propre :

```bash
make -C vms/victim reset
```

Cela supprime le dossier `vms/victim/build/` et relance depuis zéro (nouveau téléchargement non requis si l'image originale est encore dans `build/`).

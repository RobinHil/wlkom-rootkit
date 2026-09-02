# Interface CLI

Le programme attaquant est une interface en ligne de commande interactive construite avec [Rich](https://github.com/Textualize/rich). Elle tourne sur la **VM attaquante** et écoute les connexions entrantes du rootkit.

---

## Lancement

```bash
# Sur la VM attaquante
rootkit
```

Le programme vérifie d'abord les credentials locaux, puis affiche l'interface de contrôle.

---

## Workflow typique

Une session complète se déroule dans cet ordre :

1. **Lancer le programme** : `rootkit` sur la VM attaquante
2. **Saisir le mot de passe local** : celui choisi à la première configuration
3. **Attendre la connexion** : le statut passe à 🟢 CONNECTED quand le rootkit se connecte
4. **S'authentifier auprès du rootkit** : taper `AUTH <mot_de_passe_rootkit>` dans le champ de saisie
5. **Envoyer des commandes** : `PING`, `INFO`, etc.
6. **Quitter** : `stop` ou `Ctrl+C`

---

## Interface

L'interface tient en un seul panneau Rich :

```
┌─ WLKOM Attacking Program 🟢 CONNECTED 🔐 AUTH ────────────────────────┐
│ 🟢 CONNECTED | 🔐 AUTHENTICATED                                            │
│ ────────────────────────────────────────                                   │
│ Listening on 0.0.0.0:4444                                                  │
│ 🔴 STATUS: DISCONNECTED                                                    │
│ [CONNECTED] 192.168.50.2:52341                                             │
│ 🟢 STATUS: CONNECTED                                                       │
│ / >> AUTH wlkom                                                            │
│ OK authenticated                                                           │
│ 🔐 STATUS: AUTHENTICATED                                                   │
│ / >>                                                                       │
└────────────────────────────────────────────────────────────────────────────┘
```

### Barre de statut (titre du panneau Rich)

| Indicateur      | Signification                           |
| :-------------- | :-------------------------------------- |
| 🟢 CONNECTED    | Le rootkit est connecté                 |
| 🔴 DISCONNECTED | Aucun rootkit connecté                  |
| 🔐 AUTH         | Le rootkit est authentifié              |
| 🔓 NO AUTH      | Le rootkit n'est pas encore authentifié |

La bordure du panneau passe au **vert** quand un rootkit est connecté, et au **rouge** sinon.

### Zone de logs

Affiche les 17 derniers événements : connexions, déconnexions, commandes envoyées, réponses reçues, changements de statut.

### Prompt de saisie

Le prompt en bas du panneau affiche le répertoire courant et le code de retour de la dernière commande :

```
✓ /home/victim >>            # dernière commande réussie (exit code 0)
✗ 1 /home/victim >>          # dernière commande échouée (exit code 1)
/ >>                         # pas encore de commande exécutée
```

Le programme attaquant extrait automatiquement `EXIT:<code>` et `PWD:<path>` de la réponse `EXEC` pour mettre à jour ce prompt. La saisie est affichée en temps réel ; appuyez sur `Entrée` pour envoyer la commande.

---

## Authentification locale

### Premier lancement : configuration des credentials

Au tout premier lancement (pas de fichier `~/.config/wlkom/auth`), le programme lance une configuration interactive :

```
First run : setting up credentials.
These will be stored (hashed / encoded) in: /home/<user>/.config/wlkom/auth

Choose a password:
Confirm password:
XOR key [default: wlkom]:
Credentials saved. Launching...
```

- **Mot de passe** : saisi deux fois pour confirmation, hashé en Argon2id
- **Clé XOR** : utilisée pour chiffrer les communications avec le rootkit (doit correspondre à la clé du module kernel, `wlkom` par défaut)

### Lancements suivants

À chaque lancement suivant, seul le mot de passe est demandé :

```
Password (1/3):
```

- 3 tentatives maximum, puis le programme se ferme
- Algorithme de vérification : **Argon2id** (time_cost=3, memory_cost=64 Mo, parallelism=4)

### Stockage sécurisé

Les credentials sont stockés dans `~/.config/wlkom/auth` :

- Permissions `600` (lecture uniquement par le propriétaire)
- Répertoire parent `~/.config/wlkom/` en `700`
- Deux lignes encodées en base64 : hash Argon2id du mot de passe, puis clé XOR

!!! note "Deux mots de passe distincts"
Il y a deux mots de passe différents dans ce projet :

    - **Mot de passe local** : demandé au lancement, vérifié par Argon2id côté Python. Protège l'accès au programme attaquant.
    - **Mot de passe rootkit** : envoyé via la commande `AUTH`, vérifié par hash djb2 côté kernel. Protège les commandes avancées du rootkit.

    Ils sont configurés et vérifiés indépendamment.

---

## Changer le mot de passe du rootkit

La commande `HASHPASS` calcule localement le hash DJB2 du mot de passe voulu et affiche la valeur à reporter dans le code source du module kernel :

```
HASHPASS mon_nouveau_mdp
```

Le programme affiche dans les logs :

```
[HASHPASS] DJB2("mon_nouveau_mdp") = 0x4a7f3b21
[HASHPASS] #define WLKOM_PASSWORD_HASH 0x4a7f3b21
```

Il suffit ensuite de copier ce `#define` dans `rootkit/src/connection.c` à la ligne `WLKOM_PASSWORD_HASH`, puis de recompiler et de déployer sur la VM victime (voir [Changer le mot de passe du rootkit](../features/password.md#changer-le-mot-de-passe-du-rootkit) pour le workflow complet, qui implique un reboot).

!!! note "Calcul purement local"
`HASHPASS` n'envoie rien au rootkit. Le calcul se fait entièrement côté attaquant et le résultat s'affiche dans le journal.

---

## Quitter le programme

=== "Commande"
`     Commande: stop
    `

=== "Raccourci clavier"
`Ctrl+C` ou `Ctrl+D`

Les deux méthodes ferment proprement les connexions clientes avant de quitter.

---

## Support multi-clients

Le serveur TCP accepte plusieurs connexions simultanées. Chaque rootkit connecté reçoit les commandes envoyées. En pratique, un seul rootkit est attendu à la fois.

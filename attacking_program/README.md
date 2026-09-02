# Programme attaquant

Interface de contrôle du rootkit. C'est un serveur TCP en Python qui écoute les connexions du module kernel et permet d'envoyer des commandes via une TUI Rich dans le terminal.

## Architecture

```
attacking_program/
├── main.py          # Serveur TCP + TUI Rich + logique d'authentification
├── colors.py        # Constantes de couleurs pour la TUI
├── pyproject.toml   # Packaging Python (point d'entrée `rootkit`)
├── requirements.txt # Dépendances pip
└── Makefile
```

## Fonctionnement

### Authentification à deux niveaux

1. **Local (Argon2id)** : au démarrage, le programme demande le mot de passe dans le terminal. Il est vérifié contre le hash Argon2id stocké dans `~/.config/wlkom/auth`. 3 tentatives maximum.
2. **Rootkit (djb2)** : pour envoyer `AUTH <mdp>` au rootkit, la commande est saisie manuellement dans l'interface. Le rootkit vérifie de son côté avec son propre hash.

### Configuration au premier lancement

Au premier lancement, le programme détecte l'absence du fichier `~/.config/wlkom/auth` et lance une configuration interactive :

```
First run : setting up credentials.
These will be stored (hashed / encoded) in: /home/<user>/.config/wlkom/auth

Choose a password:
Confirm password:
XOR key [default: wlkom]:
```

Le mot de passe est hashé en Argon2id (`time_cost=3`, `memory_cost=64 Mo`, `parallelism=4`). Le hash et la clé XOR sont encodés en base64 et écrits dans `~/.config/wlkom/auth` (permissions `600`, répertoire parent `700`).

### Stockage des credentials

Le fichier `~/.config/wlkom/auth` contient deux lignes :

```
<base64(argon2id_hash)>
<base64(xor_key)>
```

Ce fichier n'est lisible que par l'utilisateur propriétaire (chmod 600).

### Interface TUI

La TUI Rich affiche dans un panneau unique :

- Statut de connexion : `CONNECTED` (vert) / `DISCONNECTED` (rouge)
- Statut d'authentification : `AUTHENTICATED` / `NOT AUTHENTICATED`
- Journal des 17 derniers événements (réponses rootkit, connexions, erreurs)
- Prompt de saisie en bas : `✓ <pwd> >> ` (vert si exit 0, rouge sinon)

La couleur de la bordure du panneau change selon l'état de connexion.

### Serveur TCP

- Écoute sur `0.0.0.0:4444`
- Accepte plusieurs clients simultanément (un thread par client avec timeout 5 s)
- Les données reçues sont XOR-décodées puis affichées dans le journal
- Si un client envoie `OK authenticated`, le statut d'authentification passe au vert
- Les réponses `EXEC` contenant `EXIT:<code> PWD:<path>` sont parsées pour mettre à jour le prompt

## Installation

```bash
# Avec virtualenv (recommandé)
make install    # crée .venv et installe les dépendances

# Global (sur la VM attaquante)
make glob-install
```

À l'issue de `make install`, la commande `rootkit` est disponible dans `.venv/bin/`.

## Lancement

```bash
# Avec le Makefile (installe si nécessaire et lance)
make

# Directement
.venv/bin/rootkit

# Ou globalement (après glob-install)
rootkit
```

Au premier lancement, les credentials sont configurés interactivement. Les lancements suivants demandent uniquement le mot de passe.

## Commandes disponibles dans l'interface

### Commandes locales (ne sont pas envoyées au rootkit)

| Commande | Description |
| :------- | :---------- |
| `stop` / `quit` / `exit` / `q` | Arrête le programme attaquant (Ctrl+C fonctionne aussi) |
| `LOGOUT` | Envoie `QUIT` au rootkit : ferme la session (le rootkit se reconnecte dans 3 s) |
| `HASHPASS <mdp>` | Calcule le hash DJB2 du mot de passe : affiche la valeur à mettre dans `WLKOM_PASSWORD_HASH` |
| `HELP` | Affiche la liste de toutes les commandes reconnues |

### Commandes protocole (transmises au rootkit, XORées)

| Commande | Auth rootkit requise | Description |
| :------- | :------------------: | :---------- |
| `PING` | Non | Vérifie que le rootkit répond (`PONG`) |
| `AUTH <mdp>` | Non | Authentifie la session auprès du rootkit |
| `INFO` | Oui | Retourne les infos système de la victime (`sysname`, `release`, `machine`) |
| `EXEC <cmd>` | Oui | Exécute une commande shell, le répertoire courant persiste entre les appels |
| `QUIT` | Non | Ferme la session TCP (utiliser `LOGOUT` depuis l'interface) |
| `HIDE_ADD <pattern> <filepath>` | Oui | Masquer les lignes contenant `pattern` dans `filepath` |
| `HIDE_DEL <pattern> <filepath>` | Oui | Retirer une règle de masquage |
| `HIDE_INFO` | Oui | Lister toutes les règles de masquage actives |
| `HIDE_HELP` | Oui | Afficher l'aide des commandes `HIDE_*` |

## Dépendances

| Paquet | Rôle |
| :----- | :--- |
| `rich` | TUI (panneau, Live, texte stylisé) |
| `argon2-cffi` | Hash Argon2id pour l'auth locale |

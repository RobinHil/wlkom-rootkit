# Authentification par mot de passe

---

## Vue d'ensemble

Le projet implémente **deux niveaux d'authentification indépendants** :

| Niveau      | Où                  | Algorithme | Protège quoi                                      |
| :---------- | :------------------ | :--------- | :------------------------------------------------ |
| **Local**   | Programme attaquant | Argon2id   | L'accès au programme de contrôle                  |
| **Rootkit** | Module kernel       | DJB2       | Les commandes avancées (`INFO`, `EXEC`, `HIDE_*`) |

---

## Utilisation

Les deux niveaux d'authentification se déclenchent à des moments différents et dans des endroits différents.

**Avant le lancement du TUI** (terminal de la VM attaquante) :

```bash
rootkit
# → Password (1/3): ••••••••
```

Ce mot de passe est vérifié localement (Argon2id). Si correct, le TUI démarre. Si trois tentatives échouent, le programme se ferme.

**Dans le TUI**, après connexion du rootkit (statut 🟢 CONNECTED) :

```
AUTH wlkom
```

Réponses possibles :

```
OK authenticated        ← mot de passe correct, statut passe à 🔐 AUTHENTICATED
ERR bad password        ← mot de passe incorrect
ERR unauthorized        ← commande avancée envoyée sans avoir fait AUTH d'abord
```

!!! note "Deux mots de passe indépendants"
Le mot de passe du niveau 1 (accès au TUI) et celui du niveau 2 (commande `AUTH`) sont indépendants et peuvent être différents.

---

## Authentification locale (Argon2id)

### Ce que c'est

Au démarrage du programme attaquant (`rootkit`), un mot de passe est demandé dans le terminal. Ce mot de passe est vérifié contre un hash Argon2id stocké localement dans `~/.config/wlkom/auth`. Aucune communication réseau n'a lieu à cette étape.

### Premier lancement : configuration

Au tout premier lancement (pas de fichier `~/.config/wlkom/auth`), le programme demande de créer les credentials :

```
First run : setting up credentials.
These will be stored (hashed / encoded) in: /home/operator/.config/wlkom/auth

Choose a password: ••••••••
Confirm password: ••••••••
XOR key [default: wlkom]:
Credentials saved. Launching...
```

- Le mot de passe est demandé deux fois pour confirmation
- La clé XOR est demandée (appuyer sur Entrée pour garder la valeur par défaut `wlkom`)
- Le hash Argon2id et la clé XOR sont stockés encodés en base64 dans `~/.config/wlkom/auth` (chmod 600)

### Lancements suivants

```
Password (1/3): ••••••••
```

3 tentatives maximum, puis le programme se ferme.

### Paramètres Argon2id

| Paramètre     | Valeur            |
| :------------ | :---------------- |
| `time_cost`   | 3                 |
| `memory_cost` | 65 536 Ko (64 Mo) |
| `parallelism` | 4                 |

### Stockage

Le fichier `~/.config/wlkom/auth` contient deux lignes :

```
<base64(argon2id_hash_du_mot_de_passe)>
<base64(cle_xor)>
```

Permissions : `600` (lecture uniquement par le propriétaire). Le répertoire parent `~/.config/wlkom/` est en `700`.

### Changer le mot de passe local

Supprimez le fichier de credentials et relancez le programme :

```bash
rm ~/.config/wlkom/auth
rootkit
```

Le programme reconfigure le mot de passe et la clé XOR depuis zéro.

---

## Authentification rootkit (DJB2)

### Ce que c'est

Une fois le rootkit connecté, il faut envoyer la commande `AUTH` pour déverrouiller les commandes avancées. Le rootkit vérifie le mot de passe reçu en calculant son hash DJB2 et en le comparant au hash codé en dur dans le module kernel.

### Utilisation

Dans l'interface du programme attaquant, après que le rootkit s'est connecté (🟢 CONNECTED) :

```
AUTH wlkom
```

Réponses possibles :

```
OK authenticated        ← mot de passe correct
ERR bad password        ← mot de passe incorrect
```

Une fois authentifié, le statut passe à 🔐 **AUTHENTICATED** et les commandes `INFO`, `EXEC`, `HIDE_*` deviennent disponibles.

!!! warning "Ré-authentification après déconnexion"
Si le rootkit se déconnecte et se reconnecte (après un `QUIT` ou un redémarrage), il faut renvoyer `AUTH` : la session repart de zéro.

### Changer le mot de passe du rootkit

Le hash DJB2 est codé en dur dans `rootkit/src/connection.c` :

```c
#define WLKOM_PASSWORD_HASH 0x10a572ef   // correspond au mot de passe "wlkom"
```

Pour le changer :

**Étape 1** : Dans l'interface du programme attaquant, calculez le hash du nouveau mot de passe :

```
HASHPASS mon_nouveau_mdp
```

Le programme affiche :

```
[HASHPASS] DJB2("mon_nouveau_mdp") = 0x4a7f3b21
[HASHPASS] #define WLKOM_PASSWORD_HASH 0x4a7f3b21
```

**Étape 2** : Sur la machine de développement, modifiez `rootkit/src/connection.c` :

```c
#define WLKOM_PASSWORD_HASH 0x4a7f3b21
```

**Étape 3** : Recompilez le module sur la VM victime, puis déployez-le :

```bash
# Sur la VM victime
make module

# Remplace le fichier de persistance par la nouvelle version compilée
sudo cp ~/rootkit/wlkom.ko /lib/modules/$(uname -r)/kernel/drivers/misc/wlkom.ko
sudo depmod

# Redémarre pour que le nouveau module soit chargé
sudo reboot
```

Au redémarrage, le nouveau module se charge automatiquement depuis la persistance avec le nouveau mot de passe.

!!! note "Pourquoi reboot et pas insmod ?"
Le module étant caché en mémoire une fois chargé, `sudo insmod wlkom.ko` échoue car le kernel considère que le module est déjà présent. Il faut remplacer le fichier de persistance et redémarrer.

!!! tip "Alternative : reset VM victime"
Si vous travaillez en développement, `make reset` sur la VM victime est plus simple : cela repart d'une image propre, et il suffit ensuite de recompiler et de faire `sudo insmod wlkom.ko`.

!!! note "HASHPASS est purement local"
La commande `HASHPASS` ne transmet rien au rootkit. Le calcul se fait entièrement côté programme attaquant.

---

## Résumé du flux complet

```
1. [VM attaquante] rootkit
   → "Password (1/3): " → vérifié en Argon2id local
   → Interface TUI s'affiche

2. [VM victime] sudo insmod wlkom.ko
   → Le rootkit se connecte → 🟢 STATUS: CONNECTED

3. [Interface TUI] AUTH wlkom
   → Rootkit vérifie en DJB2 → "OK authenticated"
   → Statut passe à 🔐 AUTHENTICATED

4. [Interface TUI] EXEC ls /home
   victim
   ← commande exécutée sur la victime
```

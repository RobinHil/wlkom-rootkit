# Cacher une ligne de fichier

---

## Objectif

Supprimer une ligne spécifique des résultats de lecture d'un fichier depuis l'espace utilisateur, sans modifier le fichier sur disque.

Cas d'usage immédiat : la persistance ajoute `wlkom` à `/etc/modules`. Sans cette feature, `cat /etc/modules` révèle la présence du rootkit.

```bash
cat /etc/modules    # ne doit pas afficher "wlkom"
```

---

## Implémentation

### Mise en place du hook

La première étapes de ce filtre est de récupérer l'addresse du syscall.

Cette adresse va permettre deux choses. Remplir le champs read_hook.address qui va nous permettre de configurer FTRACE (ce que l'on va voir juste apres) et de remplir le champs read_hook.original qui permet de fournir l'address du syscall à hook_read via l ' intermédiaire de orig_read.

Un hook sur le syscall `__x64_sys_read` est installé via **ftrace** au chargement du module (`wlkom_syscall_hook_init()`). Quand un processus lit un fichier , le rootkit intercepte le buffer retourné, supprime les lignes contenant le pattern ciblé si le fichier est surveillé, et retourne le buffer modifié avec la taille ajustée.

Dans read_hook.ops, on va venir insérer nos données utile pour ftrace, à savoir la fonction a executer (`ftrace_callback`) et des flags.

Ces flags permettent d'une part de sauvegarder les registres pour avoir des données à passer à notre fonction et d'autre part de pouvoir modifier ces dit registres.

La première fonction lié à ftrace (`ret = ftrace_set_filter_ip(&read_hook.ops, read_hook.address, 0, 0);`) permet comme son nom l'indique de filtrer le déclenchement du callback que quand l addresse de sys_read est détecté.

La deuxième fonction ancre définitivement le hook en place.

La fonction ftrace_callback est celle qui permet la redirection vers notre fonction hook_read en modifiant le registre regs->ip (d'ou l'importance des FLAGS !)

### Fonction de filtre

La fonction de filtre (`hook_read`) recupère les registres depuis ftrace pour notamment récupérer l'addresse du syscall.

Le buffer de l'utilisateurs va être copié dans le kernel space pour pouvoir être manipulé.

Grace au registres, on va également récupérer les informations du fichier qui est lu (notamment son chemin !)

Ensuite, on va trié les appelles de sysread. Si le chemin du fichier qui est read correspond a un element de notre liste chainé ET que le pattern est détecté dans le buffer alors on va lire l'entrée ligne par ligne et supprimer la ligne  du buffer qui contient le pattern avant de retourner le buffer modifié.

Si le chemin et / ou le pattern est incorrect, on retourne juste le résultat du syscall.

### Structure de données

```c
struct line_hidded {
    char *str_pattern;      // sous-chaîne à masquer
    char *path;             // chemin du fichier surveillé
    struct line_hidded *next;
};
```

Les patterns sont stockés dans une liste chaînée globale `hide_list`.

### Initialisation automatique

À l'init, le module enregistre automatiquement un masquage de `"wlkom"` dans `"/etc/modules"` : la ligne ajoutée par la persistance est ainsi invisible immédiatement, sans aucune action manuelle.

## Utilisation

### Prérequis

Le rootkit doit être connecté et authentifié. Dans le programme attaquant :

```
AUTH wlkom
```

### Masquer une ligne

```
HIDE_ADD <pattern> <chemin_du_fichier>
```

- `<pattern>` : sous-chaîne à masquer (toute ligne la contenant sera supprimée des lectures)
- `<chemin_du_fichier>` : chemin absolu du fichier à surveiller

!!! warning "Le pattern ne doit pas contenir d'espace"
Le premier token après `HIDE_ADD ` est le pattern, le reste est le chemin.

Exemples concrets :

```
HIDE_ADD wlkom /etc/modules
HIDE_ADD secretpassword /etc/shadow
HIDE_ADD wlkom.ko /proc/modules
```

Réponse attendue :

```
Wlkom_response : hide line correctly added
```

### Lister les patterns actifs

```
HIDE_INFO
```

Réponse :

```
pattern=wlkom path=/etc/modules
END
```

### Supprimer un masquage

```
HIDE_DEL <pattern> <chemin_du_fichier>
```

```
HIDE_DEL wlkom /etc/modules
```

Réponse attendue :

```
Wlkom_response : hide line correctly deleted
```

Si le pattern existe (dans les msg de logs vm-victime) :

```
hook: pattern detected, line hidden
```

---

## Vérification

Après `HIDE_ADD wlkom /etc/modules`, depuis la VM victime :

```bash
cat /etc/modules
```

La ligne `wlkom` n'apparaît pas, même si elle est bien présente dans le fichier sur disque. Pour confirmer que le fichier n'est pas modifié :

```bash
sudo xxd /etc/modules | grep wlkom
```

La ligne est visible dans le dump binaire mais pas dans la sortie de `cat`.

---

## Cas d'usage typiques

| Fichier             | Pattern                 | But                                                                       |
| :------------------ | :---------------------- | :------------------------------------------------------------------------ |
| `/etc/modules`      | `wlkom`                 | Masquer la persistance (configuré automatiquement au chargement)          |
| Tout fichier de log | Ligne contenant `wlkom` | Effacer les traces d'activité                                             |

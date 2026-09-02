# Documentation fonctionnelle

Documentation utilisateur du projet wlkom-rootkit, générée avec [MkDocs Material](https://squidfunk.github.io/mkdocs-material/). Elle couvre l'architecture, l'installation, l'utilisation et le détail de chaque fonctionnalité.

## Structure

```
docs/
├── mkdocs.yml
├── requirements.txt
└── docs/                             # Sources MkDocs
    ├── index.md                      # Page d'accueil (architecture + tableau des fonctionnalités)
    └── pages/
        ├── prerequisites/
        │   └── prerequisites.md      # Dépendances hôte requises
        ├── setup/
        │   ├── victim-vm.md          # Configuration et démarrage de la VM victime
        │   ├── attacking-vm.md       # Configuration et démarrage de la VM attaquante
        │   └── rootkit.md            # Compilation et chargement du module kernel
        ├── usage/
        │   ├── cli.md                # Description de l'interface TUI
        │   ├── connection.md         # Flux de connexion et d'authentification
        │   └── commands.md           # Référence des commandes du protocole
        └── features/
            ├── persistence.md        # Mécanisme de persistance au redémarrage
            ├── password.md           # Authentification par mot de passe
            ├── hide-module.md        # Cacher le module de lsmod
            ├── hide-line.md          # Cacher une ligne dans un fichier
            └── cipher.md             # Chiffrement des communications
```

## Génération

```bash
# Depuis ce dossier
make build

# Depuis la racine du projet
make docs
```

## Développement local

```bash
pip install -r requirements.txt
mkdocs serve
```

Le site est disponible sur `http://127.0.0.1:8000` avec rechargement automatique.

## Configuration

Le fichier `mkdocs.yml` configure :

- Thème Material en français avec navigation par onglets
- Extensions Markdown : admonitions, Mermaid, onglets, highlight, tasklist
- Plugin de minification HTML

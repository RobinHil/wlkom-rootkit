# Cadre d'utilisation

Ce dépôt contient un rootkit fonctionnel. Il est publié dans un but
d'apprentissage : comprendre comment un module kernel se dissimule, comment un
appel système s'intercepte, et par quels chemins un système se laisse prendre.
Comprendre une technique offensive est ce qui permet de la détecter et de s'en
défendre.

## Ce pour quoi ce code est fait

- L'étude du fonctionnement interne de Linux et de ses modules kernel.
- L'expérimentation en laboratoire fermé, entre les deux machines virtuelles
  fournies par ce dépôt, sur un réseau privé qui ne sort pas de la machine hôte.
- L'entraînement à la détection : observer ce que le rootkit masque, et chercher
  ce qui le trahit malgré tout.

## Ce pour quoi il n'est pas fait

- Toute exécution sur une machine réelle, en service, ou sur un système dont
  vous n'avez pas la responsabilité.
- Tout réseau que vous ne possédez pas, ou pour lequel vous n'avez pas
  d'autorisation écrite.

Charger ce module sur un système tiers constitue une intrusion et une atteinte à
un système de traitement automatisé de données. En France, les articles 323-1 et
suivants du code pénal la répriment ; des dispositions équivalentes existent dans
la plupart des pays. Vous êtes seul responsable de l'usage que vous en faites.

## Limites assumées

Ce code est un support d'apprentissage, pas un outil durci. Il n'a pas vocation
à être robuste, discret face à un EDR, ni sûr :

- Le chiffrement est un XOR à clé pré-partagée. Ce n'est pas de la cryptographie,
  seulement de l'obfuscation, et cela ne résiste à aucune analyse sérieuse.
- Le mot de passe du rootkit est comparé à une empreinte DJB2, une fonction de
  hachage non cryptographique, choisie pour rester lisible dans du code kernel.
- Un module kernel bogué fait tomber la machine entière. Attendez-vous à des
  paniques kernel en modifiant le code, et travaillez sur une VM jetable.

## Signaler un problème

Ce dépôt n'a pas de processus de divulgation coordonnée : il n'est déployé nulle
part, et il n'y a donc rien à compromettre. Pour un bug, une erreur dans la
documentation ou une correction, ouvrez une issue ou une pull request.

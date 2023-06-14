# CompréC
Ceci est un projet scolaire réalisé par :
- Salma ALLAY ([@Saousa](https://github.com/Saousa))
- Alexi LOREAUX ([@alexilrx](https://github.com/alexilrx))
- Flavien FOUQUERAY ([@ffouqueray](https://github.com/ffouqueray))
--------------
## Utilité de cet outil
Cet outil a pour principale utilité de pouvoir ouvrir les fichiers zip, en proposant une posibilité de passer un mot de passe, en proposant une attaque par brute force ou une attaque par dictionnaire.

Nous avons implémenté la possibilité d'activer un mode verbeux, l'ajout de fichier dans cette archive, la possibilité de supprimer un ou plusieurs fichiers de l'archive, et la possibilité d'exporter ces fichiers hors de l'archive.

Tout ceci est possible via un "shell" interactif recréé en C. Une commande help a été ajoutée au programme afin de donner des exemples ainsi que la liste des commandes.

--------------
## Comment executer notre outil
### Pour Windows
- Installer MSYS2 Mingw64 [ici](https://www.msys2.org/)
- Dans l'outil en ligne de commande MSYS2 MINGW64, taper les commandes suivantes :
  - `pacman -S --needed base-devel mingw-w64-x86_64-toolchain`
  - `pacman -S mingw-w64-x86_64-libzip`
  - `pacman -S mingw-w64-x86_64-pkg-config`
- Ajouter le répertoire `C:\msys64\mingw64\bin` au PATH de votre ordinateur
- Compiler le code source en faisant la commande suivante :
  - `gcc fonctions\*.c main.c -lzip -o main.exe`
---
### Pour Linux (exemple avec Ubuntu)
- Tapez la liste de commande suivante :
  - `sudo apt update && sudo apt upgrade`
  - `sudo apt install build-essential libzip-dev -y`
- Compiler le code source en faisant la commande suivante :
  - `gcc fonctions/*.c main.c -lzip -o main.exe`
--------------
## Bug connu 
- Il n'est pas possible de modifier l'archive (import et rm) depuis une archive chiffrée. C'est une limitation de la librairie libzip


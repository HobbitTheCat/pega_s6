#  Pegasus Game Engine (Pega_S)

**Pegasus** est un moteur de jeu réseau asynchrone haute performance développé en **C pur (C11)**. Conçu initialement pour le jeu de cartes "6 qui prend", ce projet constitue une infrastructure générique capable de gérer des milliers de connexions simultanées grâce à une architecture entièrement **orientée événements (EDA)**.

* Consultez le [Compte Rendu](https://github.com/HobbitTheCat/pega_s6/blob/main/Pegasus_PitonLeo_SemenovEgor.pdf).
* Consultez le [Rapport Technique](https://github.com/HobbitTheCat/pega_s6/blob/main/Pegasus_technical_PitonLeo_SemenovEgor.pdf) pour plus de détails.

## Architecture du Système

Le moteur repose sur une séparation stricte des couches pour garantir une scalabilité maximale et une isolation des pannes.

### 1. Noyau Réseau & Multiplexage
* **I/O Non-bloquantes** : Utilisation du mécanisme **epoll** en mode `EPOLLET` (Edge-Triggered) pour traiter un grand nombre de clients sans attente active.
* **Gestionnaire de Connexion** : Séparation entre l'entité de transport (`server_conn_t`) et l'entité logique du joueur (`server_player_t`), permettant des **reconnexions sécurisées** via tokens sans perte d'état.
* **Protocole Binaire** : Standardisation des échanges via un format compact (En-tête fixe + Payload variable) pour optimiser la bande passante et la latence.

### 2. Communication Inter-Threads (IPC) High-Speed
* **Bus SPSC Lock-free** : Communication unidirectionnelle entre le noyau réseau et les sessions via des buffers circulaires sans verrou, basés sur les atomiques C11 (`memory_order_release` / `acquire`).
* **Logging Asynchrone MPSC** : Système de journalisation déporté dans un thread dédié pour éviter tout blocage du chemin critique (I/O Wait) lors des écritures disque.

### 3. Moteur de Session (Game Container)
* **Isolation des Threads** : Chaque session de jeu s'exécute de manière autonome dans son propre thread (`pthread_detach`), gérant ses propres ressources et son cycle de vie.
* **Machine à États (FSM)** : Logique de jeu pilotée par une machine à états finis, assurant l'intégrité des séquences (Distribution, Attente des coups, Résolution).



---

## Intelligence Artificielle (Bots)
Le moteur intègre un module de bots autonomes agissant comme des clients réels. Trois niveaux de difficulté sont implémentés :
* **Niveau 1 (Heuristique)** : Évaluation locale et choix de la carte à pénalité minimale.
* **Niveau 2 (Monte-Carlo)** : Simulation de l'état courant sur 1000 itérations pour évaluer le coup le plus avantageux.
* **Niveau 3 (Deep Strategic)** : Simulation approfondie prenant en compte l'influence du coup sur plusieurs tours ultérieurs.

---

## Interface Utilisateur (TUI)
Le moteur inclut un module d'affichage avancé pour terminal :
* **Slice Rendering** : Algorithme permettant un rendu horizontal des cartes (tranche par tranche) pour contourner la contrainte d'affichage ligne par ligne des terminaux.
* **Sémantique ANSI** : Code couleur dynamique basé sur le "degré de danger" des cartes (pénalités) et utilisation des caractères Unicode Box-Drawing.

---

## Performances & Stress Test
Le système a été validé par des tests de charge intensifs :
* **Capacité** : Support stable de **2000 bots** répartis sur **500 sessions** simultanées.
* **Efficacité** : Faible consommation CPU grâce à l'absence de commutations de contexte inutiles et à l'usage d'I/O asynchrones.

---

## Stack Technique
* **Langage** : C (C11) 
* **Primitives Système** : Epoll, EventFD, Poll, Pthreads
* **Build** : Makefile, Scripts Bash & Awk 
* **Debug** : Valgrind, GDB, Logs structurés 

---

## Auteurs
* [Semenov Egor](https://github.com/HobbitTheCat) 
* [Piton Leo](https://github.com/Badoux17) 
* *Réalisé dans le cadre de la L3 Informatique - Université de Bourgogne (2025)* 

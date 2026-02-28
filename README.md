#  Pegasus Game Engine (Pega_S)

[cite_start]**Pegasus** est un moteur de jeu réseau asynchrone haute performance développé en **C pur (C11)**[cite: 53, 655]. [cite_start]Conçu initialement pour le jeu de cartes "6 qui prend", ce projet constitue une infrastructure générique capable de gérer des milliers de connexions simultanées grâce à une architecture entièrement **orientée événements (EDA)**[cite: 651, 715].



## Architecture du Système

[cite_start]Le moteur repose sur une séparation stricte des couches pour garantir une scalabilité maximale et une isolation des pannes[cite: 656, 909].

### 1. Noyau Réseau & Multiplexage
* [cite_start]**I/O Non-bloquantes** : Utilisation du mécanisme **epoll** en mode `EPOLLET` (Edge-Triggered) pour traiter un grand nombre de clients sans attente active[cite: 131, 192].
* [cite_start]**Gestionnaire de Connexion** : Séparation entre l'entité de transport (`server_conn_t`) et l'entité logique du joueur (`server_player_t`), permettant des **reconnexions sécurisées** via tokens sans perte d'état[cite: 223, 242].
* [cite_start]**Protocole Binaire** : Standardisation des échanges via un format compact (En-tête fixe + Payload variable) pour optimiser la bande passante et la latence[cite: 64, 973].

### 2. Communication Inter-Threads (IPC) High-Speed
* [cite_start]**Bus SPSC Lock-free** : Communication unidirectionnelle entre le noyau réseau et les sessions via des buffers circulaires sans verrou, basés sur les atomiques C11 (`memory_order_release` / `acquire`)[cite: 48, 54].
* [cite_start]**Logging Asynchrone MPSC** : Système de journalisation déporté dans un thread dédié pour éviter tout blocage du chemin critique (I/O Wait) lors des écritures disque[cite: 477, 482].

### 3. Moteur de Session (Game Container)
* [cite_start]**Isolation des Threads** : Chaque session de jeu s'exécute de manière autonome dans son propre thread (`pthread_detach`), gérant ses propres ressources et son cycle de vie[cite: 356, 386].
* [cite_start]**Machine à États (FSM)** : Logique de jeu pilotée par une machine à états finis, assurant l'intégrité des séquences (Distribution, Attente des coups, Résolution)[cite: 455, 947].



---

## 🤖 Intelligence Artificielle (Bots)
[cite_start]Le moteur intègre un module de bots autonomes agissant comme des clients réels[cite: 604, 611]. Trois niveaux de difficulté sont implémentés :
* [cite_start]**Niveau 1 (Heuristique)** : Évaluation locale et choix de la carte à pénalité minimale[cite: 617, 619].
* [cite_start]**Niveau 2 (Monte-Carlo)** : Simulation de l'état courant sur 1000 itérations pour évaluer le coup le plus avantageux[cite: 621, 622].
* [cite_start]**Niveau 3 (Deep Strategic)** : Simulation approfondie prenant en compte l'influence du coup sur plusieurs tours ultérieurs[cite: 625, 626].

---

## 🎨 Interface Utilisateur (TUI)
Le moteur inclut un module d'affichage avancé pour terminal :
* [cite_start]**Slice Rendering** : Algorithme permettant un rendu horizontal des cartes (tranche par tranche) pour contourner la contrainte d'affichage ligne par ligne des terminaux[cite: 585, 588].
* [cite_start]**Sémantique ANSI** : Code couleur dynamique basé sur le "degré de danger" des cartes (pénalités) et utilisation des caractères Unicode Box-Drawing[cite: 579, 598].

---

## 📊 Performances & Stress Test
Le système a été validé par des tests de charge intensifs :
* [cite_start]**Capacité** : Support stable de **2000 bots** répartis sur **500 sessions** simultanées[cite: 649, 650].
* [cite_start]**Efficacité** : Faible consommation CPU grâce à l'absence de commutations de contexte inutiles et à l'usage d'I/O asynchrones[cite: 651, 917].

---

## 🛠️ Stack Technique
* [cite_start]**Langage** : C (C11) [cite: 53]
* [cite_start]**Primitives Système** : Epoll, EventFD, Poll, Pthreads [cite: 131, 356, 549, 987]
* [cite_start]**Build** : Makefile, Scripts Bash & Awk [cite: 691, 1005]
* [cite_start]**Debug** : Valgrind, GDB, Logs structurés [cite: 470, 935]

---

## 👥 Auteurs
* [cite_start]**Léo Piton** [cite: 5]
* [cite_start]**Egor Semenov** [cite: 5]
* [cite_start]*Réalisé dans le cadre de la L3 Informatique - Université de Bourgogne (2025)* [cite: 1, 6]

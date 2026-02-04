# Application CAO Qt_OCC

Qt_OCC est une application de CAO de bureau développée en utilisant la puissance du framework Qt pour l'interface utilisateur et OpenCASCADE Technology (OCCT) comme noyau géométrique. Elle vise à fournir une plate-forme pour la modélisation et la visualisation 3D, intégrant une gamme de fonctionnalités pour la création de formes, la manipulation et l'analyse.

## ✨ Fonctionnalités

### 📂 Gestion de Fichiers
*   **Entrée/Sortie** : Prise en charge de la création de nouveaux projets, de l'ouverture de fichiers existants, de la sauvegarde et de l'exportation de modèles.

### 🎥 Visualisation & Navigation
*   **Vues Standard** : Basculez rapidement entre les vues Isométrique, Dessus, Dessous, Gauche, Droite, Avant et Arrière.
*   **Modes d'Affichage** : Basculez entre les styles de rendu Fil de fer (Wireframe), Ombré (Shaded) et Ombré avec Arêtes.
*   **Contrôle de la Vue** : Ajustement automatique de la vue au contenu de la scène.

### 🛠️ Outils de Modélisation
*   **Primitives 2D** : Créez des Points, Lignes, Rectangles, Cercles, Arcs, Ellipses, Polygones, Courbes de Bézier et Courbes NURBS.
*   **Primitives 3D** : Générez des Boîtes, Sphères, Cylindres, Cônes et Pyramides.
*   **Opérations Booléennes** : Effectuez des opérations d'Union (Fuse), d'Intersection (Common) et de Différence (Cut) sur des solides.
*   **Modifications** :
    *   **Miroir** : Reflétez des objets par Plan ou par Axe.
    *   **Motif** : Créez des motifs Linéaires et Circulaires.
    *   **Coque (Shell)** : Convertissez des solides en structures coques.

### 📏 Mesure & Analyse
*   **Mesures** : Outils précis pour mesurer la Distance, la Longueur d'arête, la Longueur d'arc/Rayon et les Angles.
*   **Analyse** : Vérification des interférences entre les objets.
*   **Visualisation d'Assemblage** : Vue éclatée pour inspecter les composants internes.
*   **Coupe** : Appliquez des plans de coupe pour des vues en coupe transversale.

### ⚙️ Utilitaires & Personnalisation
*   **Transformation** : Outils interactifs pour déplacer, faire pivoter et mettre à l'échelle des objets.
*   **Plan de Travail** : Aide au système de coordonnées configurable.
*   **Filtres de Sélection** : Filtrez la sélection par sommet, arête, face ou solide.
*   **Support de Thème** : Basculez entre les thèmes Clair et Sombre.
*   **Localisation** : Support multilingue.

## 🛠️ Stack Technologique

*   **C++** : La logique centrale de l'application est écrite en C++ moderne (standard C++17).
*   **Qt Framework** : Utilisé pour toute l'interface graphique (GUI), utilisant SARibbon pour une barre d'outils moderne de style ruban.
*   **OpenCASCADE Technology (OCCT)** : Le puissant noyau de modélisation géométrique 3D open-source qui gère toutes les opérations CAO.
*   **CMake** : Le système de construction multiplateforme utilisé pour configurer et construire le projet.

## 🚀 Démarrage

Suivez ces instructions pour obtenir une copie du projet et l'exécuter sur votre machine locale à des fins de développement et de test.

### Prérequis

Vous aurez besoin d'installer les logiciels suivants sur votre système :

*   **Un compilateur compatible C++17** :
    *   Windows : MSVC 2019 ou plus récent
*   **CMake** : Version 3.16 ou supérieure.
*   **Qt Framework** : Version 6.2 ou supérieure. Assurez-vous d'installer les composants correspondant à votre compilateur.
*   **OpenCASCADE Technology (OCCT)** : Version 7.6.0 ou supérieure.

### Étapes de Compilation

1.  **Cloner le dépôt**

    ```bash
    git clone https://github.com/bezierC0/Qt_OCC.git
    cd Qt_OCC
    ```

2.  **Créer un répertoire de build**

    ```bash
    mkdir build
    cd build
    ```

3.  **Configurer avec CMake**

    Assurez-vous de définir les chemins corrects vers vos installations Qt et OCCT.

    ```bash
    # Exemple sur Windows avec Visual Studio
    cmake .. -G "Visual Studio 17 2019" -A x64 \
    -DQt5_DIR="C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5" \
    -DOpenCASCADE_DIR="C:/Libs/OCCT/occt-7.8.0-vc14-64" \
    -DOpenCASCADE_3RDPARTY_DIR="C:/Libs/OCCT/3rdparty-vc14-64"
    ```

4.  **Construire le projet**

    ```bash
    cmake --build . --config Release
    ```

5.  **Lancer l'application**

    L'exécutable se trouvera dans le répertoire `build/bin/Release` (sur Windows) ou `build/bin` (sur Linux).

## 🤝 Contribution

Les contributions sont les bienvenues ! Si vous avez une suggestion ou souhaitez corriger un bug, n'hésitez pas à ouvrir un ticket ou à soumettre une pull request.

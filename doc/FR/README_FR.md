# Application CAO Qt_OCC

Qt_OCC est une application de bureau dédiée à la CAO et aux nuages de points. Elle utilise le framework Qt pour l'interface utilisateur et OpenCASCADE Technology (OCCT) comme noyau géométrique et de visualisation. Elle fournit des fonctions de modélisation 3D, d'affichage de nuages de points, de manipulation, de mesure et d'analyse.

## ✨ Fonctionnalités

### 📂 Gestion de Fichiers
*   **Entrée/Sortie CAO** : Importation de modèles STEP/IGES et exportation vers les formats CAO pris en charge.
*   **Importation de Nuages de Points** : Importation des fichiers `.xyz`, `.txt`, `.pts` et `.ply`.

### 🎥 Visualisation & Navigation
*   **Vues Standard** : Basculez rapidement entre les vues Isométrique, Dessus, Dessous, Gauche, Droite, Avant et Arrière.
*   **Modes d'Affichage** : Basculez entre les styles de rendu Fil de fer (Wireframe), Ombré (Shaded) et Ombré avec Arêtes.
*   **Contrôle de la Vue** : Ajustement automatique de la vue au contenu de la scène.

### ☁️ Nuages de Points
*   **Données Prises en Charge** : Enregistrements XYZ et XYZRGB, fichiers PTS avec en-tête de nombre de points facultatif, PLY ASCII et PLY binaire Little Endian.
*   **Affichage OCCT Natif** : Affichage avec `AIS_PointCloud`, sans créer un sommet topologique pour chaque point.
*   **Contrôles d'Affichage** : Afficher ou masquer le nuage, l'ajuster à la vue, le supprimer, modifier la taille des points et réinitialiser les réglages.
*   **Modes de Couleur** : Couleurs RGB d'origine, couleur unique sélectionnée ou couleurs générées selon la hauteur Z.
*   **Informations du Nuage** : Nombre d'enregistrements source, points valides et affichés, enregistrements invalides, présence de RGB, taille du fichier, limites et dimensions.
*   **Protection pour les Gros Fichiers** : Lecture dans un thread de travail avec progression et annulation. Limites d'affichage de 500K, 1M, 2M, 5M ou tous les points, avec échantillonnage uniforme si nécessaire.
*   **Isolation CAD/CAE** : Les nuages de points sont exclus des opérations de modélisation, mesure, exportation, interférence et CAE qui dépendent de la topologie.

### 🛠️ Outils de Modélisation
*   **Primitives 2D** : Créez des Points, Lignes, Rectangles, Cercles, Arcs, Ellipses, Polygones, Courbes de Bézier et Courbes NURBS.
*   **Primitives 3D** : Générez des Boîtes, Sphères, Cylindres, Cônes et Pyramides.
*   **Opérations Booléennes** : Effectuez des opérations d'Union (Fuse), d'Intersection (Common) et de Différence (Cut) sur des solides.
*   **Connecteur de Tôle** : Reliez deux faces sélectionnées par une tôle à chemin orthogonal avec plis arrondis, extrémités soudées ou boulonnées et contrôle des collisions.
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

## 📚 Documentation

*   **Guide du Connecteur de Tôle** : [`../tool_busbar.md`](../tool_busbar.md)
*   **Implémentation interne du Connecteur de Tôle** : [`../sheet_metal_connector_implementation.md`](../sheet_metal_connector_implementation.md)
*   **Plan de développement et de test manuel des nuages de points** : [`../point_cloud_development_plan.md`](../point_cloud_development_plan.md)
*   **Données d'exemple de nuages de points** : [`../../samples/pointcloud/README.md`](../../samples/pointcloud/README.md)

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

### Importer un Nuage de Points

1. Ouvrez **Fichier > Importer**.
2. Sélectionnez un fichier `.xyz`, `.txt`, `.pts` ou `.ply`.
3. Utilisez le panneau **Nuage de points** pour consulter les statistiques et modifier l'affichage.
4. Utilisez **Ajuster le nuage de points** pour cadrer toutes les données.

L'importation d'un autre modèle ou nuage de points remplace la scène actuelle. Les formats PLY Binary Big Endian, PCD et LAS/LAZ, ainsi que le recalage, le filtrage et la reconstruction de surface ne sont pas encore pris en charge. Les commandes propres à la CAO et au CAE sont indisponibles lorsqu'un nuage de points est chargé.

Les fichiers de test manuel se trouvent dans [`samples/pointcloud`](../../samples/pointcloud). Le fichier `binary_colored_cube.ply` vérifie un PLY Binary Little Endian avec un ordre de propriétés mixte et des données RGB.

## 🤝 Contribution

Les contributions sont les bienvenues ! Si vous avez une suggestion ou souhaitez corriger un bug, n'hésitez pas à ouvrir un ticket ou à soumettre une pull request.

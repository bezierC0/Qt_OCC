# Aplicación CAD Qt_OCC

Qt_OCC es una aplicación CAD de escritorio desarrollada utilizando el poder del marco de trabajo Qt para la interfaz de usuario y OpenCASCADE Technology (OCCT) como núcleo geométrico. Su objetivo es proporcionar una plataforma para el modelado y visualización 3D, incorporando una gama de características para la creación, manipulación y análisis de formas.

## ✨ Características

### 📂 Gestión de Archivos
*   **Entrada/Salida**: Soporte para crear nuevos proyectos, abrir archivos existentes, guardar y exportar modelos.

### 🎥 Visualización y Navegación
*   **Vistas Estándar**: Cambie rápidamente entre vistas Isométrica, Superior, Inferior, Izquierda, Derecha, Frontal y Trasera.
*   **Modos de Visualización**: Alterne entre los estilos de renderizado Alámbrico (Wireframe), Sombreado (Shaded) y Sombreado con Bordes.
*   **Control de Vista**: Ajuste automático de la vista al contenido de la escena.

### 🛠️ Herramientas de Modelado
*   **Primitivas 2D**: Cree Puntos, Líneas, Rectángulos, Círculos, Arcos, Elipses, Polígonos, Curvas de Bezier y Curvas NURBS.
*   **Primitivas 3D**: Genere Cajas, Esferas, Cilindros, Conos y Pirámides.
*   **Operaciones Booleanas**: Realice Unión (Fuse), Intersección (Common) y Diferencia (Cut) en sólidos.
*   **Modificaciones**:
    *   **Espejo**: Refleje objetos por Plano o Eje.
    *   **Patrón**: Cree patrones Lineales y Circulares.
    *   **Vaciado (Shell)**: Convierta sólidos en estructuras huecas.

### 📏 Medición y Análisis
*   **Mediciones**: Herramientas precisas para medir Distancia, Longitud de Borde, Longitud de Arco/Radio y Ángulos.
*   **Análisis**: Comprobación de Interferencias entre objetos.
*   **Visualización de Ensamblaje**: Vista explosionada para inspeccionar componentes internos.
*   **Recorte**: Aplique planos de corte para vistas de sección transversal.

### ⚙️ Utilidades y Personalización
*   **Transformación**: Herramientas interactivas para mover, rotar y escalar objetos.
*   **Plano de Trabajo**: Ayudante de sistema de coordenadas configurable.
*   **Filtros de Selección**: Filtre la selección por vértice, borde, cara o sólido.
*   **Soporte de Temas**: Cambie entre temas Claro y Oscuro.
*   **Localización**: Soporte multilingüe.

## 🛠️ Pila Tecnológica

*   **C++**: La lógica central de la aplicación está escrita en C++ moderno (estándar C++17).
*   **Qt Framework**: Utilizado para toda la interfaz gráfica de usuario (GUI), utilizando SARibbon para una barra de herramientas moderna estilo cinta.
*   **OpenCASCADE Technology (OCCT)**: El potente núcleo de modelado geométrico 3D de código abierto que maneja todas las operaciones CAD.
*   **CMake**: El sistema de construcción multiplataforma utilizado para configurar y compilar el proyecto.

## 🚀 Comenzando

Siga estas instrucciones para obtener una copia del proyecto y ejecutarla en su máquina local para fines de desarrollo y prueba.

### Requisitos Previos

Necesitará tener instalado el siguiente software en su sistema:

*   **Un compilador compatible con C++17**:
    *   Windows: MSVC 2019 o posterior
*   **CMake**: Versión 3.16 o superior.
*   **Qt Framework**: Versión 6.2 o superior. Asegúrese de instalar los componentes que coincidan con su compilador.
*   **OpenCASCADE Technology (OCCT)**: Versión 7.6.0 o superior.

### Pasos de Compilación

1.  **Clonar el repositorio**

    ```bash
    git clone https://github.com/bezierC0/Qt_OCC.git
    cd Qt_OCC
    ```

2.  **Crear un directorio de compilación**

    ```bash
    mkdir build
    cd build
    ```

3.  **Configurar con CMake**

    Asegúrese de establecer las rutas correctas a sus instalaciones de Qt y OCCT.

    ```bash
    # Ejemplo en Windows con Visual Studio
    cmake .. -G "Visual Studio 17 2019" -A x64 \
    -DQt5_DIR="C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5" \
    -DOpenCASCADE_DIR="C:/Libs/OCCT/occt-7.8.0-vc14-64" \
    -DOpenCASCADE_3RDPARTY_DIR="C:/Libs/OCCT/3rdparty-vc14-64"
    ```

4.  **Compilar el proyecto**

    ```bash
    cmake --build . --config Release
    ```

5.  **Ejecutar la aplicación**

    El ejecutable se ubicará en el directorio `build/bin/Release` (en Windows) o `build/bin` (en Linux).

## 🤝 Contribuyendo

¡Las contribuciones son bienvenidas! Si tiene alguna sugerencia o desea corregir un error, no dude en abrir un issue o enviar un pull request.

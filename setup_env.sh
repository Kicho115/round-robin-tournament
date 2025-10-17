#!/bin/bash
# Script para configurar CMAKE_PREFIX_PATH con todas las dependencias de vcpkg
# Nota: Copiar el contenido de CMakeLists.txt dentro de round-robin-tournament en el CMakeLists.txt del proyecto creado y hacer las modificaiones necesarias.
# Se puede no hacer lo que dice la nota pero requeriria otros pasos.

# =====================
# Comandos para ejecutar los tests desde cero
# =====================
# 1. Instalar dependencias con vcpkg (solo si no están instaladas)
#    vcpkg install
#    # Instala las bibliotecas necesarias para compilar y ejecutar los tests.
#
# 2. Limpiar la caché y los archivos de compilación previos
#    rm -rf cmake-build-debug CMakeCache.txt
#    # Elimina archivos de compilación y caché para evitar conflictos y asegurar una build limpia.
#
# ---Despues de limpiar el cache, hay que recargar el proyecto cmake---
#
# 3. Configurar las variables de entorno para vcpkg
#    source ./setup_env.sh
#    # Establece CMAKE_PREFIX_PATH para que CMake encuentre las dependencias instaladas por vcpkg.
#
# 4. Generar los archivos de compilación con CMake
#    cmake -S . -B cmake-build-debug
#    # Prepara el entorno de compilación y genera los archivos necesarios.
#
# 5. Compilar el runner de tests
#    cmake --build cmake-build-debug --target tournament_tests_runner
#    # Compila el ejecutable que corre los tests unitarios.
#
# 6. Ejecutar los tests
#    ./cmake-build-debug/tournament_services/tests/tournament_tests_runner
#    # Ejecuta el binario de tests y muestra los resultados.
# =====================

# Obtener la ruta absoluta del directorio del script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Ruta base a la instalación de vcpkg dentro del repositorio
VCPKG_INSTALLED_DIR="$SCRIPT_DIR/../vcpkg_installed/x64-linux/share"

# Construir CMAKE_PREFIX_PATH con las dependencias necesarias
export CMAKE_PREFIX_PATH="\
$VCPKG_INSTALLED_DIR/crow:\
$VCPKG_INSTALLED_DIR/hypodermic:\
$VCPKG_INSTALLED_DIR/libpqxx:\
$VCPKG_INSTALLED_DIR/gtest:\
$VCPKG_INSTALLED_DIR/nlohmann_json:\
$VCPKG_INSTALLED_DIR/activemq-cpp:\
$CMAKE_PREFIX_PATH"

echo "CMAKE_PREFIX_PATH configurado para todas las dependencias de vcpkg."
echo "Ahora puedes ejecutar cmake y compilar sin problemas de dependencias."

#!/usr/bin/env bash
set -e

export VCPKG_ROOT="$HOME/vcpkg"   # ajusta si lo tienes en otra ruta
export CMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
export VCPKG_DEFAULT_TRIPLET="x64-linux"
export VCPKG_FEATURE_FLAGS="manifests,binarycaching"

echo "VCPKG_ROOT=$VCPKG_ROOT"
echo "CMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE"
echo "VCPKG_DEFAULT_TRIPLET=$VCPKG_DEFAULT_TRIPLET"
echo "VCPKG_FEATURE_FLAGS=$VCPKG_FEATURE_FLAGS"

# Comandos a correr para configurar paqueterias en computadora de Rafita (Falta por corroborar que esto funcione en otros equipos)
# Cuando se abre nueva terminal ingresa comando: cd round-robin-tournament
# Despues de eso puedes empezar con los comandos de abajo
#
# rm -rf cmake-build-debug CMakeCache.txt
# source ./set_env.sh
#
# cmake -S . -B cmake-build-debug -G Ninja \
#   -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE" \
#   -DVCPKG_TARGET_TRIPLET="$VCPKG_DEFAULT_TRIPLET"
#
# cmake -S . -B cmake-build-debug (Este comando es muy parecido al del bloque pasado, pero sirve como confirmacion)
# cmake --build cmake-build-debug --target tournament_tests_runner
# ./cmake-build-debug/tournament_services/tests/tournament_tests_runner
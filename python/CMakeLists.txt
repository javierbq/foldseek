cmake_minimum_required(VERSION 3.15...3.27)
project(pyfoldseek)

# Find Python
find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

# Add pybind11
add_subdirectory(${CMAKE_SOURCE_DIR}/../lib/pybind11 pybind11)

# Create the Python extension module
pybind11_add_module(pyfoldseek
    src/bindings.cpp
    src/structure_wrapper.cpp
)

# Set C++ standard
target_compile_features(pyfoldseek PRIVATE cxx_std_17)

# Include directories from main foldseek project
target_include_directories(pyfoldseek PRIVATE
    ${CMAKE_SOURCE_DIR}/../src
    ${CMAKE_SOURCE_DIR}/../src/commons
    ${CMAKE_SOURCE_DIR}/../src/strucclustutils
    ${CMAKE_SOURCE_DIR}/../lib
    ${CMAKE_SOURCE_DIR}/../lib/3di
    ${CMAKE_SOURCE_DIR}/../lib/foldcomp/src
    ${CMAKE_SOURCE_DIR}/../lib/gemmi/include
    ${CMAKE_SOURCE_DIR}/../lib/mmseqs/src
    ${CMAKE_SOURCE_DIR}/../lib/mmseqs/src/commons
    ${CMAKE_SOURCE_DIR}/../lib/kerasify
    ${CMAKE_SOURCE_DIR}/../lib/pulchra
)

# Path to the main foldseek build directory
set(FOLDSEEK_BUILD_DIR ${CMAKE_SOURCE_DIR}/../build)

# Link against foldseek libraries (use full paths to built libraries)
target_link_libraries(pyfoldseek PRIVATE
    ${FOLDSEEK_BUILD_DIR}/src/strucclustutils/libgemmiwrapper.a
    ${FOLDSEEK_BUILD_DIR}/lib/3di/lib3di.a
    ${FOLDSEEK_BUILD_DIR}/lib/foldcomp/libfoldcomp.a
    ${FOLDSEEK_BUILD_DIR}/lib/pulchra/libpulchra.a
    ${FOLDSEEK_BUILD_DIR}/lib/kerasify/libkerasify.a
    ${FOLDSEEK_BUILD_DIR}/lib/mmseqs/lib/zstd/build/cmake/lib/libzstd.a
    z  # zlib
    bz2  # bzip2
)

# Installation
install(TARGETS pyfoldseek DESTINATION .)

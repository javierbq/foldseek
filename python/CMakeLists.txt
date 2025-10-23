cmake_minimum_required(VERSION 3.15...3.27)
project(pyfoldseek)

# Find Python
find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

# Add pybind11
add_subdirectory(../lib/pybind11 pybind11)

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
    ${CMAKE_SOURCE_DIR}/../lib/3di
    ${CMAKE_SOURCE_DIR}/../lib/foldcomp/src
    ${CMAKE_SOURCE_DIR}/../lib/gemmi/include
    ${CMAKE_SOURCE_DIR}/../lib/mmseqs/src
    ${CMAKE_SOURCE_DIR}/../lib/mmseqs/src/commons
)

# Link against foldseek libraries
# We need to build these first or link to existing build
target_link_libraries(pyfoldseek PRIVATE
    gemmiwrapper
    foldcomp
    # Note: The actual library names may differ based on foldseek's build
)

# Installation
install(TARGETS pyfoldseek DESTINATION .)

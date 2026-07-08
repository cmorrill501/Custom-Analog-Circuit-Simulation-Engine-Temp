cmake_minimum_required(VERSION 3.16)

project(CircuitTree VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64")

find_package(Qt6 REQUIRED COMPONENTS Widgets PrintSupport)

# ==========================================
# --- FetchContent Setup for Eigen ---
# ==========================================
include(FetchContent)

# 1. Turn off Eigen's test suite to skip checking for Boost/CUDA/etc.
set(EIGEN_BUILD_TESTING OFF CACHE BOOL "Disable Eigen testing" FORCE)

FetchContent_Declare(
    Eigen3
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG        3.4.0
)

# 2. Temporarily suppress deprecation/developer warnings from Eigen's legacy setup scripts
set(CMAKE_WARN_DEPRECATED_SAVED ${CMAKE_WARN_DEPRECATED})
set(CMAKE_WARN_DEV_SAVED ${CMAKE_WARN_DEV})
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
set(CMAKE_WARN_DEV OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(Eigen3)

# 3. Restore your normal warning behaviors for your own code architecture
set(CMAKE_WARN_DEPRECATED ${CMAKE_WARN_DEPRECATED_SAVED} CACHE BOOL "" FORCE)
set(CMAKE_WARN_DEV ${CMAKE_WARN_DEV_SAVED} CACHE BOOL "" FORCE)
# ==========================================

add_executable(CircuitTree 
    main.cpp 
    mainwindow.h mainwindow.cpp 
    Commands.h Commands.cpp
    CircuitManager.h CircuitManager.cpp
    qcustomplot.cpp 
    CircuitSolver.cpp
    Elements.cpp Waveforms.cpp
)

if(MSVC)
    target_compile_options(CircuitTree PRIVATE /bigobj)
    # add_compile_options(/FS) allows for compiling in parallel I believe
endif()

# Link both Qt6 and Eigen3 to your executable target
target_link_libraries(CircuitTree PRIVATE 
    Qt6::Widgets 
    Qt6::PrintSupport
    Eigen3::Eigen
)

target_precompile_headers(CircuitTree PRIVATE
    # Heavy Third-Party Math (Changed to angle brackets since CMake fetches it globally)
    <Eigen/Dense>
    
    # Core STL Templates used everywhere
    <vector>
    <memory>
    <string>
    <complex>
    <unordered_map>
    <tuple>
    <iostream>
    
    # Core Qt Framework pieces
    <QMainWindow>
    <QWidget>
    <QVector>
    <QApplication>
)

add_custom_target(run
    COMMAND ${CMAKE_COMMAND} -E env "PATH=${CMAKE_PREFIX_PATH}/bin;$ENV{PATH}" $<TARGET_FILE:CircuitTree>
    DEPENDS CircuitTree
)

if(WIN32)
    add_custom_command(TARGET CircuitTree POST_BUILD
        COMMAND ${Qt6_DIR}/../../../bin/windeployqt.exe $<TARGET_FILE:CircuitTree>
    )
endif()
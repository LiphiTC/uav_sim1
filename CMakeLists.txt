cmake_minimum_required(VERSION 3.16)
project(Practical_01_Task_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

add_executable(Practical_01_Task_1 main.cpp mainwindow.h mainwindow.cpp drone.h drone.cpp)
target_link_libraries(Practical_01_Task_1 PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

add_executable(drone_demo demo.cpp drone.h drone.cpp)
target_link_libraries(drone_demo PRIVATE Qt${QT_VERSION_MAJOR}::Core)

include(CTest)
if(BUILD_TESTING)
    add_executable(drone_tests tests.cpp drone.h drone.cpp)
    add_test(NAME drone_tests COMMAND drone_tests)
endif()

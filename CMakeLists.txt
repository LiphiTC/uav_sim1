cmake_minimum_required(VERSION 3.16)
project(Practical_01_Task_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets 3DCore 3DRender 3DExtras 3DInput Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets 3DCore 3DRender 3DExtras 3DInput Multimedia)

set(GUI_SOURCES mainwindow.h mainwindow.cpp scene3d.h scene3d.cpp
    dronevisual.h dronevisual.cpp batterygraph.h batterygraph.cpp point3d.h assets.qrc)
set(GUI_LIBRARIES Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::3DCore
    Qt${QT_VERSION_MAJOR}::3DRender Qt${QT_VERSION_MAJOR}::3DExtras
    Qt${QT_VERSION_MAJOR}::3DInput Qt${QT_VERSION_MAJOR}::Multimedia)
add_executable(Practical_01_Task_1 main.cpp drone.h drone.cpp ${GUI_SOURCES})
target_link_libraries(Practical_01_Task_1 PRIVATE ${GUI_LIBRARIES})

add_executable(drone_demo demo.cpp drone.h drone.cpp)
target_link_libraries(drone_demo PRIVATE Qt${QT_VERSION_MAJOR}::Core)

include(CTest)
if(BUILD_TESTING)
    add_executable(drone_tests tests.cpp drone.h drone.cpp)
    add_test(NAME drone_tests COMMAND drone_tests)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
    add_executable(window_tests window_tests.cpp drone.h drone.cpp ${GUI_SOURCES})
    target_link_libraries(window_tests PRIVATE ${GUI_LIBRARIES} Qt${QT_VERSION_MAJOR}::Test)
    add_test(NAME window_tests COMMAND window_tests)
    set_tests_properties(window_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

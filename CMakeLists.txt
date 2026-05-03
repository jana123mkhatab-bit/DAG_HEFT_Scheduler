cmake_minimum_required(VERSION 3.16)
project(HEFTVisualizer VERSION 2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_executable(HEFTVisualizer
    src/main.cpp
    src/MainWindow.cpp
    src/AlgorithmEngine.cpp
    src/DagScene.cpp
    src/GanttScene.cpp
    src/ControlPanel.cpp
    src/ComparisonPanel.cpp
    src/TaskDetailsPanel.cpp
)

target_link_libraries(HEFTVisualizer PRIVATE Qt6::Core Qt6::Widgets)
target_include_directories(HEFTVisualizer PRIVATE src)

if(WIN32)
    set_target_properties(HEFTVisualizer PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

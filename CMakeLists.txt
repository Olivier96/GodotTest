cmake_minimum_required(VERSION 3.16)

project(GodotQmlBridge VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.7 REQUIRED COMPONENTS Quick Network)

qt_standard_project_setup(REQUIRES 6.7)

qt_add_executable(GodotQmlBridge
    src/main.cpp
)

qt_add_qml_module(GodotQmlBridge
    URI GodotQmlBridge
    VERSION 1.0
    QML_FILES
        qml/Main.qml
    SOURCES
        src/GodotBridge.h
        src/GodotBridge.cpp
)

target_link_libraries(GodotQmlBridge PRIVATE
    Qt6::Quick
    Qt6::Network
)

# Default install target
install(TARGETS GodotQmlBridge
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

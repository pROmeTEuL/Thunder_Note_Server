cmake_minimum_required(VERSION 3.16)

project(Thunder_Note_Server LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Sql HttpServer)

add_executable(Thunder_Note_Server
  main.cpp
)
target_link_libraries(Thunder_Note_Server Qt::Core Qt::Sql Qt::HttpServer)

include(GNUInstallDirs)
install(TARGETS Thunder_Note_Server
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

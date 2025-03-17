cmake_minimum_required(VERSION 3.14)

project(QSerializer LANGUAGES CXX)

set(
        HEADER_FILES
        "QSerializer"
        "src/qserializer.h"
    )

add_library(${PROJECT_NAME} SHARED ${HEADER_FILES})

include_directories(${Qt${QT_VERSION_MAJOR}Core_INCLUDE_DIRS})

if (NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Core REQUIRED Xml REQUIRED)
endif()

target_include_directories(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} INTERFACE Qt::Core Qt::Xml)

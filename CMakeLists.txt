cmake_minimum_required(VERSION 3.14)

project(QSerializer LANGUAGES CXX)

set(
    HEADER_FILES
    "QSerializer"
    "src/qserializer.h"
)

# 修改为仅头文件的接口库
add_library(${PROJECT_NAME} INTERFACE)

# 查找Qt依赖
find_package(Qt6 COMPONENTS Core Xml QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Core Xml REQUIRED)
endif()

# 设置接口包含目录
target_include_directories(${PROJECT_NAME} 
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 设置接口依赖
target_link_libraries(${PROJECT_NAME} INTERFACE Qt::Core Qt::Xml)

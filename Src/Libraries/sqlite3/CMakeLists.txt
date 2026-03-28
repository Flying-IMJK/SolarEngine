cmake_minimum_required (VERSION 3.8)

# 设置工程名 使用语言c/c++
project(sqlite3 LANGUAGES C)

set("header" sqlite3.h sqlite3ext.h)
set("sources" sqlite3.c shell.c)


add_library(${PROJECT_NAME} STATIC ${header} ${sources})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

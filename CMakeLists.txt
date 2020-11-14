cmake_minimum_required(VERSION 3.0.0)
project(Botama VERSION 0.1.0)

set(source_files
${PROJECT_SOURCE_DIR}/Card.cpp
${PROJECT_SOURCE_DIR}/CardBoard.cpp
${PROJECT_SOURCE_DIR}/Board.cpp
${PROJECT_SOURCE_DIR}/notlosebot.cpp
)

add_executable(Botama ${source_files})
target_include_directories(Botama PUBLIC inc)
target_compile_features(Botama PUBLIC cxx_std_20)

set(CMAKE_CXX_FLAGS "-Ofast -march=native -DNDEBUG")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

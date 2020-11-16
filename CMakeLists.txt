cmake_minimum_required(VERSION 3.0.0)
project(Botama VERSION 0.1.0)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(source_files
${PROJECT_SOURCE_DIR}/Card.cpp
${PROJECT_SOURCE_DIR}/CardBoard.cpp
${PROJECT_SOURCE_DIR}/BoardUtil.cpp
${PROJECT_SOURCE_DIR}/Board.cpp
${PROJECT_SOURCE_DIR}/Botama.cpp
)

add_executable(Botama ${source_files})
target_include_directories(Botama PUBLIC inc)
target_compile_features(Botama PUBLIC cxx_std_20)

set(CMAKE_CXX_FLAGS "-march=native")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -DNDEBUG -D_DLL -D_MT -Xclang --dependent-lib=msvcrt")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Ofast -g -DNDEBUG -Xclang -gcodeview -D_DLL -D_MT -Xclang --dependent-lib=msvcrt")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

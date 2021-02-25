cmake_minimum_required(VERSION 3.0.0)
project(Botama VERSION 0.1.0)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(source_files
${PROJECT_SOURCE_DIR}/Card.cpp
${PROJECT_SOURCE_DIR}/CardBoard.cpp
${PROJECT_SOURCE_DIR}/BoardUtil.cpp
${PROJECT_SOURCE_DIR}/Board.cpp
${PROJECT_SOURCE_DIR}/BoardEval.cpp
${PROJECT_SOURCE_DIR}/MoveGenerator.cpp
${PROJECT_SOURCE_DIR}/Search.cpp
${PROJECT_SOURCE_DIR}/TableBase.cpp
${PROJECT_SOURCE_DIR}/TranspositionTable.cpp
${PROJECT_SOURCE_DIR}/Game.cpp
${PROJECT_SOURCE_DIR}/easywsclient.cpp
${PROJECT_SOURCE_DIR}/Connection.cpp
${PROJECT_SOURCE_DIR}/Botama.cpp
)

add_executable(Botama ${source_files})
target_compile_features(Botama PUBLIC cxx_std_20)
target_include_directories(Botama PRIVATE ${PROJECT_SOURCE_DIR}/Include)

#target_include_directories(Botama PUBLIC include)

set(CMAKE_CXX_FLAGS "-march=native")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -DNDEBUG -D_MT -Xclang --dependent-lib=msvcrt -Wmicrosoft-goto")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Ofast -g -DNDEBUG -Xclang -gcodeview -D_DLL -D_MT -Xclang --dependent-lib=msvcrt -Wmicrosoft-goto")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack) 
 
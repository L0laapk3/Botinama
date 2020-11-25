cmake_minimum_required(VERSION 3.0.0)
project(Botama VERSION 0.1.0)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(source_files
${PROJECT_SOURCE_DIR}/Card.cpp
${PROJECT_SOURCE_DIR}/CardBoard.cpp
${PROJECT_SOURCE_DIR}/BoardUtil.cpp
${PROJECT_SOURCE_DIR}/Board.cpp
${PROJECT_SOURCE_DIR}/BoardEval.cpp
${PROJECT_SOURCE_DIR}/BoardSearch.cpp
${PROJECT_SOURCE_DIR}/TableBase.cpp
${PROJECT_SOURCE_DIR}/easywsclient.cpp
${PROJECT_SOURCE_DIR}/Connection.cpp
${PROJECT_SOURCE_DIR}/Botama.cpp
)

add_executable(Botama ${source_files})
target_compile_features(Botama PUBLIC cxx_std_20)

# target_link_libraries(Botama PRIVATE ${PROJECT_SOURCE_DIR}/lib/liblzma.a)
add_subdirectory(${PROJECT_SOURCE_DIR}/7zip-cpp ${PROJECT_SOURCE_DIR}/build/build7zpp)
# I know this is a garbage way to do this but at this point I really dont care
#target_include_directories(Botama PRIVATE ${PROJECT_SOURCE_DIR}/7zip-cpp/Include "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.28.29333/include" "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.28.29333/atlmfc/include" "C:/Program Files (x86)/Windows Kits/10/Include/10.0.18362.0/ucrt" "C:/Program Files (x86)/Windows Kits/10/include/10.0.18362.0/shared" "C:/Program Files (x86)/Windows Kits/10/include/10.0.18362.0/um" "C:/Program Files (x86)/Windows Kits/10/include/10.0.18362.0/winrt")
target_include_directories(Botama PRIVATE ${PROJECT_SOURCE_DIR}/7zip-cpp/Include)
target_link_libraries(Botama 7zpp)
add_dependencies(Botama 7zpp)

#target_include_directories(Botama PUBLIC include)

set(CMAKE_CXX_FLAGS "-march=native")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -DNDEBUG -D_MT -Xclang --dependent-lib=msvcrt")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Ofast -g -DNDEBUG -Xclang -gcodeview -D_DLL -D_MT -Xclang --dependent-lib=msvcrt")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack) 
 
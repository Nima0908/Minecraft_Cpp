cmake_minimum_required(VERSION 3.14)
project(MinecraftClient LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to enable static linking
option(STATIC_LINKING "Enable static linking of dependencies" OFF)

# Compile-time definitions
add_compile_definitions(MC_ENABLE_LOGGING)

# IDE folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Use Clang and mold
if(NOT CMAKE_C_COMPILER OR NOT CMAKE_CXX_COMPILER)
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)
endif()

# Use mold as the linker
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=mold")

# Custom compile flags
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    -pipe
    -O2
)

# Prefer static or shared libraries
if(STATIC_LINKING)
    message(STATUS "Configuring static linking")
    set(Boost_USE_STATIC_LIBS ON)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
    set(ZLIB_USE_STATIC_LIBS TRUE)
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
else()
    message(STATUS "Configuring dynamic linking")
    set(Boost_USE_STATIC_LIBS OFF)
    set(BUILD_SHARED_LIBS ON)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".dylib" ".dll")
endif()

# Find dependencies
find_package(Boost REQUIRED COMPONENTS system json)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# Source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.hpp
)

# Executable
add_executable(mc_client ${SOURCES})

# Include directories
target_include_directories(mc_client PRIVATE
    ${Boost_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(mc_client
    PRIVATE
        Boost::system
        Boost::json
        OpenSSL::SSL
        OpenSSL::Crypto
        ZLIB::ZLIB
)

# Print final config
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Linker flags: ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

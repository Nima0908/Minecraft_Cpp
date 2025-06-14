cmake_minimum_required(VERSION 3.14)
project(MinecraftClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use folders in IDEs like Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Find Boost
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)

# Include Boost headers
include_directories(${Boost_INCLUDE_DIRS})

# Add source files
file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/*.hpp
)

add_executable(mc_client ${SOURCES})

# Link Boost
target_link_libraries(mc_client PRIVATE Boost::system)
target_link_libraries(mc_client PRIVATE OpenSSL::SSL OpenSSL::Crypto)


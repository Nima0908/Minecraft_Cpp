cmake_minimum_required(VERSION 3.14)
project(MinecraftClient LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# IDE folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Find dependencies
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)

# Add source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.hpp
)

# Executable
add_executable(mc_client ${SOURCES})

# Include directories
target_include_directories(mc_client PRIVATE ${Boost_INCLUDE_DIRS})

# Link libraries
target_link_libraries(mc_client
    PRIVATE
        Boost::system
        OpenSSL::SSL
        OpenSSL::Crypto
)


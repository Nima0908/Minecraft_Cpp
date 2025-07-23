cmake_minimum_required(VERSION 3.14)
project(MinecraftClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiletime definition
add_compile_definitions(MC_ENABLE_LOGGING)

# IDE folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Find dependencies
find_package(Boost REQUIRED COMPONENTS system json)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# Add source files
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

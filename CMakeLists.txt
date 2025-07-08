cmake_minimum_required(VERSION 3.14)
project(MinecraftClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# IDE folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Prefer mold if available
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-fuse-ld=mold" HAS_MOLD)
if (HAS_MOLD)
    message(STATUS "Using mold linker")
    add_link_options("-fuse-ld=mold")
endif()

# Find dependencies
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

# Add nlohmann/json (header-only library)
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

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
    ${CURL_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${Vulkan_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(mc_client
    PRIVATE
        Boost::system
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
        ZLIB::ZLIB
        nlohmann_json::nlohmann_json
        ${SDL2_LIBRARIES}
        Vulkan::Vulkan
)

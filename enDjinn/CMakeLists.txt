cmake_minimum_required(VERSION 3.14...3.31)
project(
    enDjinn # Name this whatever you want
    LANGUAGES CXX C
)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

## Install glfw
FetchContent_Declare(
    glfw3
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable( glfw3 )

## Install spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog/
    GIT_TAG v1.15.3
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable( spdlog )

# Install sokol
FetchContent_Declare(
    sokol
    GIT_REPOSITORY https://github.com/floooh/sokol
    GIT_TAG        master
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable( sokol )

# Install soloud
FetchContent_Declare(
  soloud
  GIT_REPOSITORY https://github.com/jarikomppa/soloud
  GIT_TAG        e82fd32c1f62183922f08c14c814a02b58db1873
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable( soloud )
file(GLOB soloud_sources "${soloud_SOURCE_DIR}/src/audiosource/*/*.c*" "${soloud_SOURCE_DIR}/src/c_api/*.c*" "${soloud_SOURCE_DIR}/src/core/*.c*" "${soloud_SOURCE_DIR}/src/filter/*.c*" "${soloud_SOURCE_DIR}/src/backend/miniaudio/soloud_miniaudio.cpp")
add_library(soloud ${soloud_sources} "engine/assets/Sprite.h")
target_compile_definitions(soloud PRIVATE WITH_MINIAUDIO)
target_include_directories(soloud PUBLIC "${soloud_SOURCE_DIR}/include")
if(APPLE)
    find_library(AudioUnit_LIBRARY AudioUnit)
    find_library(CoreAudio_LIBRARY CoreAudio)
    target_link_libraries(soloud INTERFACE ${AudioUnit_LIBRARY} ${CoreAudio_LIBRARY})
    target_compile_definitions(soloud PRIVATE MA_NO_RUNTIME_LINKING)
endif()
if(WIN32)
    target_compile_definitions( soloud PRIVATE MA_NO_JACK )
endif()

set(WEBGPU_BACKEND "dawn" CACHE STRING "dawn")
FetchContent_Declare(
  webgpu
  GIT_REPOSITORY https://github.com/yig/WebGPU-distribution
  GIT_TAG        cs425-2025
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable( webgpu )

FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/walterschell/Lua
    GIT_TAG 504ef66d500fa1fb4f1684b6617b01342eee704a
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
set(LUA_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable( lua )

FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/yig/sol2
    GIT_TAG cs425-2025
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable( sol2 )

FetchContent_Declare(
  glfw3webgpu
  GIT_REPOSITORY https://github.com/eliemichel/glfw3webgpu
  GIT_TAG        fdcabcc54b56b50c12c10f5317abf8ae7ac32c29
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable( glfw3webgpu )

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable( glm )

add_library(sokol INTERFACE)
target_include_directories(sokol INTERFACE ${sokol_SOURCE_DIR})

FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb/
  GIT_TAG        f58f558c120e9b32c217290b80bad1a0729fbb2c
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable( stb )
## stb doesn't have a `CMakeLists.txt`. Let's declare a header-only library with an include path.
add_library( stb INTERFACE )
target_include_directories( stb INTERFACE ${stb_SOURCE_DIR} )

## Declare the engine library
add_library(enDjinn STATIC
    engine/Engine.cpp
    engine/managers/GraphicsManager.cpp
    engine/utils/SokolImplementations.cpp 
    engine/managers/InputManager.cpp
    engine/assets/ResourceManager.cpp
    engine/managers/SoundManager.cpp
    engine/managers/ScriptManager.cpp
    engine/assets/Sprite.h)
set_target_properties(enDjinn PROPERTIES CXX_STANDARD 20)
add_custom_target(run_helloworld helloworld USES_TERMINAL WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
target_include_directories(enDjinn PUBLIC engine)
target_link_libraries(enDjinn PUBLIC glfw spdlog::spdlog sokol soloud webgpu glfw3webgpu glm stb sol2 lua_static)

add_executable(helloworld demo/helloworld.cpp "engine/utils/SokolImplementations.cpp" "engine/assets/Sprite.h")
set_target_properties(helloworld PROPERTIES CXX_STANDARD 20)
target_link_libraries(helloworld PRIVATE enDjinn)

target_copy_webgpu_binaries(helloworld)
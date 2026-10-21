cmake_minimum_required(VERSION 3.22)
project(olcpixelgameengine3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(USE_STB "Use stb_image.h for image loading" OFF)

if(NOT DEFINED ANDROID_NDK)
    if(DEFINED CMAKE_ANDROID_NDK)
        set(ANDROID_NDK ${CMAKE_ANDROID_NDK})
    else()
        set(ANDROID_NDK $ENV{ANDROID_NDK})
    endif()
endif()

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

file(GLOB_RECURSE SOURCES "*.cpp" "*.h")
add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
        ${ANDROID_NDK}/sources/android/native_app_glue)

if(USE_STB)
    target_compile_definitions(${PROJECT_NAME} PRIVATE OLC_USE_STB_IMAGE=1)
endif()

target_link_libraries(${PROJECT_NAME}
        android
        log
        EGL
        GLESv3
        jnigraphics
)

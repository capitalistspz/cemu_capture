cmake_minimum_required(VERSION 3.23)
project(cemu_capture)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CMakeDependentOption)

cmake_dependent_option(CEMU_CAPTURE_USE_LIBSYSTEMD "Use systemd for device enumeration" ON "UNIX AND NOT APPLE" OFF)

option(CEMU_CAPTURE_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})

if (CEMU_CAPTURE_USE_LIBSYSTEMD)
    find_package(libsystemd REQUIRED)
endif ()

# VCPKG sets CMAKE_FIND_PACKAGE_PREFER_CONFIG, but doesn't provide libyv::libyuv as a target, so the find module adds that alias
find_package(libyuv MODULE REQUIRED)

add_library(cemu_capture src/ContextCommon.hpp src/SourceCommon.hpp src/util.hpp)
add_library(cemu_capture::cemu_capture ALIAS cemu_capture)

target_compile_features(cemu_capture PUBLIC cxx_std_20)

target_sources(cemu_capture
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES include/cemu_capture.hpp
)

if (UNIX AND NOT APPLE)
    target_sources(cemu_capture PRIVATE
            src/v4l2/ConversionTable.hpp
            src/v4l2/FileDescriptor.hpp
            src/v4l2/Fract.hpp
            src/v4l2/V4L2Ioctls.hpp
            src/v4l2/MemoryMapped.hpp
            src/v4l2/V4L2Context.cpp
            src/v4l2/V4L2Context.hpp
            src/v4l2/V4L2Source.cpp
            src/v4l2/V4L2Source.hpp

    )
    message(STATUS "cemu_capture: V4L2 backend")
else ()
    message(SEND_ERROR "Unsupported platform")
endif ()


target_link_libraries(cemu_capture PRIVATE libyuv::libyuv)

if (CEMU_CAPTURE_USE_LIBSYSTEMD)
    target_link_libraries(cemu_capture PRIVATE libsystemd::libsystemd)
    target_compile_definitions(cemu_capture PRIVATE CEMU_CAPTURE_USE_LIBSYSTEMD)
endif ()


if (CEMU_CAPTURE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

include(GNUInstallDirs)

install(TARGETS cemu_capture
        EXPORT cemu_capture-targets
        FILE_SET HEADERS
)

install(EXPORT cemu_capture-targets
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cemu_capture
        NAMESPACE cemu_capture::
)
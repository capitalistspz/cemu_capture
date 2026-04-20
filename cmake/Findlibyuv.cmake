# Thank you Google for not even implementing pkgconfig or cmake package config
# This find_package call is really for VCPKG or Conan users
find_package(libyuv CONFIG QUIET)
if (libyuv_FOUND AND NOT TARGET libyuv::libyuv)
    add_library(libyuv::libyuv ALIAS yuv)
endif ()

if (NOT libyuv_FOUND)
    find_package(PkgConfig QUIET)
    if (PkgConfig_FOUND)
        pkg_search_module(libyuv IMPORTED_TARGET libyuv)
        if (libyuv_FOUND)
            add_library(libyuv::libyuv ALIAS PkgConfig::libyuv)
        endif ()
    endif ()
endif ()
if (NOT libyuv_FOUND)
    find_package(JPEG QUIET)
    if (JPEG_FOUND)
        find_library(libyuv_LIBRARY NAMES libyuv.so yuv.dll libyuv.dylib)
        find_library(libyuv_STATIC_LIBRARY NAMES libyuv.a yuv.lib)
        find_path(libyuv_INCLUDE_DIR libyuv.h)
        if (libyuv_INCLUDE_DIR)
            if (libyuv_LIBRARY)
                add_library(libyuv::libyuv SHARED IMPORTED)
                target_include_directories(libyuv::libyuv INTERFACE ${libyuv_INCLUDE_DIR})
                target_link_libraries(libyuv::libyuv INTERFACE JPEG::JPEG)
                set_property(TARGET libyuv::libyuv PROPERTY IMPORTED_LOCATION ${libyuv_LIBRARY})
                set(libyuv_LINK_LIBRARIES "${libyuv_LIBRARY};${JPEG_LIBRARIES}")
                set(libyuv_FOUND TRUE)
            endif ()

            if (libyuv_STATIC_LIBRARY)
                add_library(libyuv::libyuv-static STATIC IMPORTED)
                target_include_directories(libyuv::libyuv-static INTERFACE ${libyuv_INCLUDE_DIR})
                target_link_libraries(libyuv::libyuv INTERFACE JPEG::JPEG)
                set_property(TARGET libyuv::libyuv-static PROPERTY IMPORTED_LOCATION ${libyuv_STATIC_LIBRARY})
                set(libyuv_STATIC_LINK_LIBRARIES "${libyuv_STATIC_LIBRARY};${JPEG_LIBRARIES}")
                set(libyuv_FOUND TRUE)
            endif ()
        endif ()
    endif ()

endif ()

find_package_handle_standard_args(libyuv
        REQUIRED_VARS
        libyuv_FOUND
)



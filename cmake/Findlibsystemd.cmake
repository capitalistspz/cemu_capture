
find_package(libsystemd CONFIG QUIET)
if (NOT libsystemd_FOUND)
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_search_module(libsystemd IMPORTED_TARGET GLOBAL libsystemd)
        if (libsystemd_FOUND)
            add_library(libsystemd::libsystemd ALIAS PkgConfig::libsystemd)
        endif ()
    endif ()
endif ()

find_package_handle_standard_args(libsystemd
        REQUIRED_VARS
        libsystemd_FOUND
        libsystemd_LINK_LIBRARIES
        VERSION_VAR libsystemd_VERSION)
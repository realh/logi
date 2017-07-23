project(logi CXX)
cmake_minimum_required(VERSION 3.1)

include(FindPkgConfig)

if(UNIX)
    add_definitions(-Wall -Wextra -Wunused -Wno-missing-field-initializers
        -std=c++11)
endif()

set(LOGI_CORE_PKGS glibmm-2.4 gthread-2.0 gobject-2.0)

pkg_check_modules(GLIB REQUIRED ${LOGI_CORE_PKGS})
pkg_check_modules(GUDEV REQUIRED ${LOGI_CORE_PKGS} gudev-1.0)
pkg_check_modules(SQLITE REQUIRED ${LOGI_CORE_PKGS} sqlite3)

include_directories(BEFORE src)

add_subdirectory(src)
add_subdirectory(tests)

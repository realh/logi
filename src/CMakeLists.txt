add_subdirectory(scan)
add_subdirectory(udev)

set(LOGI_CORE_SOURCES
    frontend.cpp
    receiver.cpp
    section-filter.cpp
    tuning.cpp
    si/decode-string.cpp
    si/nit-section.cpp
    si/section.cpp
    si/section-data.cpp
    si/table-tracker.cpp
)

set(LOGI_CORE_HEADERS
    frontend.h
    receiver.h
    section-filter.h
    tuning.h
    si/decode-string.h
    si/descriptor.h
    si/network-name-descriptor.h
    si/nit-section.h
    si/section.h
    si/section-data.h
    si/table-tracker.h
    si/ts-data.h
)

add_library(logicore STATIC ${LOGI_CORE_SOURCES})
target_compile_options(logicore PUBLIC ${GLIB_CFLAGS})

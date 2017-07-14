add_subdirectory(scan)
add_subdirectory(udev)

set(LOGI_CORE_SOURCES
    frontend.cpp
    receiver.cpp
    section-filter.cpp
    tuning.cpp
    si/decode-string.cpp
    si/delsys-descriptor.cpp
    si/huffman.cpp
    si/huffman-table1.cpp
    si/huffman-table2.cpp
    si/nit-section.cpp
    si/section.cpp
    si/section-data.cpp
    si/service-list-descriptor.cpp
    si/table-tracker.cpp
    si/terr-delsys-descriptor.cpp
)

set(LOGI_CORE_HEADERS
    frontend.h
    receiver.h
    section-filter.h
    tuning.h
    si/decode-string.h
    si/delsys-descriptor.h
    si/descriptor.h
    si/huffman.h
    si/network-name-descriptor.h
    si/nit-section.h
    si/section.h
    si/section-data.h
    si/service-list-descriptor.h
    si/table-tracker.h
    si/terr-delsys-descriptor.h
    si/ts-data.h
)

add_custom_command(OUTPUT si/huffman-table1.cpp
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/si/huff2c.py
        ${CMAKE_CURRENT_SOURCE_DIR}/si/freesat.t1 si/huffman-table1.cpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/si/huff2c.py
        ${CMAKE_CURRENT_SOURCE_DIR}/si/freesat.t1
    WORKING_DIRECTORY ${CMAKE_CURRENT_DIR})

add_custom_command(OUTPUT si/huffman-table2.cpp
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/si/huff2c.py
        ${CMAKE_CURRENT_SOURCE_DIR}/si/freesat.t2 si/huffman-table2.cpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/si/huff2c.py
        ${CMAKE_CURRENT_SOURCE_DIR}/si/freesat.t2
    WORKING_DIRECTORY ${CMAKE_CURRENT_DIR})

add_library(logicore STATIC ${LOGI_CORE_SOURCES})
target_compile_options(logicore PUBLIC ${GLIB_CFLAGS})

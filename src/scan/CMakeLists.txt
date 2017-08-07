set(LOGI_SCAN_SOURCES
    dvbt-tuning-iterator.cpp
    freesat-channel-scanner.cpp
    freesat-tuning-iterator.cpp
    freeview-channel-scanner.cpp
    multi-scanner.cpp
    nit-processor.cpp
    sdt-processor.cpp
    single-channel-scanner.cpp
)

set(LOGI_SCAN_HEADERS
    dvbt-tuning-iterator.h
    freesat-channel-scanner.h
    freesat-tuning-iterator.h
    freeview-channel-scanner.h
    multi-scanner.h
    nit-processor.h
    sdt-processor.h
    single-channel-scanner.h
    tuning-iterator.h
)

add_library(logiscan STATIC ${LOGI_SCAN_SOURCES})
target_compile_options(logiscan PUBLIC ${GLIB_CFLAGS})
target_link_libraries(logiscan logicore)

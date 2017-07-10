set(LOGI_SCAN_SOURCES
    channel-scanner.cpp
    dvbt-tuning-iterator.cpp
    multi-scanner.cpp
    nit-processor.cpp
    standard-channel-scanner.cpp
)

set(LOGI_SCAN_HEADERS
    channel-scanner.h
    dvbt-tuning-iterator.h
    multi-scanner.h
    nit-processor.h
    standard-channel-scanner.h
    tuning-iterator.h
)

add_library(logiscan STATIC ${LOGI_SCAN_SOURCES})
target_compile_options(logiscan PUBLIC ${GLIB_CFLAGS})
target_link_libraries(logiscan logicore)

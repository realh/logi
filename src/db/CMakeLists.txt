set(LOGI_DB_SOURCES
    logi-db.cpp
    logi-sqlite.cpp
)

set(LOGI_DB_HEADERS
    logi-db.h
    logi-sqlite.h
)

add_library(logidb STATIC ${LOGI_DB_SOURCES})
target_compile_options(logidb PUBLIC ${SQLITE_CFLAGS})
target_link_libraries(logidb logicore)

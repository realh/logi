set(LOGI_UDEV_TEST_FILES
    udev.c
)

option(ENABLE_TESTS "Enable building of tests" ON)

if (ENABLE_TESTS)
    add_executable(frontends frontends.cpp)
    target_compile_options(frontends PUBLIC ${GUDEV_CFLAGS})
    target_link_libraries(frontends logiudev logicore ${GUDEV_LIBRARIES})

    add_executable(tune tune.cpp)
    target_compile_options(tune PUBLIC ${GLIB_CFLAGS})
    target_link_libraries(tune logicore ${GLIB_LIBRARIES} -lm)

    add_executable(fvscan fvscan.cpp)
    target_compile_options(fvscan PUBLIC ${GUDEV_CFLAGS} ${SQLITE_CFLAGS})
    target_link_libraries(fvscan logiscan logidb logiudev logicore
        ${GUDEV_LIBRARIES} ${SQLITE_LIBRARIES} -lpthread -lm)

    add_executable(fsscan fsscan.cpp)
    target_compile_options(fsscan PUBLIC ${GUDEV_CFLAGS} ${SQLITE_CFLAGS})
    target_link_libraries(fsscan logiscan logidb logiudev logicore
        ${GUDEV_LIBRARIES} ${SQLITE_LIBRARIES} -lpthread -lm)
endif (ENABLE_TESTS)


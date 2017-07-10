set(LOGI_UDEV_HEADERS
    udev-client.h
    udev-device.h
)

set(LOGI_UDEV_SOURCES
    udev-client.cpp
    udev-device.cpp
)

add_library(logiudev STATIC ${LOGI_UDEV_SOURCES})
target_compile_options(logiudev PUBLIC ${GUDEV_CFLAGS})

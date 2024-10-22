include (${SHARED_DIR}/utils/uncommon/shared_uncommon.cmake)
add_executable(raw main.c ${UNCOMMON_SHARED_SOURCE})
target_link_libraries(raw PRIVATE vmlib)

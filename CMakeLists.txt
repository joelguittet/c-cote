# CMake minimum version
cmake_minimum_required(VERSION 3.14)

# Project name
project(c-cote)

# Project version
set(COTE_VERSION_MAJOR 1)
set(COTE_VERSION_MINOR 0)
set(COTE_VERSION_PATCH 0)

# Additional flags
set(c_flags "${c_flags} -Os -ffunction-sections -Wall -fPIC")
set(linker_flags "${linker_flags} -Wl,-gc-sections")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")

# Definitions
add_definitions(-DCOTE_EXPORT_SYMBOLS -DCOTE_API_VISIBILITY)

# Output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/bin)

# CMake subdirectories
if (NOT TARGET amp)
  if(EXISTS lib/amp)
    add_subdirectory(lib/amp)
  endif()
endif()
if (NOT TARGET axon)
  if(EXISTS lib/axon)
    add_subdirectory(lib/axon)
  endif()
endif()
if (NOT TARGET discover)
  if(EXISTS lib/discover)
    add_subdirectory(lib/discover)
  endif()
endif()
if(NOT TARGET cjson)
  if(EXISTS lib/cJSON)
    add_subdirectory(lib/cJSON)
  endif()
endif()

# List of sources
file(GLOB_RECURSE src "src/*.c")

# Add include directories
include_directories(inc)
if(EXISTS lib/amp)
  include_directories(lib/amp/inc)
endif()
if(EXISTS lib/axon)
  include_directories(lib/axon/inc)
endif()
if(EXISTS lib/discover)
  include_directories(lib/discover/inc)
endif()
if(EXISTS lib/cJSON)
  include_directories(lib/cJSON)
endif()

# Creation of the library
add_library(cote SHARED ${src})

# Link the library with the wanted libraries
target_link_libraries(cote discover axon amp cjson pthread rt)

# Properties of the library
set_target_properties(cote
  PROPERTIES
  SOVERSION "${COTE_VERSION_MAJOR}"
  VERSION "${COTE_VERSION_MAJOR}.${COTE_VERSION_MINOR}.${COTE_VERSION_PATCH}"
)

# Creation of the examples binaries
option(ENABLE_COTE_EXAMPLES "Enable building cote examples" OFF)
if(ENABLE_COTE_EXAMPLES)
  add_executable(monitor "examples/monitor/monitor.c")
  target_link_libraries(monitor cote)
  add_executable(publisher "examples/pubsub/publisher.c")
  target_link_libraries(publisher cote)
  add_executable(subscriber "examples/pubsub/subscriber.c")
  target_link_libraries(subscriber cote)
  add_executable(publisher_namespace1 "examples/pubsub_namespace/publisher_namespace1.c")
  target_link_libraries(publisher_namespace1 cote)
  add_executable(subscriber_namespace1 "examples/pubsub_namespace/subscriber_namespace1.c")
  target_link_libraries(subscriber_namespace1 cote)
  add_executable(publisher_topic1_topic2 "examples/pubsub_topics/publisher_topic1_topic2.c")
  target_link_libraries(publisher_topic1_topic2 cote)
  add_executable(subscriber_topic1_topic2 "examples/pubsub_topics/subscriber_topic1_topic2.c")
  target_link_libraries(subscriber_topic1_topic2 cote)
  add_executable(subscriber_topic1 "examples/pubsub_topics/subscriber_topic1.c")
  target_link_libraries(subscriber_topic1 cote)
  add_executable(subscriber_topic2 "examples/pubsub_topics/subscriber_topic2.c")
  target_link_libraries(subscriber_topic2 cote)
  add_executable(subscriber_topics "examples/pubsub_topics/subscriber_topics.c")
  target_link_libraries(subscriber_topics cote)
  add_executable(requester "examples/reqrep/requester.c")
  target_link_libraries(requester cote)
  add_executable(responder "examples/reqrep/responder.c")
  target_link_libraries(responder cote)
endif()

# Installation
set(CMAKE_INSTALL_FULL_LIBDIR lib)
set(CMAKE_INSTALL_FULL_BINDIR bin)
set(CMAKE_INSTALL_FULL_INCLUDEDIR include)
if(EXISTS lib/cJSON)
  install(FILES lib/cJSON/cJSON.h DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
  install(TARGETS cjson
    ARCHIVE DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}"
  )
endif()
if(EXISTS lib/amp)
  install(FILES lib/amp/inc/amp.h DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
  install(TARGETS amp
    ARCHIVE DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}"
  )
endif()
if(EXISTS lib/axon)
  install(FILES lib/axon/inc/axon.h DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
  install(TARGETS axon
    ARCHIVE DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}"
  )
endif()
if(EXISTS lib/discover)
  install(FILES lib/discover/inc/discover.h DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
  install(TARGETS discover
    ARCHIVE DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}"
  )
endif()
install(FILES inc/cote.h DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
install(TARGETS cote
  ARCHIVE DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_BINDIR}"
  INCLUDES DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}"
)
if(ENABLE_COTE_EXAMPLES)
  install(TARGETS monitor publisher subscriber publisher_namespace1 subscriber_namespace1 publisher_topic1_topic2 subscriber_topic1_topic2 subscriber_topic1 subscriber_topic2 subscriber_topics requester responder
    ARCHIVE DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}"
  )
endif()

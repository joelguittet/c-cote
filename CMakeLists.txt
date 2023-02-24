# @file      CMakeLists.txt
# @brief     c-cote library CMakeLists file
#
# MIT License
#
# Copyright (c) 2021-2023 joelguittet and c-cote contributors
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# CMake minimum version
cmake_minimum_required(VERSION 3.14)

# Project name
project(c-cote)

# Project version
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION PROJECT_VER LIMIT_COUNT 1)
string(REPLACE "." ";" PROJECT_VER_LIST ${PROJECT_VER})
list(GET PROJECT_VER_LIST 0 PROJECT_VER_MAJOR)
list(GET PROJECT_VER_LIST 1 PROJECT_VER_MINOR)
list(GET PROJECT_VER_LIST 2 PROJECT_VER_PATCH)

# Use GNU installation directories
include(GNUInstallDirs)

# Additional flags
set(c_flags "${c_flags} -Os -ffunction-sections -Wall -fPIC")
set(linker_flags "${linker_flags} -Wl,-gc-sections")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")

# Definitions
add_definitions(-DCOTE_EXPORT_SYMBOLS -DCOTE_API_VISIBILITY)

# CMake subdirectories
if(NOT TARGET amp)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/amp/CMakeLists.txt)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/amp)
    endif()
endif()
if(NOT TARGET axon)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/axon/CMakeLists.txt)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/axon)
    endif()
endif()
if(NOT TARGET discover)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/discover/CMakeLists.txt)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/discover)
    endif()
endif()

# List of sources
file(GLOB_RECURSE src ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

# Add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/amp/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/amp/include)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/amp/lib/cJSON/cJSON.h)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/amp/lib/cJSON)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/axon/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/axon/include)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/discover/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/discover/include)
endif()

# Creation of the library
add_library(cote SHARED ${src})

# Link the library with the wanted libraries
target_link_libraries(cote discover axon amp cjson pthread rt)

# Properties of the library
set_target_properties(cote
    PROPERTIES
    SOVERSION "${PROJECT_VER_MAJOR}"
    VERSION "${PROJECT_VER_MAJOR}.${PROJECT_VER_MINOR}.${PROJECT_VER_PATCH}"
)

# Creation of the examples binaries
option(ENABLE_COTE_EXAMPLES "Enable building cote examples" OFF)
if(ENABLE_COTE_EXAMPLES)
    add_executable(monitor ${CMAKE_CURRENT_SOURCE_DIR}/examples/monitor/monitor.c)
    target_link_libraries(monitor cote)
    add_executable(publisher ${CMAKE_CURRENT_SOURCE_DIR}/examples/pubsub/publisher.c)
    target_link_libraries(publisher cote)
    add_executable(subscriber ${CMAKE_CURRENT_SOURCE_DIR}/examples/pubsub/subscriber.c)
    target_link_libraries(subscriber cote)
    add_executable(publisher_namespace1 ${CMAKE_CURRENT_SOURCE_DIR}/examples/pubsub_namespace/publisher_namespace1.c)
    target_link_libraries(publisher_namespace1 cote)
    add_executable(subscriber_namespace1 ${CMAKE_CURRENT_SOURCE_DIR}/examples/pubsub_namespace/subscriber_namespace1.c)
    target_link_libraries(subscriber_namespace1 cote)
    add_executable(publisher_topic1_topic2 ${CMAKE_CURRENT_SOURCE_DIR}/examples/pubsub_topics/publisher_topic1_topic2.c)
    target_link_libraries(publisher_topic1_topic2 cote)
    add_executable(subscriber_topic1_topic2 ${CMAKE_CURRENT_SOURCE_DIR}/examples/pubsub_topics/subscriber_topic1_topic2.c)
    target_link_libraries(subscriber_topic1_topic2 cote)
    add_executable(subscriber_topic1 ${CMAKE_CURRENT_SOURCE_DIR}/examples/pubsub_topics/subscriber_topic1.c)
    target_link_libraries(subscriber_topic1 cote)
    add_executable(subscriber_topic2 ${CMAKE_CURRENT_SOURCE_DIR}/examples/pubsub_topics/subscriber_topic2.c)
    target_link_libraries(subscriber_topic2 cote)
    add_executable(subscriber_topics ${CMAKE_CURRENT_SOURCE_DIR}/examples/pubsub_topics/subscriber_topics.c)
    target_link_libraries(subscriber_topics cote)
    add_executable(requester ${CMAKE_CURRENT_SOURCE_DIR}/examples/reqrep/requester.c)
    target_link_libraries(requester cote)
    add_executable(responder ${CMAKE_CURRENT_SOURCE_DIR}/examples/reqrep/responder.c)
    target_link_libraries(responder cote)
endif()

# Installation
set(CMAKE_INSTALL_FULL_LIBDIR lib)
set(CMAKE_INSTALL_FULL_BINDIR bin)
set(CMAKE_INSTALL_FULL_INCLUDEDIR include)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/cote.h DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
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

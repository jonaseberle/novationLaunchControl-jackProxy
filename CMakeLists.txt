cmake_minimum_required(VERSION 2.8)

project(jack2-novation-lauch-control-xl C CXX)
add_executable(${PROJECT_NAME} "jack2-novation-lauch-control-xl.c")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")


enable_language(CXX)
enable_language(C)

if(CMAKE_VERSION VERSION_LESS 3.0.0)
    set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "library install dir (lib)")
    set(CMAKE_INSTALL_INCLUDEDIR "include" CACHE PATH "header base install dir (include)")
    set(CMAKE_INSTALL_BINDIR "bin" CACHE PATH "user executables (bin)")
else()
    cmake_policy(SET CMP0042 NEW)
    cmake_policy(SET CMP0046 NEW)
    include(GNUInstallDirs)
endif()

#set(CMAKE_FIND_LIBRARY_PREFIXES 1)
#set(CMAKE_CXX_FLAGS "-ljack -pthread")

find_package(JACK REQUIRED)
include_directories(
    SYSTEM
    ${JACK_INCLUDES}
)


#if (JACK_FOUND)
#   include_directories(${JACK_INCLUDE_DIRS})
#   add_definitions(${JACK_DEFINITIONS})
#endif ()

#add_library (jack jack/jack.h)

find_package(Threads)

set(LIBSOUNDIO_LIBS
    ${JACK_LIBRARY}
    ${PULSEAUDIO_LIBRARY}
    ${ALSA_LIBRARIES}
    ${COREAUDIO_LIBRARY}
    ${COREFOUNDATION_LIBRARY}
    ${AUDIOUNIT_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Werror -pedantic")
set(CMAKE_C_CFLAGS "-Wall -ljack -pthread")
add_definitions(${JACK_DEFINITIONS})
target_link_libraries(jack2-novation-lauch-control-xl ${JACK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

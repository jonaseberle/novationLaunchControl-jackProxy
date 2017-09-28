cmake_minimum_required(VERSION 3.0)

enable_language(CXX)
project(novationLaunchControl-jackProxy
    LANGUAGES CXX
    VERSION 1.0.0
)
add_executable(${PROJECT_NAME}
    "src/novationLaunchControl-jackProxy.cpp"
)
set_target_properties(novationLaunchControl-jackProxy
    PROPERTIES
    DEBUG_POSTFIX "_d"
    RELWITHDEBINFO_POSTFIX "_rd"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# JACK
find_package(JACK REQUIRED)
add_definitions(${JACK_DEFINITIONS})
include_directories(
    SYSTEM
    ${JACK_INCLUDES}
)

#threads
find_package(Threads)

target_link_libraries(novationLaunchControl-jackProxy ${JACK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")


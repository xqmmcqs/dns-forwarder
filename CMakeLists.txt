cmake_minimum_required(VERSION 3.27.1)
project(dns-forwarder VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

aux_source_directory(${PROJECT_SOURCE_DIR}/src SOURCE_FILES)
add_executable(dns-forwarder ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME}
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/include
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

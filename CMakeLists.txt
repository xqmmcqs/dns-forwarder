cmake_minimum_required(VERSION 3.22.1)
project(dns-forwarder VERSION 0.1.0 LANGUAGES C CXX)
set (CMAKE_CXX_STANDARD 17)

aux_source_directory(${PROJECT_SOURCE_DIR}/src SOURCE_FILES)
add_executable(dns-forwarder ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME}
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/include
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

include(CTest)
enable_testing()

find_package(Python COMPONENTS Interpreter REQUIRED)

set(VENV_DIR "${CMAKE_SOURCE_DIR}/.venv")
add_custom_command(OUTPUT ${VENV_DIR}
    COMMAND ${Python_EXECUTABLE} -m venv ${VENV_DIR}
    COMMENT "Creating virtual environment"
)
add_custom_target(venv ALL
    DEPENDS ${VENV_DIR}
    COMMAND ${VENV_DIR}/bin/python -m pip install dnspython
    COMMENT "Installing dnspython package in virtual environment"
)

set(PYTHON_TEST "${VENV_DIR}/bin/python")

add_test(NAME qtype
    COMMAND ${PYTHON_TEST} ${PROJECT_SOURCE_DIR}/test/test_qtype.py
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_test(NAME ipv6
    COMMAND ${PYTHON_TEST} ${PROJECT_SOURCE_DIR}/test/test_ipv6.py
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_test(NAME tcp
    COMMAND ${PYTHON_TEST} ${PROJECT_SOURCE_DIR}/test/test_tcp.py
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
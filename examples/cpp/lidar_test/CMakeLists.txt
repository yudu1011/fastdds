# Copyright 2024 Proyectos y Sistemas de Mantenimiento SL (eProsima).
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.20)

project(fastdds_lidar_test_example VERSION 1 LANGUAGES CXX)

# set(CMAKE_FIND_ROOT_PATH /home/byd/Fast-DDS/Fast-CDR/build
# 			/home/byd/Fast-DDS/foonathan_memory_vendor/build/foo_mem_ext_prj_install
#             		/home/byd/Fast-DDS/Fast-DDS/build
#             		/usr
#             		/usr/bin
#             		/home/byd/Fast-DDS/install
#             		/home/byd/arm-Fast-DDS/asio-1.30.2)
# set(ENV{LD_LIBRARY_PATH} "/usr/local/lib:/home/byd/Fast-DDS/install/lib:$ENV{LD_LIBRARY_PATH}")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};/home/byd/Fast-DDS/install")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};/usr/local/lib")

# include_directories("${CMAKE_FIND_ROOT_PATH}/usr/include")
# include_directories("${CMAKE_FIND_ROOT_PATH}/include/fastcdr")
# include_directories("${CMAKE_FIND_ROOT_PATH}/usr/local/include")
# include_directories("${CMAKE_FIND_ROOT_PATH}/include")
# include_directories("${CMAKE_FIND_ROOT_PATH}/lib")

#set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
#set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
# Find requirements
if(NOT fastcdr_FOUND)
    find_package(fastcdr 2 REQUIRED)
endif()

if(NOT fastdds_FOUND)
    find_package(fastdds 3 REQUIRED)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# #Check C++11
include(CheckCXXCompilerFlag)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    check_cxx_compiler_flag(-std=c++11 SUPPORTS_CXX11)
    check_cxx_compiler_flag(-std=c++11 SUPPORTS_CXX11)
    if(NOT SUPPORTS_CXX11)
        message(FATAL_ERROR "Compiler doesn't support C++11")
    endif()
endif()

message(STATUS "Configuring LIDAR_TEST example...")

file(GLOB LIDAR_TEST_SOURCES_CXX "*.cxx")
file(GLOB LIDAR_TEST_SOURCES_CPP "*.cpp")

add_executable(lidar_test ${LIDAR_TEST_SOURCES_CXX} ${LIDAR_TEST_SOURCES_CPP})
target_compile_definitions(lidar_test PRIVATE
    $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
    $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
    $<$<BOOL:${SHM_TRANSPORT_DEFAULT}>:SHM_TRANSPORT_BUILTIN> # Enable SHM as built-in transport
)

target_link_libraries(lidar_test fastdds fastcdr)
install(TARGETS lidar_test
    RUNTIME DESTINATION ${DATA_INSTALL_DIR}/fastdds/examples/cpp/lidar_test/${BIN_INSTALL_DIR})

# # Copy the XML files over to the build directory
file(GLOB_RECURSE XML_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.xml)
# for each xml file detected
foreach(XML_FILE_COMPLETE_PATH ${XML_FILES})
    # obtain the file name
    get_filename_component(XML_FILE ${XML_FILE_COMPLETE_PATH} NAME_WE)
    # copy the file from src to build folders
    configure_file(
        ${XML_FILE_COMPLETE_PATH}                   # from full src path
        ${CMAKE_CURRENT_BINARY_DIR}/lidartest_profile.xml # to relative build path
        COPYONLY)
    install(FILES ${XML_FILE_COMPLETE_PATH}
        DESTINATION ${DATA_INSTALL_DIR}/fastdds/examples/cpp/lidar_test/${BIN_INSTALL_DIR})
endforeach()

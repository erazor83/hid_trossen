CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(hid_trossen)
SET(VERSION "1.1.0")

SET(Boost_USE_STATIC_LIBS        OFF)
SET(Boost_USE_MULTITHREADED      OFF)


SET(CMAKE_CXX_FLAGS "-I ${CMAKE_CURRENT_BINARY_DIR}/src -I ${CMAKE_CURRENT_SOURCE_DIR}/src -g -Wall")

FIND_PACKAGE(Boost COMPONENTS system REQUIRED)
FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED)


ADD_SUBDIRECTORY(src)


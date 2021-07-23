cmake_minimum_required(VERSION 3.0.2)
project(iort_lib)

## Compile as C++11, supported in ROS Kinetic and newer
add_compile_options(-fPIC -shared)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

## Find catkin macros and libraries
find_package(catkin REQUIRED)

## System dependencies are found with CMake's conventions
find_package(jsoncpp REQUIRED)

## Local dependencies
add_subdirectory(dep)

###################################
## catkin specific configuration ##
###################################
## The catkin_package macro generates cmake config files for your package
## Declare things to be passed to dependent projects
## INCLUDE_DIRS: uncomment this if your package contains header files
## LIBRARIES: libraries you create in this project that dependent projects also need
## CATKIN_DEPENDS: catkin_packages dependent projects also need
## DEPENDS: system dependencies of this project that dependent projects also need
catkin_package(
    INCLUDE_DIRS lib
    LIBRARIES iort_lib
#  CATKIN_DEPENDS other_catkin_pkg
#  DEPENDS system_lib
)

###########
## Build ##
###########
set(iort_LIBS
    lib/${PROJECT_NAME}/iort.hpp
)

set(iort_HDRS
    inc/config.h
)

set(iort_SRCS
    src/${PROJECT_NAME}/iort.cpp
)

## Specify additional locations of header files
## Your package locations should be listed before other locations
include_directories(
    inc
    lib
    dep
    ${catkin_INCLUDE_DIRS}
)

## Declare a C++ library
add_library(${PROJECT_NAME}
    ${iort_LIBS}
    ${iort_HDRS}
    ${iort_SRCS}
)

target_link_libraries(${PROJECT_NAME} 
    jsoncpp_lib
    paho-mqttpp3
    paho-mqtt3as
    cpr::cpr
    ${catkin_LIBRARIES}
)

## Copy Certificates into build directory
FILE(COPY certs/aws-root-ca.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
FILE(COPY certs/certificate.pem.crt DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
FILE(COPY certs/private.pem.key DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

#############
## Install ##
#############

install(DIRECTORY lib/iort_lib/
    DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
    FILES_MATCHING PATTERN "*.hpp"
)

## Mark libraries for installation
## See http://docs.ros.org/melodic/api/catkin/html/howto/format1/building_libraries.html
install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    RUNTIME DESTINATION ${CATKIN_GLOBAL_BIN_DESTINATION}
)

#############
## Testing ##
#############

## Add gtest based cpp test target and link libraries
# catkin_add_gtest(${PROJECT_NAME}-test test/test_iort_lib.cpp)
# if(TARGET ${PROJECT_NAME}-test)
#   target_link_libraries(${PROJECT_NAME}-test ${PROJECT_NAME})
# endif()

## Add folders to be run by python nosetests
# catkin_add_nosetests(test)

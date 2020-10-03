cmake_minimum_required( VERSION 3.1 )
project( GlitterBrowserDetector )

set( CMAKE_CXX_STANDARD 11 )

if( NOT CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE Release )
endif( NOT CMAKE_BUILD_TYPE )

set( CMAKE_CXX_FLAGS_RELEASE )
add_compile_options( -g -Wall -O3 )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "../bin" )

# include apriltag library in build
include_directories( apriltag )
file( GLOB_RECURSE APRILTAG_SRCS apriltag/*.o )

# include glitter library in build
include_directories( glitter )
file( GLOB GLITTER_SRCS glitter/*.c )

# compile apriltag_demo example
add_executable(apriltag_demo apriltag_demo.c)
target_link_libraries(apriltag_demo ${APRILTAG_SRCS} )

# compile apriltag_quads example
add_executable(apriltag_quads apriltag_quads.c ${GLITTER_SRCS})
target_link_libraries(apriltag_quads ${APRILTAG_SRCS} )

# find OpenCV, if exists
find_package(OpenCV QUIET)
if (OpenCV_FOUND)
    include_directories(${OpenCV_INCLUDE_DIRS})

    # compile opencv_demo example
    add_executable(opencv_demo opencv_demo.cpp)
    target_link_libraries(opencv_demo ${APRILTAG_SRCS} ${OpenCV_LIBS} )
else()
    message("OpenCV not found. Examples that use OpenCV will not be compiled!")
endif()

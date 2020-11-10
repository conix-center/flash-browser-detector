cmake_minimum_required( VERSION 3.1 )
project( GlitterBrowserDetector )

set( CMAKE_CXX_STANDARD 11 )

if( NOT CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE Release )
endif( NOT CMAKE_BUILD_TYPE )

set( CMAKE_CXX_FLAGS_RELEASE )
add_compile_options( -g -Wall -O3 )

set( BIN ../bin )
file( MAKE_DIRECTORY ${BIN} )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN} )

# include apriltag library in build
include_directories( apriltag )
file( GLOB_RECURSE APRILTAG_SRCS apriltag/*.c )

# remove pywrap and unnecessary tag families
list(REMOVE_ITEM APRILTAG_SRCS ${CMAKE_SOURCE_DIR}/apriltag/apriltag_pywrap.c)
list(REMOVE_ITEM APRILTAG_SRCS ${CMAKE_SOURCE_DIR}/apriltag/tagCircle49h12.c)
list(REMOVE_ITEM APRILTAG_SRCS ${CMAKE_SOURCE_DIR}/apriltag/tagCustom48h12.c)
list(REMOVE_ITEM APRILTAG_SRCS ${CMAKE_SOURCE_DIR}/apriltag/tagStandard52h13.c)

add_library( apriltag STATIC ${APRILTAG_SRCS} )

# include glitter library in build
include_directories( glitter )
file( GLOB GLITTER_SRCS glitter/*.c )

add_library( glitter STATIC ${GLITTER_SRCS} )

set( EXAMPLES_DIR examples )

# compile apriltag_demo example
add_executable( apriltag_demo ${EXAMPLES_DIR}/apriltag_demo.c )
target_link_libraries( apriltag_demo apriltag )

# compile apriltag_quads example
add_executable( apriltag_quads ${EXAMPLES_DIR}/apriltag_quads.c )
target_link_libraries( apriltag_quads apriltag glitter )

# compile lightanchor_demo example
add_executable( lightanchor_demo ${EXAMPLES_DIR}/lightanchor_demo.c )
target_link_libraries( lightanchor_demo apriltag glitter )

# find OpenCV, if exists
find_package( OpenCV QUIET )
if ( OpenCV_FOUND )
    include_directories( ${OpenCV_INCLUDE_DIRS} )

    # compile opencv_demo example
    add_executable( opencv_demo ${EXAMPLES_DIR}/opencv_demo.cpp )
    target_link_libraries( opencv_demo ${OpenCV_LIBS} apriltag )

    # compile webcam_quads example
    add_executable( webcam_quads ${EXAMPLES_DIR}/webcam_quads.cpp)
    target_link_libraries( webcam_quads ${OpenCV_LIBS} apriltag glitter )

    # compile webcam_lightanchors example
    add_executable( webcam_lightanchors ${EXAMPLES_DIR}/webcam_lightanchors.cpp)
    target_link_libraries( webcam_lightanchors ${OpenCV_LIBS} apriltag glitter )
else()
    message( "OpenCV not found. Examples that use OpenCV will not be compiled!" )
endif()

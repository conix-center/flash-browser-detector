cmake_minimum_required( VERSION 3.1 )
project( GlitterBrowserDetector )

set( CMAKE_CXX_STANDARD 11 )

if( NOT CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE Release )
endif( NOT CMAKE_BUILD_TYPE )

set( CMAKE_CXX_FLAGS_RELEASE )
add_compile_options( -g -Wall -O3 )

# include apriltag library in build
include_directories( ../apriltag )
file( GLOB_RECURSE APRILTAG_SRCS ../apriltag/*.c )

# remove pywrap and unnecessary tag families
list(REMOVE_ITEM APRILTAG_SRCS ${CMAKE_SOURCE_DIR}/../apriltag/apriltag_pywrap.c)
list(REMOVE_ITEM APRILTAG_SRCS ${CMAKE_SOURCE_DIR}/../apriltag/tagCircle49h12.c)
list(REMOVE_ITEM APRILTAG_SRCS ${CMAKE_SOURCE_DIR}/../apriltag/tagCustom48h12.c)
list(REMOVE_ITEM APRILTAG_SRCS ${CMAKE_SOURCE_DIR}/../apriltag/tagStandard52h13.c)

# include glitter library in build
include_directories( ../glitter )
file( GLOB GLITTER_SRCS ../glitter/*.c )

add_executable( glitter_wasm glitter_wasm.c ${APRILTAG_SRCS} ${GLITTER_SRCS} )
set_target_properties( glitter_wasm PROPERTIES LINK_FLAGS "-s EXPORT_NAME='GlitterWASM' -s NODERAWFS=0 -s MODULARIZE=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS='['_malloc', '_free']' -s EXTRA_EXPORTED_RUNTIME_METHODS='['ccall', 'cwrap', 'getValue', 'setValue']' -s WASM=1" )

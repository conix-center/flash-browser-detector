cmake_minimum_required( VERSION 3.1 )
project( GlitterBrowserDetector )

set( CMAKE_CXX_STANDARD 11 )

if( NOT CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE Release )
endif( NOT CMAKE_BUILD_TYPE )

set( CMAKE_CXX_FLAGS_RELEASE )
add_compile_options( -Wall -O3 )

# include apriltag library in build
include_directories( ../apriltag )
file( GLOB_RECURSE APRILTAG_SRCS ../apriltag/*.c )

# remove pywrap and unnecessary tag families
list(REMOVE_ITEM APRILTAG_SRCS ${CMAKE_SOURCE_DIR}/../apriltag/apriltag_pywrap.c)
list(REMOVE_ITEM APRILTAG_SRCS ${CMAKE_SOURCE_DIR}/../apriltag/tagCircle49h12.c)
list(REMOVE_ITEM APRILTAG_SRCS ${CMAKE_SOURCE_DIR}/../apriltag/tagCustom48h12.c)
list(REMOVE_ITEM APRILTAG_SRCS ${CMAKE_SOURCE_DIR}/../apriltag/tagStandard52h13.c)

add_library( apriltag STATIC ${APRILTAG_SRCS} )

# include glitter library in build
include_directories( ../glitter )
file( GLOB GLITTER_SRCS ../glitter/*.c )

add_library( glitter STATIC ${GLITTER_SRCS} )

add_executable( glitter_wasm glitter_wasm.c )
target_link_libraries( glitter_wasm apriltag glitter )
set_target_properties( glitter_wasm PROPERTIES LINK_FLAGS "-s EXPORT_NAME='GlitterWASM' -s MODULARIZE=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS='['_malloc', '_free']' -s EXTRA_EXPORTED_RUNTIME_METHODS='['cwrap', 'getValue', 'setValue']' -s WASM=1" )

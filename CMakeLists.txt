project( shp2gpx )

cmake_minimum_required( VERSION 3.16.0 )

include_directories( "." ".." )

set( SOURCES shp2gpx.cpp )
add_executable( shp2gpx ${SOURCES} )

if( WIN32 )

  include_directories("${PROJECT_SOURCE_DIR}/lib/shapelib/include")
  target_link_libraries( shp2gpx ${PROJECT_SOURCE_DIR}/lib/shapelib/lib/libshp.dll.a )

endif( WIN32 )

if( UNIX )

  include_directories( "/usr/include" "/usr/local/include" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14" )

  find_library( SHAPELIB shp REQUIRED )
  target_link_libraries( shp2gpx ${SHAPELIB} )

endif( UNIX )


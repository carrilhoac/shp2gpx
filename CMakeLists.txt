project( shp2gpx )

cmake_minimum_required( VERSION 3.16.0 )

include_directories( "." ".." )

set( SOURCES shp2gpx.cpp )
add_executable( shp2gpx ${SOURCES} )

if( WIN32 )

  find_package( args REQUIRED )

  find_package( shapelib COMPONENTS shp REQUIRED )

  include_directories( ${args_INCLUDE_DIRS} ${shapelib_INCLUDE_DIRS} )
  link_directories( ${shapelib_LIBRARY_DIRS} )
  target_link_libraries( shp2gpx ${shapelib_LIBRARIES} )

endif( WIN32 )

if( UNIX )

  include_directories( "/usr/include" "/usr/local/include" ) # Linux default include dirs
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14" )

  find_library( SHAPELIB shp REQUIRED )
  target_link_libraries( shp2gpx ${SHAPELIB} )

endif( UNIX )

install( TARGETS shp2gpx DESTINATION bin )

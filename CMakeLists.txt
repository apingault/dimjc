#################################################
# cmake file for building dimjc package
# @author Eté Rémi
CMAKE_MINIMUM_REQUIRED( VERSION 2.6 FATAL_ERROR )
#################################################

# project name
PROJECT( DIMJC )

SET( ${PROJECT_NAME}_VERSION_MAJOR 1 )
SET( ${PROJECT_NAME}_VERSION_MINOR 2 )
SET( ${PROJECT_NAME}_VERSION_PATCH 0 )

# --------------- ilcsoft dependencies -----------
LIST( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )

# load default settings
INCLUDE( default_settings )

###############################
# Find Threads or multithreading.
FIND_PACKAGE( Threads REQUIRED )

###############################
# Find DIM package
FIND_PACKAGE( DIM REQUIRED SHARED )
FIND_PACKAGE( JSONCPP REQUIRED )

INCLUDE_DIRECTORIES( ${DIM_INCLUDE_DIRS} )
LINK_LIBRARIES( ${DIM_LIBRARIES} )
ADD_DEFINITIONS ( ${DIM_DEFINITIONS} ) 

INCLUDE_DIRECTORIES( ${JSONCPP_INCLUDE_DIRS} )
LINK_LIBRARIES( ${JSONCPP_LIBRARIES} )
ADD_DEFINITIONS ( ${JSONCPP_DEFINITIONS} ) 

IF( CMAKE_USE_PTHREADS_INIT )
    ADD_DEFINITIONS( -lpthread )
ENDIF()

###############################
# Export ${PROJECT_NAME}_DEPENDS* to ${PROJECT_NAME}Config.cmake
SET( ${PROJECT_NAME}_DEPENDS_INCLUDE_DIRS  ${DIM_INCLUDE_DIRS} ${JSONCPP_INCLUDE_DIRS} )
SET( ${PROJECT_NAME}_DEPENDS_LIBRARY_DIRS  ${DIM_LIBRARY_DIRS} ${JSONCPP_LIBRARY_DIRS} )
SET( ${PROJECT_NAME}_DEPENDS_LIBRARIES     ${DIM_LIBRARIES}    ${JSONCPP_LIBRARIES}    )

###############################
# Generate DIMJCConfig.h 
CONFIGURE_FILE( "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.h.cmake.in"
    "${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}Config.h" @ONLY )

###############################
# include directories
INCLUDE_DIRECTORIES( include )

# require proper c++
ADD_DEFINITIONS( "-pedantic -Wunused-value -O2" )
ADD_DEFINITIONS("-Wno-long-long -Wreturn-type")

SET(
    SRC_FILES
    src/DimJobControl.cc
    src/DimJobInterface.cc
    src/fileTailer.cxx
)

ADD_SHARED_LIBRARY( dimjc ${SRC_FILES} )

INSTALL( 
    TARGETS dimjc
    LIBRARY DESTINATION lib
)

ADD_EXECUTABLE( dimjc_dqm_daemon src/djc.cxx )
TARGET_LINK_LIBRARIES( dimjc_dqm_daemon dimjc )

INSTALL( 
    TARGETS dimjc_dqm_daemon
    RUNTIME DESTINATION bin
)

install(
    PROGRAMS etc/dimjcdqmd
    DESTINATION bin
 )
###############################
# display some variables and write them to cache
DISPLAY_STD_VARIABLES()

###############################
# generate and install following configuration files
GENERATE_PACKAGE_CONFIGURATION_FILES( ${PROJECT_NAME}Config.cmake ${PROJECT_NAME}ConfigVersion.cmake ${PROJECT_NAME}LibDeps.cmake )


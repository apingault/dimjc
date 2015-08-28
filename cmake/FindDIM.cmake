# - Try to find DIM include dirs and libraries
# This module finds the DIM library and includes. 
# Author: Barthelemy von Haller
#
# There are two components : STATIC and SHARED (uppercase!). 
# If none is indicated, we return the shared library. 
# Same thing if both are indicated.
# Note that the first call to find_package(DIM) will decide what
# component to choose. The subsequent calls are ignored. This is
# especially important if you need a package that itself needs DIM. 
# Be sure to call find_package(DIM xxx) before calling 
# find_package(package_depending_on_dim). 
#
# Users can overwrite DIM_LIBRARY in ccmake to force the use of 
# a different DIM library than the one found by this script.
# 
# This script will set the following variables:
#     DIM_FOUND - System has DIM
#     DIM_INCLUDE_DIRS - The DIM include directories
#     DIM_LIBRARIES - The libraries needed to use DIM
#
# Usage examples: 
#  find_package(DIM REQUIRED SHARED)
#  find_package(DIM)

# Initialization
include(FindPackageHandleStandardArgs)
UNSET(DIM_INCLUDE_DIRS)
UNSET(DIM_LIBRARIES)
SET(DIM_FOUND FALSE) 

# Warn or bail out if "SHARED" and "STATIC" have both been requested, and choose shared.
# Default to shared if no component specified
IF( DIM_FIND_REQUIRED_STATIC AND DIM_FIND_REQUIRED_SHARED)

    MESSAGE( WARNING "Two incompatible components specified : static and shared. We are going to ignore the static component.")
    LIST(REMOVE_ITEM DIM_FIND_COMPONENTS static)
    UNSET(DIM_FIND_REQUIRED_STATIC) 

ENDIF()

IF(NOT DIM_FIND_COMPONENTS)
    LIST(APPEND DIM_FIND_COMPONENTS shared)
    SET(DIM_FIND_REQUIRED_SHARED TRUE)
ENDIF(NOT DIM_FIND_COMPONENTS)


# Check for DIMDIR
IF( NOT DIMDIR )
    SET( DIMDIR "/opt/dim" )
ENDIF()

# set include dirs
SET( DIM_INCLUDE_DIR ${DIMDIR}/dim )
LIST(APPEND DIM_INCLUDE_DIRS ${DIM_INCLUDE_DIR} )

SET( DIMLIB_DIR linux)
IF( APPLE )
    SET( DIMLIB_DIR darwin ) 
ENDIF()


IF( NOT EXISTS "${DIM_INCLUDE_DIR}")
    IF( NOT IS_DIRECTORY "${DIM_INCLUDE_DIR}")
    	IF( DIM_FIND_REQUIRED )
    	    MESSAGE( FATAL "Couldn't find DIM includes directory")
     	    MESSAGE( FATAL "Please set the correct dim dir with -DDIMDIR=/path/to/dim")
	ENDIF()
    ENDIF()
ENDIF()


# DIM_FIND_REQUIRED_STATIC and DIM_FIND_REQUIRED_SHARED decide which flavour
# of the library to find. 
IF( DIM_FIND_REQUIRED_STATIC)

    SET(DIM_STATIC_LIBRARY ${DIMDIR}/${DIMLIB_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}dim${CMAKE_STATIC_LIBRARY_SUFFIX})

    IF( NOT EXISTS "${DIM_STATIC_LIBRARY}" AND DIM_FIND_REQUIRED )
    	MESSAGE( FATAL "Couldn't find DIM static library")
    	MESSAGE( FATAL "Please set the correct dim dir with -DDIMDIR=/path/to/dim")
    ENDIF()

    SET(DIM_LIBRARY ${DIM_STATIC_LIBRARY}) 

ELSEIF( DIM_FIND_REQUIRED_SHARED )

    SET(DIM_SHARED_LIBRARY ${DIMDIR}/${DIMLIB_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}dim${CMAKE_SHARED_LIBRARY_SUFFIX})

    IF( NOT EXISTS "${DIM_SHARED_LIBRARY}" AND DIM_FIND_REQUIRED )
    	MESSAGE( FATAL "Couldn't find DIM shared library")
    	MESSAGE( FATAL "Please set the correct dim dir with -DDIMDIR=/path/to/dim")
    ENDIF()

    SET(DIM_LIBRARY ${DIM_SHARED_LIBRARY}) 

ENDIF( DIM_FIND_REQUIRED_STATIC) 

SET(DIM_LIBRARIES ${DIM_LIBRARY})

# handle the QUIETLY and REQUIRED arguments and set DIM_FOUND to TRUE
# if all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DIM DEFAULT_MSG DIM_LIBRARIES DIM_INCLUDE_DIR)

MARK_AS_ADVANCED(DIM_INCLUDE_DIRS DIM_SHARED_LIBRARY DIM_STATIC_LIBRARY DIM_FORCE_STATIC)

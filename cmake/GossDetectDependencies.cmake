# Boost

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)

if (POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost 1.71.0 COMPONENTS system iostreams program_options filesystem timer fiber REQUIRED)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif (Boost_FOUND)


# Multithreading

find_package(Threads REQUIRED)


# ZLIB

find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
	include_directories(ZLIB_INCLUDE_DIRS)
endif (ZLIB_FOUND)


# BZip2

find_package(BZip2 REQUIRED)
if (BZIP2_FOUND)
    include_directories(BZIP2_INCLUDE_DIRS)
endif (BZIP2_FOUND)

# ZIP

find_package(libzip REQUIRED)
if (ZIP_FOUND)
    include_directories(ZIP_INCLUDE_DIRS)
endif(ZIP_FOUND)

if (BUILD_electus)
    # Sqlite

    if (SQLITE_INCLUDE_DIR AND SQLITE_LIBRARIES)
        # in cache already
        set(Sqlite_FIND_QUIETLY TRUE)
    endif (SQLITE_INCLUDE_DIR AND SQLITE_LIBRARIES)

    find_package(PkgConfig)
    pkg_check_modules(PC_SQLITE sqlite3)

    set(SQLITE_DEFINITIONS ${PC_SQLITE_CFLAGS_OTHER})

    find_path(SQLITE_INCLUDE_DIR NAMES sqlite3.h
        PATHS
    )

    find_library(SQLITE_LIBRARIES NAMES sqlite3
        PATHS
    )

    include(FindPackageHandleStandardArgs)
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(Sqlite DEFAULT_MSG SQLITE_INCLUDE_DIR SQLITE_LIBRARIES)
    mark_as_advanced(SQLITE_INCLUDE_DIR SQLITE_LIBRARIES)
    include_directories(SQLITE_INCLUDE_DIR)
endif(BUILD_electus)

################################################################################
### CMakeLists.txt for STOAP (Single-threaded OLAP Aggregation Processor)
###
### Building on Windows has never been tested.
### @author Jerome Meinke <jerome.meinke@googlemail.com>
################################################################################

# print config message
message(STATUS "==== Configuring StOAP (Single-threaded OLAP Aggregation Processor)  ====")

# minimum required cmake version
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# set project name
project("StOAP" CXX C)

set(MAJOR 1)
set(MINOR 0)

# set path for extra cmake modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/Modules/CMake")

# include cmake modules
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckIncludeFiles)
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckTypeSize)
include(CheckHeaderSTDC)
include(CheckCXXSourceCompiles)
include(CMakeDependentOption)
include(CheckIncludeFile)
include(CheckTypeExists)
include(UserFunctions)
include(CppLint)
# include(FindGTest)
include(FindSparsehash)
include(FindGlog)
# include(FindPatl)

# Parsing errors more efficiently extra for eclipse
if(CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")
endif(CMAKE_COMPILER_IS_GNUCC)

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")
endif(CMAKE_COMPILER_IS_GNUCXX)

################################################################################
### check build system
################################################################################

if("${CMAKE_HOST_SYSTEM_PROCESSOR}" MATCHES "x86_64")
    set(ARCH_HINT "x86_64")
    set(LIB_SUFFIX "64")
    set(CMAKE_LIBRARY_ARCHITECTURE "x86_64-linux-gnu") 
    message(STATUS "build system is: ${CMAKE_SYSTEM}-${ARCH_HINT}")
else("${CMAKE_HOST_SYSTEM_PROCESSOR}" MATCHES "x86_64")
    set(ARCH_HINT "i386")
    set(LIB_SUFFIX "")
    set(CMAKE_LIBRARY_ARCHITECTURE "i386-linux-gnu")
    message(STATUS "build system is: ${CMAKE_SYSTEM}-${ARCH_HINT}")
endif("${CMAKE_HOST_SYSTEM_PROCESSOR}" MATCHES "x86_64")

message(STATUS "c compiler is: ${CMAKE_C_COMPILER_ID}")
message(STATUS "c++ compiler is: ${CMAKE_CXX_COMPILER_ID}")

################################################################################
### define variables
################################################################################

# set include files variable 
set(INCLUDES "")

# libs variables
# set(LIBS "gcov")

################################################################################
### checks for programs
################################################################################

# tells the compiler to warn about all the things it finds unusual
check_c_compiler_flag(-Wall WallC)
check_cxx_compiler_flag(-Wall WallCXX)
if(WallC AND WallCXX)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

# check if C compiler doesn't accept -c and -o together
set(CCO "-c -o")
check_c_compiler_flag(${CCO} MINUS_C_MINUS_O)
if(MINUS_C_MINUS_O)
     set(NO_MINUS_C_MINUS_O YES)
else(MINUS_C_MINUS_O)
     set(NO_MINUS_C_MINUS_O NO)
endif(MINUS_C_MINUS_O)

################################################################################
### set project build type
################################################################################

set(PROJECT_BUILD_TYPE "Release" CACHE STRING "Select a build type")
set_property(CACHE PROJECT_BUILD_TYPE PROPERTY STRINGS "Release" "Debug")
set(CMAKE_BUILD_TYPE ${PROJECT_BUILD_TYPE})

# user can change CMAKE_BUILD_TYPE from command-line
if(NOT (CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL Debug))
    message(WARNING "only [Release] or [Debug] build types allowed")
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "use default build type - ${CMAKE_BUILD_TYPE}")
else(NOT (CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL Debug))
    message(STATUS "build type - ${CMAKE_BUILD_TYPE}")
endif(NOT (CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL Debug))
message(STATUS "to change project build type use -DPROJECT_BUILD_TYPE={Release|Debug}") 

if(CMAKE_BUILD_TYPE STREQUAL Release)
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -std=c++0x")
else(CMAKE_BUILD_TYPE STREQUAL Release)
    set(CMAKE_C_FLAGS_DEBUG "-g -fvar-tracking-assignments -Og -D_DEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -fvar-tracking-assignments -Og -D_DEBUG -std=c++0x")
endif(CMAKE_BUILD_TYPE STREQUAL Release)

################################################################################
### define output build paths
################################################################################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
message(STATUS "binary output directory is: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

################################################################################
### check for the Boost library
################################################################################

set(Boost_USE_STATIC_LIBS        ON) # only find static libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.56.0 COMPONENTS thread system regex timer)
if(Boost_FOUND)
	set(INCLUDES ${INCLUDES} ${Boost_INCLUDE_DIRS})
    set(LIBS ${LIBS} boost_thread boost_system boost_regex boost_timer) 
	set(HAVE_LIBBOOST_THREAD YES)
    set(HAVE_BOOST_THREAD_HPP YES)
else(Boost_FOUND)
    message(FATAL_ERROR "Could't find Boost version > version 1.56 on your system!")
endif()

################################################################################
### check for Google GLog
################################################################################

unset(GLOG_FOUND CACHE)
find_package(Glog REQUIRED)
if(GLOG_FOUND)
	set(INCLUDES ${INCLUDES} ${GLOG_INCLUDE_DIRS})
	set(LIBS ${LIBS} glog) 
else(GLOG_FOUND)
    message(FATAL_ERROR "Google Glog not available, please install it!")
endif(GLOG_FOUND)

################################################################################
### check for Google Sparsehash
################################################################################

unset(SPARSEHASH_FOUND CACHE)
find_package(Sparsehash REQUIRED)
if(SPARSEHASH_FOUND)
	set(INCLUDES ${INCLUDES} ${SPARSEHASH_INCLUDE_DIRS})
else(SPARSEHASH_FOUND)
    message(FATAL_ERROR "Google Sparsehash not available, please install it!")
endif(SPARSEHASH_FOUND)

################################################################################
### check for PATL - C++ library for PATRICIA trie
################################################################################

# unset(PATL_FOUND CACHE)
# find_package(Patl REQUIRED)
# if(PATL_FOUND)
#     set(INCLUDES ${INCLUDES} ${PATL_INCLUDE_DIRS})
# else(PATL_FOUND)
#     message(FATAL_ERROR "PATL not available, please install it!")
# endif(PATL_FOUND)

################################################################################
#########################  ADVANCE USER OPTIONS ################################
################################################################################

if(NOT WIN32) # linux only

################################################################################
### set verbose build
################################################################################

set(ENABLE_VERBOSE ON CACHE BOOL "set verbose build")

if(ENABLE_VERBOSE STREQUAL ON)
    set(CMAKE_VERBOSE_MAKEFILE ON)
else(ENABLE_VERBOSE STREQUAL ON)
    set(CMAKE_VERBOSE_MAKEFILE OFF)
endif(ENABLE_VERBOSE STREQUAL ON)

message(STATUS "enable verbose make files is [${CMAKE_VERBOSE_MAKEFILE}]")
message(STATUS "to enable/disable verbose build add -DENABLE_VERBOSE={ON/OFF}")

################################################################################
### enable unittests and doxygen
################################################################################

# enable_testing()
# find_package(GTest REQUIRED)
# include_directories(${GTEST_INCLUDE_DIRS})

find_package(Doxygen)

if(DOXYGEN_FOUND)
    configure_file(${CMAKE_SOURCE_DIR}/Doxyfile.in.cmake ${CMAKE_BINARY_DIR}/doc/Doxyfile @ONLY)
    add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/doc/Doxyfile
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif(DOXYGEN_FOUND)

################################################################################
### checks for system characteristics
################################################################################

# TODO: FIXIT test for DIR_SEPARATOR_CHAR missing
set(SEP_CHAR "/")
set(DIR_SEPARATOR_CHAR "${SEP_CHAR}")
set(DIR_SEPARATOR_STR "${SEP_CHAR}")

################################################################################
### checks for header files
################################################################################

# check stdc headers
CHECK_HEADER_STDC()
check_include_files(signal.h HAVE_SIGNAL_H)
check_include_files(string.h HAVE_STRING_H)
check_include_files(strings.h HAVE_STRINGS_H)
check_include_files(memory.h HAVE_MEMORY_H)
check_include_files(stdlib.h HAVE_STDLIB_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(regex.h HAVE_REGEX_H)

check_include_files(sys/types.h HAVE_SYS_TYPES_H)
if(NOT HAVE_SYS_TYPES_H)
    set(SIZE_T "unsigned int")
endif()

# check headers
check_include_files(dlfcn.h HAVE_DLFCN_H)
check_include_files(inttypes.h HAVE_INTTYPES_H)

check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(sys/stat.h HAVE_SYS_STAT_H)

check_include_files(unistd.h HAVE_UNISTD_H)

################################################################################
### checks for typedefs, structures, and compiler characteristics
################################################################################

check_type_size(suseconds_t HAVE_SUSECONDS_T)
check_type_size(size_t SIZEOF_SIZE_T)
check_type_size(double SIZEOF_DOUBLE)
check_type_size(void* SIZEOF_VOIDP)

# TODO: fix this alignof tests
check_type_size(double ALIGNOF_DOUBLE)
check_type_size(void* ALIGNOF_VOIDP)

# TODO: check if this correct
# check overloading functions for size_t and uint32_t/uint64_t

check_cxx_source_compiles(
    "#include <stdint.h>
     #include <stddef.h>
     void emil (size_t a) {}
     void emil (uint32_t a) {}
     void emil (uint64_t a) {}"
    OVERLOAD_FUNCS_SIZE_T)
if(OVERLOAD_FUNCS_SIZE_T)
    message(STATUS "overloading functions for size_t and uint32_t/uint64_t is possible")
endif()

# overloading functions for size_t and long int
check_cxx_source_compiles(
    "#include <stdint.h>
     #include <stddef.h>
     void emil (size_t a) {}
     void emil (long int a) {}
     emil (1);"
    OVERLOAD_FUNCS_SIZE_T_LONG)
if(OVERLOAD_FUNCS_SIZE_T_LONG)
    message(STATUS "overloading functions for size_t and long int is possible")
endif()

################################################################################
### check whether CC expects -export-dynamic
################################################################################

# TODO: fix check for -export-dynamic
check_library_exists(dl dlopen "" HAVE_DLOPEN)

if(HAVE_DLOPEN)
    set(INCLUDES ${INCLUDES} ${dl_INCLUDE_DIRS})
    set(LIBS ${LIBS} dl)
    link_directories(${dl_LIBRARY_DIRS})
else(HAVE_DLOPEN)
    # use -export-dynamic
endif(HAVE_DLOPEN)

endif(NOT WIN32)

################################################################################
### find subversion client to set right build number
################################################################################

set(REVISION "000")
if(EXISTS ${PROJECT_SOURCE_DIR}/.svn OR EXISTS ${PROJECT_SOURCE_DIR}/../../.svn)
    find_package(Subversion)
    if(Subversion_FOUND)
        Subversion_WC_INFO(${PROJECT_SOURCE_DIR} Source)
        set(REVISION "${Source_WC_LAST_CHANGED_REV}")
        message(STATUS "Current revision number: ${REVISION}")
    else(Subversion_FOUND)
        message(WARNING "Please install subversion to get right revision number!")
        set(REVISION "000")
    endif(Subversion_FOUND)
endif()

################################################################################
### set project information
################################################################################

# palo patch changed automatically
if(ARCH_HINT STREQUAL i386)
    set(ARCH i386)
else(ARCH_HINT STREQUAL i386)
    set(ARCH amd64)
endif(ARCH_HINT STREQUAL i386)

# set package
set(PACKAGE "STOAP")
set(PACKAGE_AUTHOR "Jerome Meinke, University of Freiburg, Germany")
set(PACKAGE_BUGREPORT "jerome.meinke@googlemail.com")
set(PACKAGE_NAME "Single-threaded OLAP Aggregation Processor")
set(PACKAGE_VERSION "${MAJOR}.${MINOR} Rev. ${REVISION}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_TARNAME "${PACKAGE}")
set(PACKAGE_URL "")

################################################################################
### update version and revison out of source
################################################################################

configure_file(${PROJECT_SOURCE_DIR}/Config/build.h.in.cmake ${CMAKE_BINARY_DIR}/Config/build.h)

if(NOT WIN32)

# create config.h file
configure_file(${PROJECT_SOURCE_DIR}/Config/config.h.in.cmake ${CMAKE_BINARY_DIR}/Config/config.h)

endif(NOT WIN32)

if(WIN32)
    # create config_win.h file
    configure_file(${PROJECT_SOURCE_DIR}/Config/config_win.h.in.cmake ${CMAKE_BINARY_DIR}/Config/config_win.h)
endif(WIN32)

# add includes
include_directories(${INCLUDES})
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR}/Config)

# set directory list
set(STOAP_DIR_LIST
    Exceptions
    Engine
    Collections
    InputOutput
    Olap
    System
    Stoap
)


CollectFilesForLib("${STOAP_DIR_LIST}" STOAP_SOURCES)
list(INSERT STOAP_SOURCES 0 stoapMain.cpp Olap.h)


if(WIN32)
    # THIS HAS NEVER BEEN TESTED!
    add_executable(stoapMain ${STOAP_SOURCES})
    target_link_libraries(stoapMain ${LIBS})
    set_target_properties(stoapMain PROPERTIES LINK_FLAGS "/INCREMENTAL:NO /NOLOGO /SUBSYSTEM:CONSOLE /DEBUG")
else(WIN32)
    add_custom_target(clean.${PACKAGE} COMMAND rm -f ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/stoapMain VERBATIM)
    add_executable(stoapMain ${STOAP_SOURCES})
    target_link_libraries(stoapMain ${CMAKE_EXE_LINKER_FLAGS} ${LIBS})
endif(WIN32)

###############################################################################
### cpplint
###############################################################################

# add_style_check_target(cpplint "${STOAP_SOURCES}")

###############################################################################
### summary
###############################################################################

message(STATUS "==================== Summary =========================")
message(STATUS "Configuring: Current version is ${PACKAGE_VERSION}")
message(STATUS "Configuring: Current revison is ${REVISION}")
message(STATUS "Configuring: Current target system is ${CMAKE_SYSTEM}-${ARCH_HINT}")


if(NOT WIN32)
message(STATUS "Configuring: Current project build type is: ${PROJECT_BUILD_TYPE}")

if(PROJECT_BUILD_TYPE STREQUAL Release)
    message(STATUS "Configuring: C_FLAGS: ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE}")
    message(STATUS "Configuring: CXX_FLAGS: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
else(PROJECT_BUILD_TYPE STREQUAL Release)
    message(STATUS "Configuring: C_FLAGS: ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG}")
    message(STATUS "Configuring: CXX_FLAGS: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
endif(PROJECT_BUILD_TYPE STREQUAL Release)

message(STATUS "Configuring: LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "Configuring: LIBS: ${LIBS}")
message(STATUS "Configuring: verbose make files is: ${CMAKE_VERBOSE_MAKEFILE}")
endif(NOT WIN32)
message(STATUS "======================================================")


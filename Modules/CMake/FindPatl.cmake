# - Try to find PATL from
# https://code.google.com/p/patl
#
# Once done this will define
#  PATL_FOUND - System has PATL
#  PATL_INCLUDE_DIRS - The PATL include directories

find_path(PATL_INCLUDE_DIR NAMES patl
        PATHS         ${PATL_ROOT} $ENV{PATL_ROOT} /usr/local/ /usr/ /sw/ /opt/local /opt/csw/ /opt/ ENV CPLUS_INCLUDE_PATH
        PATH_SUFFIXES include/
)

set(PATL_INCLUDE_DIRS ${PATL_INCLUDE_DIR})

# handle the QUIETLY and REQUIRED arguments and set PATL_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Patl DEFAULT_MSG PATL_INCLUDE_DIR)

mark_as_advanced(PATL_INCLUDE_DIR)


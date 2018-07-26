# - Try to find jq
# Once done, this will define
#
#  jq_FOUND - system has jq
#  jq_INCLUDE_DIR - the jq include directories
#  jq_LIBRARY - link these to use jq

include(LibFindMacros)
find_path(jq_INCLUDE_DIR NAMES jq.h jv.h)
find_library(jq_LIBRARY NAMES jq)
libfind_process(jq)

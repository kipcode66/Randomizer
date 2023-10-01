file(GLOB_RECURSE TPR_TP_REL_CPPFILES "${CMAKE_SOURCE_DIR}/externals/libtp_rel/source/*.cpp")
file(GLOB_RECURSE TPR_TP_REL_ASMFILES "${CMAKE_SOURCE_DIR}/externals/libtp_rel/source/*.s")

add_library(tp_rel STATIC "${TPR_TP_REL_CPPFILES}" "${TPR_TP_REL_ASMFILES}")
set_property(TARGET tp_rel PROPERTY COMPILE_FLAGS "-g -c -Oz -Wall ${DEVKITPRO_MACHDEP}")
target_include_directories(tp_rel PUBLIC "${CMAKE_SOURCE_DIR}/externals/libtp_rel/include")
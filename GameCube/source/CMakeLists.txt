file(GLOB_RECURSE srcs CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE asms CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.s" "${CMAKE_CURRENT_SOURCE_DIR}/*.S")
list(APPEND srcs ${asms})
tpr_add_main_module(tpr_main "${srcs}" "${CMAKE_CURRENT_SOURCE_DIR}/include")
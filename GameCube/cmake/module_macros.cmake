# Custom target that will compile all the modules at once
add_custom_target(modules)

set_property(TARGET modules PROPERTY "TPR_MODULE_LAST_ID" 4096)
set(TPR_MODULE_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/rels)
set(TPR_MODULE_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

make_directory(${TPR_MODULE_OUT_DIR})

# Get source from common/rels. They are sources that have to be compiled with every REL module.
file(GLOB_RECURSE TPR_EXTERNAL_REL_CPP CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/externals/libtp_rel/source/*.cpp")
file(GLOB_RECURSE TPR_EXTERNAL_REL_ASM CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/externals/libtp_rel/source/*.s")

list(APPEND TPR_EXTERNAL_REL_SRC ${TPR_EXTERNAL_REL_CPP})
list(APPEND TPR_EXTERNAL_REL_SRC ${TPR_EXTERNAL_REL_ASM})

include_directories(${CMAKE_SOURCE_DIR}/externals/libtp_rel/include/)

# Definition of the functions which sets up REL modules
function(tpr_add_main_module module_name module_srcs)
    if(ARGC GREATER "2")
        set(module_inc_paths ${ARGV2})
    endif()
    if(ARGC GREATER "3")
        set(module_lib_paths ${ARGV3})
    endif()

    add_compile_options(-fdiagnostics-color=always -fno-exceptions -fno-rtti -std=gnu++20 -fno-threadsafe-statics -nostdlib -ffreestanding -ffunction-sections -fdata-sections -g -Oz -Wall -Werror -Wextra -Wshadow -Wno-address-of-packed-member -r -e_prolog -u_prolog -u_epilog -u_unresolved -Wl,--gc-sections -nostdlib -g ${DEVKITPRO_MACHDEP_LIST})
    add_link_options(-r -e_prolog -u_prolog -u_epilog -u_unresolved -Wl,--gc-sections -nostdlib -g ${DEVKITPRO_MACHDEP_LIST})

    get_target_property(TPR_MODULE_ID_COUNTER modules TPR_MODULE_LAST_ID)

    add_executable(${module_name} EXCLUDE_FROM_ALL)
    target_sources(${module_name}
        PUBLIC ${module_srcs} ${TPR_EXTERNAL_REL_SRC})
    set_property(TARGET ${module_name} APPEND PROPERTY LINK_OPTIONS -Wl,-Map,${module_name}.map -Wl,--gc-sections,--gc-keep-exported)
    set_property(TARGET ${module_name} APPEND PROPERTY LINK_OPTIONS ${CMAKE_SOURCE_DIR}/externals/libtp_rel/source/cxx.ld)

    if(module_inc_paths)
        target_include_directories(${module_name} PUBLIC ${module_inc_paths})
    endif()
    if(module_lib_paths)
        target_link_directories(${module_name} ${module_lib_paths})
    endif()

    file(GLOB relative_module_name LIST_DIRECTORIES true RELATIVE ${TPR_MODULE_SRC_DIR} CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR})
    add_custom_command(OUTPUT ${TPR_MODULE_OUT_DIR}/${module_name}.rel
        DEPENDS ${module_name} ${CMAKE_SOURCE_DIR}/assets/${PLATFORM}_${REGION}.lst
        COMMAND ${TPR_ELF2REL_EXE} $<TARGET_FILE:${module_name}> -s ${CMAKE_SOURCE_DIR}/assets/${PLATFORM}_${REGION}.lst --rel-id ${TPR_MODULE_ID_COUNTER} -o ${TPR_MODULE_OUT_DIR}/${module_name}.rel)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${module_name}.lst
        DEPENDS ${module_name} ${CMAKE_SOURCE_DIR}/assets/${PLATFORM}_${REGION}.lst
        COMMAND ${Python3_EXECUTABLE} ${TPR_RELMAPPER_EXE} -o ${CMAKE_CURRENT_BINARY_DIR}/${module_name}.lst -i ${CMAKE_SOURCE_DIR}/assets/${PLATFORM}_${REGION}.lst -s ${TPR_MODULE_ID_COUNTER} $<TARGET_FILE:${module_name}>)
    add_custom_target(${module_name}_rel
        DEPENDS ${TPR_MODULE_OUT_DIR}/${module_name}.rel ${CMAKE_CURRENT_BINARY_DIR}/${module_name}.lst)
    set_property(TARGET ${module_name}_rel PROPERTY TPR_MODULE_FILE ${TPR_MODULE_OUT_DIR}/${module_name}.rel)

    set_property(TARGET modules PROPERTY TPR_MAIN_MODULE_LST ${CMAKE_CURRENT_BINARY_DIR}/${module_name}.lst)
    set_property(TARGET modules PROPERTY TPR_MAIN_MODULE_TRG ${module_name}_rel)
    set_property(TARGET modules PROPERTY TPR_MAIN_MODULE_INC ${module_inc_paths})

    add_dependencies(modules ${module_name}_rel)
    set_property(TARGET modules APPEND PROPERTY TPR_MODULES ${module_name})

    math(EXPR TPR_MODULE_ID_COUNTER "${TPR_MODULE_ID_COUNTER}+1")
    set_property(TARGET modules PROPERTY "TPR_MODULE_LAST_ID" ${TPR_MODULE_ID_COUNTER})
endfunction()


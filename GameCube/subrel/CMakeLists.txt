# This function will setup an individual module to be compiled and added to the dependencies of the "modules" target
function(tpr_add_module module_name module_srcs)
    if(ARGC GREATER "2")
        set(module_inc_paths ${ARGV2})
    endif()
    if(ARGC GREATER "3")
        set(module_lib_paths ${ARGV3})
    endif()

    get_target_property(TPR_MODULE_ID_COUNTER modules TPR_MODULE_LAST_ID)
    get_target_property(TPR_MAIN_MODULE_LST modules TPR_MAIN_MODULE_LST)
    get_target_property(TPR_MAIN_MODULE_TRG modules TPR_MAIN_MODULE_TRG)
    get_target_property(TPR_MAIN_MODULE_INC modules TPR_MAIN_MODULE_INC)

    add_executable(${module_name} EXCLUDE_FROM_ALL)
    target_sources(${module_name}
        PUBLIC ${module_srcs} ${TPR_EXTERNAL_REL_SRC})
    set_property(TARGET ${module_name} APPEND PROPERTY LINK_OPTIONS -Wl,-Map,${module_name}.map -Wl,--gc-sections,--gc-keep-exported)
    set_property(TARGET ${module_name} APPEND PROPERTY LINK_OPTIONS ${CMAKE_SOURCE_DIR}/externals/libtp_rel/source/cxx.ld)

    target_include_directories(${module_name} PUBLIC ${TPR_MAIN_MODULE_INC})

    if(module_inc_paths)
        target_include_directories(${module_name} PUBLIC ${module_inc_paths})
    endif()
    if(module_lib_paths)
        target_link_directories(${module_name} ${module_lib_paths})
    endif()

    file(GLOB relative_module_name LIST_DIRECTORIES true RELATIVE ${TPR_MODULE_SRC_DIR} CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR})
    add_custom_command(OUTPUT ${TPR_MODULE_OUT_DIR}/${module_name}.rel
        DEPENDS ${module_name} ${TPR_MAIN_MODULE_TRG} ${TPR_MAIN_MODULE_LST}
        COMMAND ${TPR_ELF2REL_EXE} $<TARGET_FILE:${module_name}> -s ${TPR_MAIN_MODULE_LST} --rel-id ${TPR_MODULE_ID_COUNTER} -o ${TPR_MODULE_OUT_DIR}/${module_name}.rel)
    add_custom_target(${module_name}_rel
        DEPENDS ${TPR_MODULE_OUT_DIR}/${module_name}.rel)
    set_property(TARGET ${module_name}_rel PROPERTY TPR_MODULE_FILE ${TPR_MODULE_OUT_DIR}/${module_name}.rel)

    add_dependencies(modules ${module_name}_rel)
    set_property(TARGET modules APPEND PROPERTY TPR_MODULES ${module_name})

    math(EXPR TPR_MODULE_ID_COUNTER "${TPR_MODULE_ID_COUNTER}+1")
    set_property(TARGET modules PROPERTY "TPR_MODULE_LAST_ID" ${TPR_MODULE_ID_COUNTER})
endfunction()

add_compile_options(-fdiagnostics-color=always -fno-exceptions -fno-rtti -std=gnu++20 -fno-threadsafe-statics -nostdlib -ffreestanding -ffunction-sections -fdata-sections -g -Oz -Wall -Werror -Wextra -Wshadow -Wno-address-of-packed-member -r -e_prolog -u_prolog -u_epilog -u_unresolved -Wl,--gc-sections -nostdlib -g ${DEVKITPRO_MACHDEP_LIST})
add_link_options(-r -e_prolog -u_prolog -u_epilog -u_unresolved -Wl,--gc-sections,--gc-keep-exported -nostdlib -g ${DEVKITPRO_MACHDEP_LIST})

file(GLOB TPR_SUB_MODULES CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*")

# Include all the submodules
foreach(module ${TPR_SUB_MODULES})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${module})
        message(VERBOSE "Including submodule '${module}'")
        add_subdirectory(${module})
    endif()
endforeach()

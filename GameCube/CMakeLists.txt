cmake_minimum_required(VERSION 3.16)

# Check if the platform is valid
if(NOT DEFINED PLATFORM)
    set(PLATFORM "GCN" CACHE STRING "The platform of the version to be patched (GCN or WII).")
    message(STATUS "PLATFORM not defined, defaulting to ${PLATFORM}")
elseif(NOT ${PLATFORM} STREQUAL "GCN")
    if(NOT ${PLATFORM} STREQUAL "WII")
        message(FATAL_ERROR "PLATFORM has to be either GCN or WII. \"${PLATFORM}\" was given.")
    endif()
endif()

if(${PLATFORM} STREQUAL "GCN")
    # Supported regions for GCN
    list(APPEND TPR_SUPPORTED_REGIONS NTSCU NTSCJ PAL)
    set(TPR_REGION_SHORT_NTSCU "US")
    set(TPR_REGION_SHORT_NTSCJ "JP")
    set(TPR_REGION_SHORT_PAL "EU")
    set(TPR_BANNER "${CMAKE_SOURCE_DIR}/assets/banner.raw")
    set(TPR_ICON "${CMAKE_SOURCE_DIR}/assets/icon.raw")
else()
    # Supported regions for Wii
    list(APPEND TPR_SUPPORTED_REGIONS NTSCU_10 NTSCU_12 NTSCJ PAL)
    set(TPR_REGION_SHORT_NTSCU_10 "US2")
    set(TPR_REGION_SHORT_NTSCU_12 "US2")
    set(TPR_REGION_SHORT_NTSCJ "JP")
    set(TPR_REGION_SHORT_PAL "EU")
    set(TPR_BANNER "${CMAKE_SOURCE_DIR}/assets/wii_banner.raw")
    set(TPR_ICON "${CMAKE_SOURCE_DIR}/assets/icons/tpr_0.raw")
endif()

if(NOT DEFINED REGION)
    # If the region is not defined, set one by default (the first in the supported list)
    list(GET TPR_SUPPORTED_REGIONS 0 REGION_ELEMENT)
    set(REGION ${REGION_ELEMENT} CACHE STRING "The region of the version to be patched.")
    message(STATUS "REGION not defined, defaulting to ${REGION}")
else()
    list(FIND TPR_SUPPORTED_REGIONS ${REGION} REGION_IDX)
    if(REGION_IDX EQUAL -1)
        message(FATAL_ERROR "\"${REGION}\" is not a valid region for the platform ${PLATFORM}")
    endif()
endif()

if(NOT DEFINED VARIANT)
    set(VARIANT "public")
    message(VERBOSE "VARIANT no defined, defaulting to ${VARIANT}")
endif()

if(DEFINED DVD)
    set(DVD_DEV "-DDVD")
endif()

# Start configuring the build directory
message(STATUS "Configuring for ${PLATFORM}_${REGION}...")

# These two lists are used as a map to get the right ISO file name for each version.
set(TPR_VERSIONS GCN_NTSCU GCN_NTSCJ GCN_PAL WII_NTSCU_10 WII_NTSCU_12 WII_NTSCJ WII_PAL)
set(TPR_GAMECODES    GZ2E01    GZ2J01    GZ2P01  RZDE01       RZDE01_2     RZDJ01    RZDP01)

list(FIND TPR_VERSIONS ${PLATFORM}_${REGION} TPR_VERSION)
list(GET TPR_GAMECODES ${TPR_VERSION} TPR_GAMECODE)

list(APPEND CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/cmake
)

# Once we have the platform and region ready, we can check and setup the compiler toolchain.
include(cmake/CheckDevkitPro.cmake)

include_directories(${DEVKITPRO_LIBOGC_INC})
link_directories(${DEVKITPRO_LIBOGC_LIB})

include(cmake/packer.cmake)
include(cmake/elf2rel.cmake)
include(cmake/relmapper.cmake)

project(Randomizer
        VERSION 1.1.0
        DESCRIPTION "Twilight Princess Randomizer"
        HOMEPAGE_URL "tprandomizer.com"
        LANGUAGES C CXX ASM)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# Get Build id
string(TIMESTAMP BUILDID "%Y%m%d%H%M")
set(BUILDID "${BUILDID}${TPR_REGION_SHORT_${REGION}}")

# Set compiler stuff
include_directories(include externals/libtp_rel/include)
add_definitions(-D${PLATFORM}_${REGION} -DPLATFORM_${PLATFORM} -DTP_${TPR_REGION_SHORT_${REGION}} -D_PROJECT_NAME="${CMAKE_PROJECT_NAME}" -D_VERSION="${PROJECT_VERSION}" -D_VERSION_MAJOR="${PROJECT_VERSION_MAJOR}" -D_VERSION_MINOR="${PROJECT_VERSION_MINOR}" -D_VERSION_PATCH="${PROJECT_VERSION_PATCH}" -D_VARIANT="${VARIANT}" -D_BUILDID="${BUILDID}")
add_compile_options(-fdiagnostics-color=always -fvisibility=hidden)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

include(cmake/module_macros.cmake)

add_subdirectory(source)
add_subdirectory(subrel)

add_custom_target(tpr)
add_dependencies(tpr modules)

get_target_property(TPR_MODULES modules TPR_MODULES)

set(TPR_RELS ${TPR_MODULES})
list(TRANSFORM TPR_RELS REPLACE "(.+)" "\\1_rel")

foreach(rel_tgt ${TPR_RELS})
    get_target_property(rel_tgt_file ${rel_tgt} TPR_MODULE_FILE)
    list(APPEND TPR_REL_FILES ${rel_tgt_file})
endforeach()

if(${PLATFORM} STREQUAL "GCN")
    set(TPR_OUTPUT "Randomizer.${TPR_REGION_SHORT_${REGION}}.gci")
    set(tpr_tmp_modules_list ${TPR_REL_FILES})
    list(POP_FRONT tpr_tmp_modules_list tpr_tmp_main_rel)
    list(TRANSFORM tpr_tmp_modules_list PREPEND " --rel ")
    list(JOIN tpr_tmp_modules_list "" tpr_tmp_modules_str)
    string(REPLACE ".rel" ".gci" TPR_MAIN_MODULE_GCI ${tpr_tmp_main_rel})
    string(SUBSTRING ${TPR_GAMECODE} 0 4 TPR_GAMECODE_4)
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${TPR_OUTPUT}
        DEPENDS tpr modules ${TPR_PACKER_EXE} ${CMAKE_SOURCE_DIR}/assets ${TPR_REL_FILES}
        COMMAND ${Python3_EXECUTABLE} ${TPR_PACKER_EXE} ${tpr_tmp_main_rel} "\"Custom REL File\"" "\"Twilight Princess\"" "\"(${TPR_REGION_SHORT_${REGION}}) ${CMAKE_PROJECT_NAME}\"" ${TPR_BANNER} ${TPR_ICON} ${TPR_GAMECODE_4}
        COMMAND ${CMAKE_COMMAND} -E rename "${TPR_MAIN_MODULE_GCI}" "${CMAKE_BINARY_DIR}/${TPR_OUTPUT}")
else()
    set(TPR_OUTPUT "Randomizer.${REGION}.bin")
    # TODO add custom command to generate a wii save file using nanpack
endif()

if(${PLATFORM} STREQUAL "GCN")
    add_custom_target(gci ALL
        DEPENDS ${CMAKE_BINARY_DIR}/${TPR_OUTPUT})
endif()

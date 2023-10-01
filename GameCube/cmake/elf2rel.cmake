# Get Host system distribution
execute_process(
    COMMAND uname -o
    OUTPUT_VARIABLE TPR_HOST_SYS)
string(STRIP "${TPR_HOST_SYS}" TPR_HOST_SYS)

set(TPR_ELF2REL_EXE ${CMAKE_SOURCE_DIR}/bin/elf2rel.exe)

if(TPR_HOST_SYS MATCHES "Linux")
    set(TPR_ELF2REL_EXE ${CMAKE_SOURCE_DIR}/bin/elf2rel)
    message(VERBOSE "We're on Linux [${TPR_HOST_SYS}], found '${TPR_ELF2REL_EXE}'")
else()
    message(VERBOSE "We are not on Linux [${TPR_HOST_SYS}], ${TPR_ELF2REL_EXE}")
endif()

if(NOT TPR_ELF2REL_EXE)
    message(FATAL_ERROR "please put \"elf2rel.exe\" in the bin/ folder")
endif()
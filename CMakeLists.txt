cmake_minimum_required(VERSION 3.12)

set(AVR_MCU atmega324pa CACHE STRING "MCU type")
set(MCU_SPEED 20000000 CACHE STRING "MCU clock speed")
set(AVRDUDE_PROGRAMMER usbasp CACHE STRING "Programmer type")
set(AVRDUDE_PORT "" CACHE STRING "Programmer port (empty for autodetect)")
set(AVRDUDE_BAUD_RATE "" CACHE STRING "Programmer baud rate")

set(PASMO_PATH "" CACHE PATH "Path to pasmo assember's directory")

find_program(AVR_CC avr-gcc REQUIRED)
find_program(AVR_CXX avr-g++ REQUIRED)
find_program(AVR_OBJCOPY avr-objcopy REQUIRED)
find_program(AVRDUDE avrdude REQUIRED)
find_program(PASMO pasmo HINTS ${PASMO_PATH} REQUIRED)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)
set(CMAKE_C_COMPILER ${AVR_CC})
set(CMAKE_CXX_COMPILER ${AVR_CXX})

set(AVRDUDE_COMMON_OPTIONS -c ${AVRDUDE_PROGRAMMER} -p ${AVR_MCU})
if (AVRDUDE_PORT)
    list(APPEND AVRDUDE_COMMON_OPTIONS -P ${AVRDUDE_PORT})
endif()
if (AVRDUDE_BAUD_RATE)
    list(APPEND AVRDUDE_COMMON_OPTIONS -b ${AVRDUDE_BAUD_RATE})
endif()

project(atmega-z80)

function(add_z80_executable EXECUTABLE_NAME ASM_FILE)   
   set(Z80_FILE ${EXECUTABLE_NAME}.z80)
   set (${EXECUTABLE_NAME}_Z80_TARGET ${Z80_FILE} PARENT_SCOPE)
   
   # z80 file
   add_custom_command(
      OUTPUT ${Z80_FILE}
      COMMAND
         ${PASMO} --bin ${CMAKE_CURRENT_SOURCE_DIR}/${ASM_FILE} ${Z80_FILE}
      DEPENDS ${ASM_FILE}
   )
   
   add_custom_target(
      ${EXECUTABLE_NAME}
      ALL
      DEPENDS ${Z80_FILE}
   )
   
endfunction(add_z80_executable)

function(add_avr_executable EXECUTABLE_NAME)
   if(NOT ARGN)
      message(FATAL_ERROR "No source files given for ${EXECUTABLE_NAME}.")
   endif(NOT ARGN)

   # set file names
   set(elf_file ${EXECUTABLE_NAME}.elf)
   set(hex_file ${EXECUTABLE_NAME}.hex)
   set(eeprom_image ${EXECUTABLE_NAME}-eeprom.hex)

   set (${EXECUTABLE_NAME}_ELF_TARGET ${elf_file} PARENT_SCOPE)
   set (${EXECUTABLE_NAME}_HEX_TARGET ${hex_file} PARENT_SCOPE)
   set (${EXECUTABLE_NAME}_EEPROM_TARGET ${eeprom_file} PARENT_SCOPE)
   
   # elf file
   add_executable(${elf_file} EXCLUDE_FROM_ALL ${ARGN})

   set_target_properties(
      ${elf_file}
      PROPERTIES
         COMPILE_FLAGS "-mmcu=${AVR_MCU} -DF_CPU=${MCU_SPEED} -Wall -Os -std=c99"
         LINK_FLAGS "-mmcu=${AVR_MCU} -Wl,--gc-sections -mrelax"
   )

   add_custom_command(
      OUTPUT ${hex_file}
      COMMAND
         ${AVR_OBJCOPY} -j .text -j .data -O ihex ${elf_file} ${hex_file}
      DEPENDS ${elf_file}
   )
   
   # eeprom
   add_custom_command(
      OUTPUT ${eeprom_image}
      COMMAND
         ${AVR_OBJCOPY} -j .eeprom --set-section-flags=.eeprom=alloc,load
            --change-section-lma .eeprom=0 --no-change-warnings
            -O ihex ${elf_file} ${eeprom_image}
      DEPENDS ${elf_file}
   )

   add_custom_target(
      ${EXECUTABLE_NAME}
      ALL
      DEPENDS ${hex_file} ${eeprom_image}
   )

   set_target_properties(
      ${EXECUTABLE_NAME}
      PROPERTIES
         OUTPUT_NAME "${elf_file}"
   )

   # upload - with avrdude
   add_custom_target(
      upload_${EXECUTABLE_NAME}
      ${AVRDUDE} ${AVRDUDE_COMMON_OPTIONS}
         -U flash:w:${hex_file}
      DEPENDS ${hex_file}
      COMMENT "Uploading ${hex_file} to ${AVR_MCU} using ${AVR_PROGRAMMER}"
   )

   # upload eeprom only - with avrdude
   # see also bug http://savannah.nongnu.org/bugs/?40142
   add_custom_target(
      upload_${EXECUTABLE_NAME}_eeprom
      ${AVRDUDE} ${AVRDUDE_COMMON_OPTIONS}
         -U eeprom:w:${eeprom_image}
      DEPENDS ${eeprom_image}
      COMMENT "Uploading ${eeprom_image} to ${AVR_MCU} using ${AVR_PROGRAMMER}"
   )

endfunction(add_avr_executable)

add_custom_target(
    atmega_fuse_init
    ${AVRDUDE} ${AVRDUDE_COMMON_OPTIONS}
        -U lfuse:w:0xF7:m
        -U hfuse:w:0x99:m
        -U efuse:w:0xFF:m
        -U lock:w:0xFF:m
)

add_custom_target(
    atmega_fuse_factory_reset
    ${AVRDUDE} ${AVRDUDE_COMMON_OPTIONS}
        -U lfuse:w:0x62:m
        -U hfuse:w:0x99:m
        -U efuse:w:0xFF:m
        -U lock:w:0xFF:m
)

add_subdirectory(src)

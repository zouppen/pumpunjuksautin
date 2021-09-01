# May work with older, this is my version
cmake_minimum_required(VERSION 3.16)
# Project name
project("Pumpunjuksautin")

# Git integration
find_package(Git)
if(Git_FOUND)
  EXECUTE_PROCESS(
    COMMAND ${GIT_EXECUTABLE} describe --tags --dirty
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE_VERSION
    RESULT_VARIABLE GIT_DESCRIBE_RESULT
    ERROR_VARIABLE GIT_DESCRIBE_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  if (GIT_DESCRIBE_RESULT)
    set(PRODUCT_VERSION "unknown-not-git-tree")
    message(STATUS "Not a git repo, not embedding version information")
  else ()
    set(PRODUCT_VERSION ${GIT_DESCRIBE_VERSION})
    message(STATUS "Version: ${PRODUCT_VERSION}")
  endif()
else(Git_FOUND)
  set(PRODUCT_VERSION "unknown-no-git-support")
  message(STATUS "Git not found. Install git to embed version information to the binary")
endif()

# Product filename
set(PRODUCT_NAME juksautin)

# LTO check
include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT error)
if( lto_supported )
    message(STATUS "IPO / LTO enabled")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif()

## AVR Chip Configuration
# 8Mhz, this should match the crystal on your board,
# I use 8Mhz and 3.3V for the lowest power consumption
set(F_CPU 16000000)
# CPU, you can find the list here:
# https://gcc.gnu.org/onlinedocs/gcc/AVR-Options.html
set(MCU atmega328p)
# Clock configuration. These values multiplied must equal to F_CPU to
# make the clock tick once per second.
set(CLOCK_A 250 CACHE STRING "Clock counter TOP value in OCR2A")
set(CLOCK_B 250 CACHE STRING "Clock software divider")
set(CLOCK_PRESCALER 256 CACHE STRING "Clock prescaler for TIMER2")

# Default Baudrate for UART, read avr include/util/setbaud.h for usage
set(BAUD 9600 CACHE STRING "Serial port baud rate")

message(STATUS "${MCU} running at ${F_CPU} Hz")

# Sanity check to make sure the clock ticks once per second.
math(EXPR clock_sanitycheck "${CLOCK_A} * ${CLOCK_B} * ${CLOCK_PRESCALER} - ${F_CPU}" OUTPUT_FORMAT DECIMAL)
if(clock_sanitycheck)
  message(SEND_ERROR "CLOCK_A * CLOCK_B * CLOCK_PRESCALER must equal to ${F_CPU}")
endif()

# MODBUS_SILENCE is the 14 bit long duration on the serial line,
# measured in TIMER2 ticks. We consider a frame to be ready after 14
# bits and after another 14 bits we can start transmitting.
# https://en.wikipedia.org/wiki/Modbus#Modbus_RTU_frame_format_(primarily_used_on_asynchronous_serial_data_lines_like_RS-485/EIA-485)
math(EXPR MODBUS_SILENCE "14 * ${F_CPU} / ${BAUD} / ${CLOCK_PRESCALER}" OUTPUT_FORMAT DECIMAL)
message(STATUS "Modbus silence duration: ${MODBUS_SILENCE} ticks of TIMER2")
if(MODBUS_SILENCE GREATER_EQUAL CLOCK_A)
  message(SEND_ERROR "Modbus silence too long. Must be smaller than ${CLOCK_A}. Adjust baud rate or clock parameters.")
endif()

# The programmer to use, read avrdude manual for list
set(PROG_TYPE usbasp CACHE STRING "Programmer type in avrdude")

# AVR Fuses, must be in concordance with your hardware and F_CPU
# http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega328p
set(E_FUSE 0xfd)
set(H_FUSE 0xde)
set(L_FUSE 0xff)
set(LOCK_BIT 0xff)

# Use AVR GCC toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_ASM_COMPILER avr-gcc)

# Pass defines to compiler
add_definitions(
    -DF_CPU=${F_CPU}UL
    -DCLOCK_A=${CLOCK_A}
    -DCLOCK_B=${CLOCK_B}
    -DCLOCK_PRESCALER=${CLOCK_PRESCALER}
    -DBAUD=${BAUD}
    -DMODBUS_SILENCE=${MODBUS_SILENCE}
    -DPRODUCT_VERSION=${PRODUCT_VERSION}
)

# mmcu MUST be passed to both the compiler and linker, this handles the linker
set(CMAKE_EXE_LINKER_FLAGS -mmcu=${MCU})

add_compile_options(
    -mmcu=${MCU} # MCU
    -std=gnu11 # C11 standard
    -O2 # optimize
    -Wall # enable warnings
    -fshort-enums # Make enums as short as possible
)
file(GLOB SRC_FILES "src/*.c") # Load all files in src folder

# Create one target
add_executable(${PRODUCT_NAME} ${SRC_FILES})

# Rename the output to .elf as we will create multiple files
set_target_properties(${PRODUCT_NAME} PROPERTIES OUTPUT_NAME ${PRODUCT_NAME}.elf)

# Strip binary for upload
add_custom_target(strip ALL avr-strip ${PRODUCT_NAME}.elf DEPENDS ${PRODUCT_NAME})

# Transform binary into hex file, we ignore the eeprom segments in the step
add_custom_target(hex ALL avr-objcopy -R .eeprom -O ihex ${PRODUCT_NAME}.elf ${PRODUCT_NAME}.hex DEPENDS strip)

# Transform binary into hex file, this is the eeprom part (empty if you don't
# use eeprom static variables)
add_custom_target(eeprom avr-objcopy -j .eeprom  --set-section-flags=.eeprom="alloc,load"  --change-section-lma .eeprom=0 -O ihex ${PRODUCT_NAME}.elf ${PRODUCT_NAME}.eep DEPENDS strip)

# Upload the firmware with avrdude
add_custom_target(upload avrdude -c ${PROG_TYPE} -p ${MCU} -U flash:w:${PRODUCT_NAME}.hex DEPENDS hex)

# Upload the eeprom with avrdude
add_custom_target(upload_eeprom avrdude -c ${PROG_TYPE} -p ${MCU} -U eeprom:w:${PRODUCT_NAME}.eep DEPENDS eeprom)

# Burn fuses
add_custom_target(fuses avrdude -c ${PROG_TYPE} -p ${MCU} -B 3 -U lfuse:w:${L_FUSE}:m -U hfuse:w:${H_FUSE}:m -U efuse:w:${E_FUSE}:m -U lock:w:${LOCK_BIT}:m )

# Clean extra files
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${PRODUCT_NAME}.hex;${PRODUCT_NAME}.eeprom;${PRODUCT_NAME}.lst")

cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME stm32-liquid)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # App: application layer (bare-metal super loop, no FreeRTOS)
    ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/app_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/app_imu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/app_led.c
    ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/app_oled.c
    ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/app_irq.c
    ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/app_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/liquid.c

    # Drivers/BSP: board support (LED / OLED / MPU6050 / USART)
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP/Src/bsp_led.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP/Src/bsp_oled.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP/Src/bsp_usart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP/Src/bsp_mpu6050.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP/Src/bsp_mpu6050_dmp.c

    # Drivers/Platform: HAL resource wrappers
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/Platform/Src/i2c_hardware.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/Platform/Src/inv_mpu_port.c

    # Drivers/ThirdParty/InvenSense: DMP driver
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/ThirdParty/InvenSense/inv_mpu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/ThirdParty/InvenSense/inv_mpu_dmp_motion_driver.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/App/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/Platform/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/ThirdParty/InvenSense
)

# Add project symbols (macros)
# 注意：MPU6050 / MOTION_DRIVER_TARGET_STM32F1 由 inv_mpu.h 自行定义，这里不要重复定义。
# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

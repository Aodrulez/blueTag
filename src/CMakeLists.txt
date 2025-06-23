# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.12)

# Include build functions from Pico SDK
# include(${USERHOME}/research/blueTag/pico-sdk/external/pico_sdk_import.cmake)
 include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C   standards
project(blueTag C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# Tell CMake where to find the executable source file
add_executable(${PROJECT_NAME} 
    blueTag.c
    usbDescriptors.c
    modules/usb2serial/uartBridge.c
    modules/flashProgrammer/serProg.c
    modules/openocd/openocdHandler.c
    modules/openocd/openocdJTAG.c
    modules/openocd/openocdSWD.c
    modules/cmsis-dap/cmsisDAP.c 
    modules/cmsis-dap/CMSIS_5/CMSIS/DAP/Firmware/Source/DAP.c
    modules/cmsis-dap/CMSIS_5/CMSIS/DAP/Firmware/Source/JTAG_DP.c
    modules/cmsis-dap/CMSIS_5/CMSIS/DAP/Firmware/Source/DAP_vendor.c
    modules/cmsis-dap/CMSIS_5/CMSIS/DAP/Firmware/Source/SWO.c
    modules/cmsis-dap/CMSIS_5/CMSIS/DAP/Firmware/Source/SW_DP.c
    modules/cmsis-dap/bsp/rp2040/cdc_uart.c
)

include_directories( ./
    modules/usb2serial
    modules/flashProgrammer
    modules/openocd
    modules/cmsis-dap/
    modules/cmsis-dap/CMSIS_5/CMSIS/DAP/Firmware/Include/
    modules/cmsis-dap/CMSIS_5/CMSIS/Core/Include/
    modules/cmsis-dap/bsp/rp2040/
)



target_link_libraries(${PROJECT_NAME} 
    pico_stdlib
    hardware_flash
    hardware_spi
	pico_multicore
    pico_rand
    pico_stdio_usb
	tinyusb_device
    tinyusb_board
)


# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})





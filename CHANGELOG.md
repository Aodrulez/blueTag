# Changelog

## 2.1.2 (2025-07-03)

Fixes:

 - Fixed regression in USB-to-Serial hardware boot mode caused by the fix for [Issue #13](https://github.com/Aodrulez/blueTag/issues/13) 
 
Features:
 - None

Others:
 - None

## 2.1.1 (2025-07-03)

Fixes:

 - Fixed [Issue #13](https://github.com/Aodrulez/blueTag/issues/13) where GPIO0 & GPIO1 weren't working as expected in JTAGulator scan mode for SWD
 - Fixed channel & progress-bar count mismatch for both JTAG & SWD scans
 
Features:
 - None

Others:
 - Updated blueTag's USB vendor & product IDs to official values from [pid.codes](https://pid.codes/1209/A0D1/)
 - Initial changes to implement alignment with Buspirate codebase
   
## 2.1.0 (2025-02-21)

Fixes:

 - Fixed the issue of some dev-boards booting straight into CMSIS-DAP hardware mode
 - Fixed USB-to-Serial mode conflict with CMSIS-DAP mode's UART interface
 
Features:
 - None

Others:
 - Removed Flashrom's programmer mode from boot mode
   
## 2.0.0 (2025-02-16)

Fixes:

 - Debug interface scan logic for both JTAG & SWD is now improved as well as safer than before
 - Fixed a bug where SWD scan mode logic didn't switch back the target device to JTAG mode when successful
 - Fixed a bug related to TRST pin identification logic for the JTAG scan mode
 
Features:
 - Added support for four new hardware modes (USB-to-Serial, Flashrom's serial programmer, Openocd JTAG/SWD debugger, CMSIS-DAP v1.0 debugger)
 - Added the option for blueTag to boot straight into above mentioned hardware modes

Others:
 - Updated description of the project with relevant details for this major release

## 1.0.2 (2024-11-11)

Fixes:

 - None
 
Features:
 - Updated default available channels from 9 to 16 

Others:
 - Updated description of the project with an image describing the pinout information

## 1.0.1 (2024-07-31)

Fixes:

 - None
 
Features:
 - Updated "jep106.inc" to include recent information (July 2024)

Others:
 - Implemented github actions for online compilation
 - Added workflow to automatically generate uf2 binary on each release

## 1.0.0 (2021-09-29)

Fixes:

 - None
 
Features:

 - Initial release

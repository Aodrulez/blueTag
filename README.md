# [ blueTag ] 
    
JTAGulator alternative for RP2040 microcontroller based development boards including RPi Pico. Huge shout-out to Joe Grand for his JTAGulator project!




![](images/BlueTag.png?raw=true "blueTag detecting SWD pinout on STM32 Blue Pill & a Raspberry Pi Pico")


## Pinout
![](images/BlueTagPinout.png?raw=true "blueTag Pinout")


## Installation
- Download latest release version of blueTag ("blueTag-vX.X.X.uf2") from this github repository's release section
- Press & hold 'BOOTSEL' button on a RP2040 microcontroller based development board, connect it to a computer via USB cable & then release the button
- Copy "blueTag-vX.X.X.uf2" file onto the newly detected flash drive (RPI-RP2*) on your computer


## Usage
- Connect the RP2040 microcontroller based development board running blueTag to your computer using USB cable
- Connect the development board's GPIO pins (GPIO0-GPIO15 so 16 channels in all) to your target's test-points
- Connect the development board's "GND" pin to target's "GND"
- Open a terminal emulator program of your choice that supports "Serial" communication (Ex. Teraterm, Putty, Minicom)
- Select "Serial" communication & connect to the development board's newly assigned COM port
- blueTag supports auto-baudrate detection so you should not have to perform any additional settings
- Press any key in the terminal emulator program to start using blueTag

> **_NOTE 1:_** Most RP2040 microcontroller based development board's GPIO pins function at 3.3v. For connecting to devices running other voltage levels, use of external level shifter(s) will be required.

> **_NOTE 2:_** Since the algorithm verifies channels in order (from 0 to 15), connect the channels in sequence (from 0 to 15) to your target's testpads/test points for the quickest execution time.  

## References & special thanks

- https://github.com/grandideastudio/jtagulator
- https://research.kudelskisecurity.com/2019/05/16/swd-arms-alternative-to-jtag/
- https://github.com/jbentham/picoreg
- https://github.com/szymonh/SWDscan
- Arm Debug Interface Architecture Specification (debug_interface_v5_2_architecture_specification_IHI0031F.pdf)

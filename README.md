# [ blueTag ] 
    
JTAGulator alternative based on Raspberry Pi Pico. Huge shout-out to Joe Grand for his JTAGulator project!




![](images/swd.JPG?raw=true "blueTag detecting SWD pinout on STM32 Blue Pill & a Raspberry Pi Pico")

## Installation
- Download latest release version of blueTag ("blueTag-vX.uf2") from this github repository
- Press & hold 'BOOTSEL' button on a Raspberry Pi Pico, connect it to a computer via USB cable & then release the button
- Copy "blueTag-vX.uf2" file onto the newly detected flash drive (RPI-RP2) on your computer


## Usage
- Connect Raspberry Pi Pico running blueTag to your computer using USB cable
- Connect Raspberry Pi Pico's GPIO pins (GPIO0-GPIO8 so 9 channels in all) to your target's test-points
- Open a terminal emulator program of your choice that supports "Serial" communication (Ex. Teraterm, Putty, Minicom)
- Select "Serial" communication & connect to Pico's newly assigned COM port
- blueTag supports auto-baudrate detection so you should not have to perform any additional settings
- Press any key in the terminal emulator program to start using blueTag

Note: Pico's GPIO pins function at 3.3v. For connecting to devices running other voltage levels, use of external level shifter(s) will be required
## References & special thanks

- https://github.com/grandideastudio/jtagulator
- https://research.kudelskisecurity.com/2019/05/16/swd-arms-alternative-to-jtag/
- https://github.com/jbentham/picoreg
- https://github.com/szymonh/SWDscan
- Arm Debug Interface Architecture Specification (debug_interface_v5_2_architecture_specification_IHI0031F.pdf)

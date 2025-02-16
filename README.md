# [ blueTag ] 
    
JTAGulator alternative & a hardware hacker's multi-tool for RP2040 microcontroller based development boards including RPi Pico & RP2040-Zero. Huge shout-out to Joe Grand for his JTAGulator project!

![](images/bluetag-v2.0.png?raw=true "blueTag v2.0.0 Interface")

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
- UI provides detailed guidance for all commands & hardware modes

> **_NOTE 1:_** Most RP2040 microcontroller based development board's GPIO pins function at 3.3v. For connecting to devices running other voltage levels, use of external level shifter(s) will be required.

> **_NOTE 2:_** Since the algorithm verifies channels in order (from 0 to 15), connect the channels in sequence (from 0 to 15) to your target's testpads/test points for the quickest execution time.  

## Local build instructions
> **Recommended to use Docker**
- Open a command-prompt or terminal
- Change to the desired directory & then execute the following commands:
```sh
git clone https://github.com/Aodrulez/blueTag.git
cd blueTag
docker build -t pico-builder-image .
docker create --name pico-builder-container pico-builder-image
docker cp pico-builder-container:/project/src/build/blueTag.uf2 ./
```

## References & special thanks
>       JTAGulator features :
>        https://github.com/grandideastudio/jtagulator
>        https://research.kudelskisecurity.com/2019/05/16/swd-arms-alternative-to-jtag/
>        https://github.com/jbentham/picoreg
>        https://github.com/szymonh/SWDscan
>        Yusufss4 (https://gist.github.com/amullins83/24b5ef48657c08c4005a8fab837b7499?permalink_comment_id=4554839#gistcomment-4554839)
>        Arm Debug Interface Architecture Specification (debug_interface_v5_2_architecture_specification_IHI0031F.pdf)
        
>      Flashrom support :
>        https://www.flashrom.org/supported_hw/supported_prog/serprog/serprog-protocol.html
>        https://github.com/stacksmashing/pico-serprog

>      Openocd support  : 
>        http://dangerousprototypes.com/blog/2009/10/09/bus-pirate-raw-bitbang-mode/
>        http://dangerousprototypes.com/blog/2009/10/27/binary-raw-wire-mode/
>        https://github.com/grandideastudio/jtagulator/blob/master/PropOCD.spin
>        https://github.com/DangerousPrototypes/Bus_Pirate/blob/master/Firmware/binIO.c

>      USB-to-Serial support :
>        https://github.com/xxxajk/pico-uart-bridge
>        https://github.com/Noltari/pico-uart-bridge

>      CMSIS-DAP support :
>        https://github.com/majbthrd/DapperMime
>        https://github.com/raspberrypi/debugprobe

## License details
TinyUSB is licensed under the MIT license.
ARM's CMSIS_5 code is licensed under the Apache 2.0 license.
Source code files within this project are licensed under the MIT license unless otherwise stated.


// References:
// http://dangerousprototypes.com/docs/Bitbang

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "hardware/clocks.h"
#include "openocdHandler.h"


uint PIN_TCK     = 0;   // CS
uint PIN_TMS     = 1;   // CLK/SCL
uint PIN_TDI     = 2;   // MOSI
uint PIN_TDO     = 3;   // MISO
uint PIN_TRST    = 4;   // AUX
uint PIN_SWDCLK  = 2;
uint PIN_SWDIO   = 3;

uint OCD_MODE    = 0;   // Generic by default

void openocdModeSendVersion(void) 
{
    printf("BBIO1");    
}

void openocdModeInitPins(void)
{
    // Initialize pins
    if (OCD_MODE == OPENOCD_MODE_SWD)
    {
        gpio_init(PIN_SWDCLK);
        gpio_init(PIN_SWDIO);
    }
    else
    {   
        // SWD pins for Generic configuration are covered
        gpio_init(PIN_TCK);
        gpio_init(PIN_TMS);
        gpio_init(PIN_TDI);
        gpio_init(PIN_TDO);
    }
      
    // Mode setup
    if (OCD_MODE == OPENOCD_MODE_SWD)
    {   
        gpio_set_dir(PIN_SWDCLK, GPIO_OUT);
        gpio_set_dir(PIN_SWDIO, GPIO_IN);
    }
    else
    {
        gpio_set_dir(PIN_TDI, GPIO_OUT);
        gpio_set_dir(PIN_TCK, GPIO_OUT);
        gpio_set_dir(PIN_TMS, GPIO_OUT);
        gpio_set_dir(PIN_TDO, GPIO_IN);

        gpio_put(PIN_TCK, 0);
    } 

}


void openocdModeReset(void) 
{
    openocdModeInitPins();
}

unsigned char binBBpindirectionset(unsigned char inByte) {
    unsigned char i;
    i = 0;
    if (inByte & 0b10000)i = 1;
    gpio_set_dir(PIN_TRST, i);

    i = 0;
    if (inByte & 0b1000)i = 1;
    gpio_set_dir(PIN_TDI, i);

    i = 0;
    if (inByte & 0b100)i = 1;
    gpio_set_dir(PIN_TCK, i);

    i = 0;
    if (inByte & 0b10)i = 1;
    gpio_set_dir(PIN_TDO, i);

    i = 0;
    if (inByte & 0b1)i = 1;
    gpio_set_dir(PIN_TMS, i);

    //delay for a brief period
    sleep_ms(5);

    //return PORT read
    inByte &= (~0b00011111);
    if (gpio_get(PIN_TRST) != 0)inByte |= 0b10000;
    if (gpio_get(PIN_TDI) != 0)inByte |= 0b1000;
    if (gpio_get(PIN_TCK) != 0)inByte |= 0b100;
    if (gpio_get(PIN_TDO) != 0)inByte |= 0b10;
    if (gpio_get(PIN_TMS) != 0)inByte |= 0b1;

    return inByte; //return the read
}

unsigned char binBBpinset(unsigned char inByte) {
    unsigned char i;

    i = 0;
    if (inByte & 0b10000)i = 1;
    gpio_put(PIN_TRST, i);

    i = 0;
    if (inByte & 0b1000)i = 1;
    gpio_put(PIN_TDI, i);

    i = 0;
    if (inByte & 0b100)i = 1;
    gpio_put(PIN_TCK, i);

    i = 0;
    if (inByte & 0b10)i = 1;
    gpio_put(PIN_TDO, i);

    i = 0;
    if (inByte & 0b1)i = 1;
    gpio_put(PIN_TMS, i);

    //delay for a brief period
    sleep_ms(5);

    //return PORT read
    inByte &= (~0b00011111);
    if (gpio_get(PIN_TRST) != 0)inByte |= 0b10000;
    if (gpio_get(PIN_TDI) != 0)inByte |= 0b1000;
    if (gpio_get(PIN_TCK) != 0)inByte |= 0b100;
    if (gpio_get(PIN_TDO) != 0)inByte |= 0b10;
    if (gpio_get(PIN_TMS) != 0)inByte |= 0b1;
}

void openocdModeProcessCommands(void)
{
    static unsigned char command;
    unsigned int i;
    openocdModeReset();

    while (1) 
    {
        command = getc(stdin);

        if (command == 0x0a)
        {            
            while(command != '#' || command != 0) 
            {
                command == getc(stdin);
            }
            openocdModeReset();
            openocdModeSendVersion();
        }

        if ((command & 0b10000000) == 0) 
        {   
            switch (command) 
            {
                case CMD_RESET:          // reset, send BB version
                    openocdModeSendVersion();
                    break;
                case CMD_ENTER_SPI_MODE: //  Respond with unsupported version for clean exit
                    printf("SPI0");
                    break;
                case CMD_ENTER_I2C_MODE: //  Respond with unsupported version for clean exit
                    printf("I2C0");
                    break;
                case CMD_ENTER_UART_MODE: // Respond with unsupported version for clean exit
                    printf("ART0");
                    break;
                case CMD_ENTER_1WIRE_MODE: // Respond with unsupported version for clean exit
                    printf("1W00");
                    break;

                case CMD_ENTER_RAW_WIRE_MODE: // goto RAW WIRE mode
                    printf("RAW1");           //openocdModeSWD();
                    ocdModeSWD(PIN_SWDCLK, PIN_SWDIO);
                    break;

                case CMD_ENTER_JTAG_MODE: // goto OpenOCD mode
                    printf("OCD1");
                    openocdModeJTAG(PIN_TCK, PIN_TMS, PIN_TDI, PIN_TDO);
                    break;

                case CMD_RESET_BLUETAG: // return to terminal
                    putchar(1);
                    openocdModeSendVersion();
                    break;

                default:
                    if ((command >> 5) & 0b010) 
                    { 
                        putchar(binBBpindirectionset(command));
                    } else 
                    { // unknown command, error
                        putchar(0);
                    }
                    break;
            }
        } 
        else 
        { 
            putchar(binBBpinset(command)); 
        }
     
    }
}

void initOpenocdMode(uint ocdJTCK,uint ocdJTMS,uint ocdJTDI,uint ocdJTDO,uint ocdSwdClk,uint ocdSwdIO, uint ocdMode)
{
    // Setup Mode
    OCD_MODE = ocdMode;
    // setup Openocd pins
    if (ocdJTCK != OPENOCD_PIN_DEFAULT)
        PIN_TCK = ocdJTCK;
    if (ocdJTMS != OPENOCD_PIN_DEFAULT)
        PIN_TMS = ocdJTMS;   
    if (ocdJTDI != OPENOCD_PIN_DEFAULT)
        PIN_TDI = ocdJTDI;
    if (ocdJTDO != OPENOCD_PIN_DEFAULT)
        PIN_TDO = ocdJTDO;
    if (ocdSwdClk != OPENOCD_PIN_DEFAULT)
        PIN_SWDCLK = ocdSwdClk;
    if (ocdSwdIO != OPENOCD_PIN_DEFAULT)
        PIN_SWDIO = ocdSwdIO;    
    
    stdio_set_translate_crlf(&stdio_usb, false);
    openocdModeInitPins();
    openocdModeProcessCommands();
}
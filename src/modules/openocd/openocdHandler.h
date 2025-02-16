
#ifndef _OPENOCDHANDLER_H_
#define _OPENOCDHANDLER_H_

#define PIN_LED                 25
#define OPENOCD_PIN_DEFAULT     66
#define OPENOCD_MODE_GENERIC    0
#define OPENOCD_MODE_JTAG       1
#define OPENOCD_MODE_SWD        2

/*
Buspirate BinIO mode
 
2.1 00000000 - Reset, responds "BBIO1"
2.2 00000001 - Enter binary SPI mode, responds "SPI1"
2.3 00000010 - Enter binary I2C mode, responds "I2C1"
2.4 00000011 - Enter binary UART mode, responds "ART1"
2.5 00000100 - Enter binary 1-Wire mode, responds "1W01"
2.6 00000101 - Enter binary raw-wire mode, responds "RAW1"
2.7 00000110 - Enter OpenOCD JTAG mode
2.8 0000xxxx - Reserved for future raw protocol modes
2.9 00001111 - Reset Bus Pirate
2.10 0001000x - Bus Pirate self-tests
2.11 00010010 - Setup pulse-width modulation (requires 5 byte setup)
2.12 00010011 - Clear/disable PWM
2.13 00010100 - Take voltage probe measurement (returns 2 bytes)
2.14 00010101 - Continuous voltage probe measurement
2.15 00010110 - Frequency measurement on AUX pin
2.16 010xxxxx - Configure pins as input(1) or output(0): AUX|MOSI|CLK|MISO|CS
2.17 1xxxxxxx - Set on (1) or off (0): POWER|PULLUP|AUX|MOSI|CLK|MISO|CS 
*/

#define CMD_RESET               0b00000000
#define CMD_ENTER_SPI_MODE      0b00000001
#define CMD_ENTER_I2C_MODE      0b00000010 
#define CMD_ENTER_UART_MODE     0b00000011
#define CMD_ENTER_1WIRE_MODE    0b00000100
#define CMD_ENTER_RAW_WIRE_MODE 0b00000101
#define CMD_ENTER_JTAG_MODE     0b00000110
#define CMD_RESET_BLUETAG       0b00001111

void initOpenocdMode(uint ocdJTCK,uint ocdJTMS,uint ocdJTDI,uint ocdJTDO,uint ocdSwdClk,uint ocdSwdIO,uint ocdMode);
void initOpenocdModeJTAG(void);
void openocdModeJTAG(uint ocdJTCK,uint ocdJTMS,uint ocdJTDI,uint ocdJTDO);
void ocdModeSWD(uint PIN_SWDCLK, uint PIN_SWDIO);

#endif
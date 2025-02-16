
#include "stdio.h"
#include "pico/stdlib.h"
#include "string.h"
#include "openocdHandler.h"

#define CMD_UNKNOWN       0x00
#define CMD_PORT_MODE     0x01
#define CMD_FEATURE       0x02
#define CMD_READ_ADCS     0x03
#define CMD_TAP_SHIFT     0x05
#define CMD_ENTER_OOCD    0x06 
#define CMD_UART_SPEED    0x07
#define CMD_JTAG_SPEED    0x08
#define SERIAL_NORMAL     0
#define SERIAL_FAST       1

uint ocdModeJTAGPinTMS;
uint ocdModeJTAGPinTCK;
uint ocdModeJTAGPinTDI;
uint ocdModeJTAGPinTDO;

uint8_t openocdModeJTAGtdoRead(void)
{
    bool volatile tdoStatus;
    gpio_put(ocdModeJTAGPinTCK, 1);
    __asm volatile ("nop":);
    tdoStatus=gpio_get(ocdModeJTAGPinTDO);
    gpio_put(ocdModeJTAGPinTCK, 0);
    return(tdoStatus);
}

void openocdModeJTAGtdiHigh(void)
{
    gpio_put(ocdModeJTAGPinTDI, 1);
}

void openocdModeJTAGtdiLow(void)
{
    gpio_put(ocdModeJTAGPinTDI, 0);
}

void openocdModeJTAGtmsHigh(void)
{
    gpio_put(ocdModeJTAGPinTMS, 1);
}

void openocdModeJTAGtmsLow(void)
{
    gpio_put(ocdModeJTAGPinTMS, 0);
}

static void openocdModeJTAGAnswer(unsigned char *buf, unsigned int len) {
	unsigned int i;
	for (i=0; i < len; i++ )
    {
		putchar(buf[i]);
	}
}

void tapShiftRain(uint8_t *in_buf, uint16_t end_cnt) {
    uint16_t buf_off = 0;
    uint8_t output_byte = 0;
    uint8_t bit_index = 0;

    for (uint16_t bit_cnt = 0; bit_cnt < end_cnt; bit_cnt++) 
    {
        buf_off = bit_cnt / 8;
        uint8_t bit_mask = 1 << (bit_cnt % 8);

        if (in_buf[buf_off * 2] & bit_mask) {
            openocdModeJTAGtdiHigh();
        } else {
            openocdModeJTAGtdiLow();
        }

        if (in_buf[buf_off * 2 + 1] & bit_mask) {
            openocdModeJTAGtmsHigh();
        } else {
            openocdModeJTAGtmsLow();
        }

        if (openocdModeJTAGtdoRead()) {
            output_byte |= bit_mask;
        } else {
            output_byte &= ~bit_mask;
        }

        bit_index++;

        // Output the completed byte
        if (bit_index == 8 || bit_cnt == end_cnt - 1) {
            putchar(output_byte);
            fflush(stdout);
            output_byte = 0;  // Reset for the next byte
            bit_index = 0;
        }
    }
}


void openocdModeJTAG(uint ocdJTCK,uint ocdJTMS,uint ocdJTDI,uint ocdJTDO)
{
    // Pin setup
    ocdModeJTAGPinTCK = ocdJTCK;
    ocdModeJTAGPinTMS = ocdJTMS;
    ocdModeJTAGPinTDI = ocdJTDI;
    ocdModeJTAGPinTDO = ocdJTDO;

    uint i; 
    uint numSequences;
    unsigned char inByte, inByte2, command, value;
    unsigned char buf[0x2010];
    unsigned char dataBuffer[0x2010];
    uint x = 0;

    while(1)
    {
        command = getc(stdin);
        switch(command)
        {
            case CMD_UNKNOWN:
                printf("BBIO1");
                return;

            case CMD_ENTER_OOCD:
                printf("OCD1");
                break;

            case CMD_READ_ADCS:     // Unsupported
                buf[0] = CMD_READ_ADCS;
                buf[1] = 8;
                for (x = 2; x < 10; x++)
                {
                    buf[x] = 0;
                }
                openocdModeJTAGAnswer(buf, 10);
                break;

            case CMD_PORT_MODE:     // Unsupported
                inByte=getc(stdin);
                break;

            case CMD_FEATURE:       // Unsupported
                inByte=getc(stdin);
                inByte2=getc(stdin);
                break;

            case CMD_JTAG_SPEED:    // Unsupported
                inByte=getc(stdin);
                inByte2=getc(stdin);
                break;

            case CMD_UART_SPEED:    // Unsupported for now
				inByte=getc(stdin);
				inByte=getc(stdin);
				inByte2=getc(stdin);
				buf[0] = CMD_UART_SPEED;
				buf[1] = SERIAL_NORMAL;
				openocdModeJTAGAnswer(buf, 2);
				break;

            case CMD_TAP_SHIFT:
                inByte=getc(stdin);
                inByte2=getc(stdin);
                numSequences = (inByte << 8) | inByte2; // number of bit sequences

                // this fixes possible buffer overflow
                if (numSequences > 0x2000) 
                    numSequences = 0x2000;

                i = (numSequences+7)/8; // number of bytes used
                for (x = 0; x < 2*i; x++)
                {
                    inByte = getc(stdin);
                    dataBuffer[x] = inByte;
                }

                buf[0] = CMD_TAP_SHIFT;
                buf[1] = inByte;
                buf[2] = inByte2;
                openocdModeJTAGAnswer(buf, 3);
                tapShiftRain(dataBuffer, numSequences);
                break;
 
            default:
                buf[0] = 0x00; // unknown command
                buf[1] = 0x00;
                openocdModeJTAGAnswer(buf, 1);
                break;
        }
        fflush(stdout);
    }
}
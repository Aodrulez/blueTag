#include "stdio.h"
#include "pico/stdlib.h"
#include "string.h"
#include "openocdHandler.h"


/* http://dangerousprototypes.com/docs/Raw-wire_(binary)

2.1 00000000 – Exit to bitbang mode, responds “BBIOx”
2.2 00000001 – Display mode version string, responds “RAWx”
2.3 0000001x - I2C-style start (0) / stop (1) bit
2.4 0000010x- CS low (0) / high (1)
2.5 00000110 - Read byte
2.6 00000111 - Read bit
2.7 00001000 - Peek at input pin
2.8 00001001 - Clock tick
2.9 0000101x - Clock low (0) / high (1)
2.10 0000110x - Data low (0) / high (1)
2.11 0001xxxx – Bulk transfer, send 1-16 bytes (0=1byte!)
2.12 0010xxxx - Bulk clock ticks, send 1-16 ticks
2.13 0011xxxx - Bulk bits, send 1-8 bits of the next byte (0=1bit!) (added in v4.5)
2.14 0100wxyz – Configure peripherals w=power, x=pullups, y=AUX, z=CS
2.15 010100xy - Pull up voltage select (BPV4 only)- x=5v y=3.3v
2.16 011000xx – Set speed, 3=~400kHz, 2=~100kHz, 1=~50kHz, 0=~5kHz
2.17 1000wxyz – Config, w=HiZ/3.3v, x=2/3wire, y=msb/lsb, z=not used
2.18 10100100 - PIC write. Send command + 2 bytes of data, read 1 byte (v5.1)
2.19 10100101 - PIC read. Send command, read 1 byte of data (v5.1)
 */

#define CMD_EXIT                    0b00000000
#define CMD_DISPLAY_VERSION         0b00000001
#define CMD_I2C_START		        0b00000010
#define CMD_I2C_STOP		        0b00000011
#define CMD_CS_LOW		            0b00000100
#define CMD_CS_HIGH		            0b00000101
#define CMD_READ_BYTE		        0b00000110
#define CMD_READ_BIT		        0b00000111
#define CMD_PEEK_INPUT		        0b00001000
#define CMD_CLK_TICK		        0b00001001
#define CMD_CLK_LOW		            0b00001010
#define CMD_CLK_HIGH		        0b00001011
#define CMD_DATA_LOW		        0b00001100
#define CMD_DATA_HIGH		        0b00001101
#define CMD_BULK_TRANSFER	        0b00010000
#define CMD_BULK_CLK		        0b00100000
#define CMD_BULK_BIT		        0b00110000
#define CMD_CONFIG_PERIPH	        0b01000000
#define CMD_SET_SPEED		        0b01100000
#define CMD_CONFIG		            0b10000000
#define DEV_BITORDER_MSB		    0
#define DEV_BITORDER_LSB		    1
#define MODE_2WIRE					0
#define MODE_3WIRE					1
#define BSP_OK						0

uint ocdModeSWDPinCLK = 2;
uint ocdModeSWDPinDIO = 3;
uint bitOrderConfig   = DEV_BITORDER_MSB;
uint currentMode      = MODE_2WIRE;
uint pinOutputType    = 0;


void ocdModeSWDCLKLow(void)
{
    gpio_put(ocdModeSWDPinCLK, 0);
}

void ocdModeSWDCLKHigh(void)
{
    gpio_put(ocdModeSWDPinCLK, 1);
}

void ocdModeSWDDIOLow(void)
{
    gpio_put(ocdModeSWDPinDIO, 0);
}

void ocdModeSWDDIOHigh(void)
{
    gpio_put(ocdModeSWDPinDIO, 1);
}

uint8_t ocdModeSWDPeekDIO(void)
{
	return(gpio_get(ocdModeSWDPinDIO));
}

uint8_t reverse_u8(uint8_t value)
{
	value = (value & 0xcc) >> 2 | (value & 0x33) << 2;
	value = (value & 0xaa) >> 1 | (value & 0x55) << 1;
	return (value >> 4 | value << 4);
}

void ocdModeSWDToggleCLK(void)
{

		if (pinOutputType == 0) 
		{
			ocdModeSWDCLKHigh();
			__asm volatile ("nop":);
			ocdModeSWDCLKLow();			
		} 
		else 
		{
			ocdModeSWDCLKLow();	
			__asm volatile ("nop":);			
			ocdModeSWDCLKHigh();			
		}
}

void ocdModeSWD2WireSendBit(uint8_t bit)
{
	gpio_set_dir(ocdModeSWDPinDIO, GPIO_OUT);
	if (bit) 
	{
		ocdModeSWDDIOHigh();
	} 
	else 
	{
		ocdModeSWDDIOLow();
	}
	ocdModeSWDToggleCLK();
}

uint8_t ocdModeSWD2WireReadBit(void)
{
	gpio_set_dir(ocdModeSWDPinDIO, GPIO_IN);
	return gpio_get(ocdModeSWDPinDIO);
}

uint8_t ocdModeSWD2WireReadBitClock(void)
{
	uint8_t bit;
	gpio_set_dir(ocdModeSWDPinDIO, GPIO_IN);

	if (pinOutputType == 0) {
		ocdModeSWDCLKHigh();
		__asm volatile ("nop":);
		bit = gpio_get(ocdModeSWDPinDIO);
		ocdModeSWDCLKLow();
	} 
	else 
	{
		ocdModeSWDCLKLow();
		__asm volatile ("nop":);
		bit = gpio_get(ocdModeSWDPinDIO);
		ocdModeSWDCLKHigh();
	}
	return bit;
}

uint8_t ocdModeSWD2WireReadU8(void)
{
	uint8_t value;
	uint8_t i;
	gpio_set_dir(ocdModeSWDPinDIO, GPIO_IN);

	value = 0;
	for(i=0; i<8; i++) 
	{
		value |= (ocdModeSWD2WireReadBitClock() << i);
	}
	if(bitOrderConfig == DEV_BITORDER_MSB) 
	{
		value = reverse_u8(value);
	}
	return value;
}

uint8_t ocdModeSWD2WireWriteU8(uint8_t tx_data)
{
	uint8_t i;
	gpio_set_dir(ocdModeSWDPinDIO, GPIO_OUT);
	if(bitOrderConfig == DEV_BITORDER_MSB) 
	{
		tx_data = reverse_u8(tx_data);
	}
	for (i=0; i<8; i++) 
	{
		ocdModeSWD2WireSendBit((tx_data>>i) & 1);
	}
	return BSP_OK;
}

static void ocdModeSWDAnswer(unsigned char *buf, unsigned int len) 
{
	uint i;
	for (i=0; i < len; i++ )
    {
		putchar(buf[i]);
	}
}

void ocdModeSWD(uint ocdSWDCLK,uint ocdSWDDIO)
{
    // Pin setup
    ocdModeSWDPinCLK = ocdSWDCLK;
    ocdModeSWDPinDIO = ocdSWDDIO;


    uint i; 
    uint numSequences;
	uint8_t data;
    unsigned char inByte, inByte2, command, value;
    unsigned char buf[0x2010];
    unsigned char dataBuffer[0x2010];
    uint x = 0;

    while(1)
    {
        command = getc(stdin);
        switch(command)
        {
            case CMD_EXIT:
                printf("BBIO1");
                return;

			case CMD_DISPLAY_VERSION:
                printf("RAW1");
                return;

			case CMD_READ_BYTE:
				buf[0] = ocdModeSWD2WireReadU8();
				ocdModeSWDAnswer(buf, 1);
				break;

			case CMD_READ_BIT:
				buf[0] = ocdModeSWD2WireReadBitClock();
				ocdModeSWDAnswer(buf, 1);
				break;

			case CMD_PEEK_INPUT:
				buf[0] = ocdModeSWDPeekDIO(); 
				ocdModeSWDAnswer(buf, 1);
				break;

			case CMD_CLK_TICK:
				ocdModeSWDToggleCLK();
				buf[0] = 0x01; 
				ocdModeSWDAnswer(buf, 1);
				break;

			case CMD_CLK_LOW:
				ocdModeSWDCLKLow();
				buf[0] = 0x01; 
				ocdModeSWDAnswer(buf, 1);
				break;

			case CMD_CLK_HIGH:
				ocdModeSWDCLKHigh();
				buf[0] = 0x01; 
				ocdModeSWDAnswer(buf, 1);
				break;

			case CMD_DATA_LOW:
				ocdModeSWDDIOLow();
				buf[0] = 0x01; 
				ocdModeSWDAnswer(buf, 1);
				break;

			case CMD_DATA_HIGH:
				ocdModeSWDDIOHigh();
				buf[0] = 0x01; 
				ocdModeSWDAnswer(buf, 1);
				break;

			default:
				if ((command & CMD_BULK_BIT) == CMD_BULK_BIT) 
                {
					data = (command & 0b1111) + 1;
					buf[0] = getc(stdin);
					if(bitOrderConfig == DEV_BITORDER_MSB) 
					{
						buf[0] = reverse_u8(buf[0]);
					}
					for (i=0; i<data; i++) 
					{
						ocdModeSWD2WireSendBit((buf[0]>>i) & 1);
					}

					buf[0] = 0x01; 
					ocdModeSWDAnswer(buf, 1);

				} 
				else if ((command & CMD_BULK_TRANSFER) == CMD_BULK_TRANSFER) 
				{
					data = (command & 0b1111) + 1;
					for (x = 0; x < data; x++)
					{
						dataBuffer[x] = getc(stdin);
					}

					buf[0] = 0x01; 
					ocdModeSWDAnswer(buf, 1);

					for(i=0; i<data; i++) 
					{
						buf[i] = ocdModeSWD2WireWriteU8(dataBuffer[i]);
					}
					ocdModeSWDAnswer(buf, data);
				} 
				else if ((command & CMD_BULK_CLK) == CMD_BULK_CLK) 
				{
					data = (command & 0b1111) + 1;
					for(i=0; i<data; i++) 
					{
						ocdModeSWDToggleCLK();
					}
					
					buf[0] = 0x01; 
					ocdModeSWDAnswer(buf, 1);
				} 
				else if ((command & CMD_CONFIG) == CMD_CONFIG) 
				{
					if(command & 0b100)
					{
						currentMode = MODE_3WIRE;
					} else 
					{
						currentMode = MODE_2WIRE;
					}

					if(command & 0b10)
					{
						bitOrderConfig = DEV_BITORDER_LSB;
					} else 
					{
						bitOrderConfig = DEV_BITORDER_MSB;
					}

					if(command & 0b1000)
					{

					} else 
					{

					}

					if(command & 0b1)
					{
						pinOutputType = 1;
					} else 
					{
						pinOutputType = 0;
					}

					if (pinOutputType == 0) 
					{
						ocdModeSWDCLKLow();
					} 
					else 
					{
						ocdModeSWDCLKHigh();
					}

					ocdModeSWDDIOLow();

					// Response
					buf[0] = 0x01; 
					ocdModeSWDAnswer(buf, 1);
				}
				else if ((command & CMD_SET_SPEED) == CMD_SET_SPEED) 
				{
					buf[0] = 0x01; 
					ocdModeSWDAnswer(buf, 1);
				} 
				else
				{
					buf[0] = 0x01; 
					ocdModeSWDAnswer(buf, 1);					
				}			
        }
        fflush(stdout);
    }

}
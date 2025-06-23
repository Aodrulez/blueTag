#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "hardware/clocks.h"
#include "serProg.h"

#define PIN_CS 1
#define PIN_SCK 2
#define PIN_MOSI 3
#define PIN_MISO 4
#define PIN_LED 25
#define SPI_MODE 0x8 
#define S_CMD_MAP ( \
  (1 << S_CMD_NOP)       | \
  (1 << S_CMD_Q_IFACE)   | \
  (1 << S_CMD_Q_CMDMAP)  | \
  (1 << S_CMD_Q_PGMNAME) | \
  (1 << S_CMD_Q_SERBUF)  | \
  (1 << S_CMD_Q_BUSTYPE) | \
  (1 << S_CMD_SYNCNOP)   | \
  (1 << S_CMD_O_SPIOP)   | \
  (1 << S_CMD_S_BUSTYPE) | \
  (1 << S_CMD_S_SPI_FREQ)  \
)

unsigned char writeBuffer[4096];

static inline void cs_select(uint cs_pin) 
{
    asm volatile("nop \n nop \n nop"); 
    gpio_put(cs_pin, 0);
    asm volatile("nop \n nop \n nop"); 
}

static inline void cs_deselect(uint cs_pin) 
{
    asm volatile("nop \n nop \n nop"); 
    gpio_put(cs_pin, 1);
    asm volatile("nop \n nop \n nop"); 
}

uint32_t get24bits(void) 
{
    uint32_t result = 0;
    for (int i = 0; i < 3; i++) {
        result |= ((uint32_t)getc(stdin)) << (i * 8);
    }
    return result;
}

uint32_t get32bits(void) 
{
    uint32_t result = 0;
    for (int i = 0; i < 4; i++) {
        result |= ((uint32_t)getc(stdin)) << (i * 8);
    }
    return result;
}

void put32bits(uint32_t d) 
{
    for (int i = 0; i < 4; i++) {
        putchar((d >> (i * 8)) & 0xFF);
    }
}

void processCommands(char command)
{
     switch(command) 
     {
        case S_CMD_NOP:
            putchar(S_ACK);
            break;

        case S_CMD_Q_IFACE:
            putchar(S_ACK);
            putchar(0x01);
            putchar(0x00);
            break;

        case S_CMD_Q_CMDMAP:
            putchar(S_ACK);
            put32bits(S_CMD_MAP);

            for(int i = 0; i < 32 - sizeof(uint32_t); i++) {
                putchar(0);
            }
            break;

        case S_CMD_Q_PGMNAME:
            putchar(S_ACK);
            fwrite("blueTag\x0\x0\x0\x0\x0\x0\x0\x0\x0", 1, 16, stdout);
            fflush(stdout);
            break;

        case S_CMD_Q_SERBUF:
            putchar(S_ACK);
            putchar(0xFF);
            putchar(0xFF);
            break;

        case S_CMD_Q_BUSTYPE:
            putchar(S_ACK);
            putchar(SPI_MODE);
            break;

        case S_CMD_SYNCNOP:
            putchar(S_NAK);
            putchar(S_ACK);
            break;

        case S_CMD_S_BUSTYPE:
            {
                int bustype = getc(stdin);
                putchar(((bustype | SPI_MODE) == SPI_MODE) ? S_ACK : S_NAK);
            }
            break;

        case S_CMD_O_SPIOP:
            {
                uint32_t writelen = get24bits();
                uint32_t readlen = get24bits();

                cs_select(PIN_CS);
                fread(writeBuffer, 1, writelen, stdin);
                spi_write_blocking(spi_default, writeBuffer, writelen);

                putchar(S_ACK);
                uint32_t chunk;
                char buf[128];

                for(uint32_t i = 0; i < readlen; i += chunk) 
                {
                    chunk = MIN(readlen - i, sizeof(buf));
                    spi_read_blocking(spi_default, 0, buf, chunk);
                    fwrite(buf, 1, chunk, stdout);
                    fflush(stdout);
                }

                cs_deselect(PIN_CS);
            }
            break;

        case S_CMD_S_SPI_FREQ:
            {
                uint32_t reqFreq = get32bits();
                if (reqFreq >= 1) {
                    putchar(S_ACK);
                    put32bits(spi_set_baudrate(spi_default, reqFreq));
                } else {
                    putchar(S_NAK);
                }
            }
            break;

        default:
            putchar(S_NAK);
     }
}

int initSerProg(void) 
{
    stdio_set_translate_crlf(&stdio_usb, false);

    // Initialize CS
    gpio_init(PIN_CS);
    gpio_put(PIN_CS, 1);
    gpio_set_dir(PIN_CS, GPIO_OUT);

    // 1 MHz
    spi_init(spi_default, 1000 * 1000);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    while(1) 
    {
        int command = getc(stdin);
        processCommands(command);
    }

    return 0;
}
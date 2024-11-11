/* 
    [ blueTag - JTAGulator alternative based on Raspberry Pi Pico ]

        Inspired by JTAGulator. 

    [References & special thanks]
        https://github.com/grandideastudio/jtagulator
        https://research.kudelskisecurity.com/2019/05/16/swd-arms-alternative-to-jtag/
        https://github.com/jbentham/picoreg
        https://github.com/szymonh/SWDscan
        Arm Debug Interface Architecture Specification (debug_interface_v5_2_architecture_specification_IHI0031F.pdf)  
*/

#include <stdio.h>
#include "pico/stdlib.h"

const char *banner=R"banner(
            _______ ___     __   __ _______ _______ _______ _______ 
           |  _    |   |   |  | |  |       |       |   _   |       |
           | |_|   |   |   |  | |  |    ___|_     _|  |_|  |    ___|
           |       |   |   |  |_|  |   |___  |   | |       |   | __ 
           |  _   ||   |___|       |    ___| |   | |       |   ||  |
           | |_|   |       |       |   |___  |   | |   _   |   |_| |
           |_______|_______|_______|_______| |___| |__| |__|_______|)banner";


char *version="1.0.2";
#define  MAX_DEVICES_LEN    32                             // Maximum number of devices allowed in a single JTAG chain
#define  MIN_IR_LEN          2                             // Minimum length of instruction register per IEEE Std. 1149.1
#define  MAX_IR_LEN         32                             // Maximum length of instruction register
#define  MAX_IR_CHAIN_LEN   MAX_DEVICES_LEN * MAX_IR_LEN   // Maximum total length of JTAG chain w/ IR selected
#define  MAX_DR_LEN         4096                           // Maximum length of data register
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*array))


const uint onboardLED = 25;
const uint unusedGPIO = 28;                               // Pins on Pico are accessed using GPIO names
const uint MAX_NUM_JTAG  = 32;
const uint maxChannels = 16;                               // Max number of channels supported by Pico  

char cmd;

uint jTDI;           
uint jTDO;
uint jTCK;
uint jTMS;
uint jTRST;
uint jDeviceCount;
bool jPulsePins;
uint32_t deviceIDs[MAX_DEVICES_LEN];                         // Array to store identified device IDs

uint xTDI;           
uint xTDO;
uint xTCK;
uint xTMS;
uint xTRST;


// include file from openocd/src/helper
static const char * const jep106[][126] = {
#include "jep106.inc"
};



void splashScreen(void)
{
    printf("\n%s",banner);
    printf("\n");
    printf("\n          [ JTAGulator alternative for Raspberry Pi RP2040 Dev Boards ]");
    printf("\n          +-----------------------------------------------------------+");
    printf("\n          | @Aodrulez             https://github.com/Aodrulez/blueTag |");
    printf("\n          +-----------------------------------------------------------+\n\n");   
}

void showPrompt(void)
{
    printf(" > ");
}

void showMenu(void)
{
    printf(" Supported commands:\n\n");
    printf("     \"h\" = Show this menu\n");
    printf("     \"v\" = Show current version\n");
    printf("     \"p\" = Toggle pulsing of pins (Default:ON)\n");
    printf("     \"j\" = Perform JTAG pinout scan\n");
    printf("     \"s\" = Perform SWD pinout scan\n\n");
    printf(" [ Note: Disable 'local echo' in your terminal emulator program ]\n\n");
}

int getChannels(void)
{
    char x;
    printf("     Enter number of channels hooked up (Min 4, Max %d): ", maxChannels);
    x = getc(stdin);
    printf("%c\n",x);
    x = x - 48;
    while(x < 4 || x > maxChannels)
    {
        printf("     Enter a valid value: ");
        x = getc(stdin);
        printf("%c\n",x);
        x = x - 48;
    }
    printf("     Number of channels set to: %d\n\n",x);
    return(x);
}

// Function that sets all used channels to output high
void setPinsHigh(int channelCount)
{
    for(int x = 0; x < channelCount; x++)
    {
        gpio_put(x, 1);
    }
}

// Function that sets all used channels to output high
void setPinsLoW(int channelCount)
{
    for(int x = 0; x < channelCount; x++)
    {
        gpio_put(x, 0);
    }
}

// Function that sets all used channels to output high
void resetPins(int channelCount)
{
    setPinsHigh(channelCount);
    sleep_ms(100);
    setPinsLoW(channelCount);
    sleep_ms(100);
    setPinsHigh(channelCount);
    sleep_ms(100);
}

void pulsePins(int channelCount)
{
    setPinsLoW(channelCount);
    sleep_ms(20);
    setPinsHigh(channelCount);
    sleep_ms(20);
}

void jtagConfig(uint tdiPin, uint tdoPin, uint tckPin, uint tmsPin)
{
    // Output
    gpio_set_dir(tdiPin, GPIO_OUT);
    gpio_set_dir(tckPin, GPIO_OUT);
    gpio_set_dir(tmsPin, GPIO_OUT);

    // Input
    gpio_set_dir(tdoPin, GPIO_IN);
}

// Generate one TCK pulse. Read TDO inside the pulse.
// Expects TCK to be low upon being called.
bool tdoRead(void)
{
    bool tdoStatus;
    gpio_put(jTCK, 1);
    tdoStatus=gpio_get(jTDO);
    gpio_put(jTCK, 0);
    return(tdoStatus);
}


// Generates on TCK Pulse
// Expects TCK to be low when called & ignores TDO
void tckPulse(void)
{
    bool tdoStatus;
    tdoStatus=tdoRead();
}


void tdiHigh(void)
{
    gpio_put(jTDI, 1);
}

void tdiLow(void)
{
    gpio_put(jTDI, 0);
}

void tmsHigh(void)
{
    gpio_put(jTMS, 1);
}

void tmsLow(void)
{
    gpio_put(jTMS, 0);
}



void restoreIdle(void)
{
    tmsHigh();
    for(int x=0; x<5; x++)
    {
        tckPulse();
    }
    tmsLow();
    tckPulse();                 // Got to Run-Test-Idle
}

void enterShiftDR(void)
{
    tmsHigh();
    tckPulse();                 // Go to Select DR
    tmsLow();
    tckPulse();                 // Go to Capture DR
    tmsLow();
    tckPulse();                 // Go to Shift DR
}

void enterShiftIR(void)
{
    tmsHigh();
    tckPulse();                 // Go to Select DR
    tmsHigh();
    tckPulse();                 // Go to Select IR
    tmsLow();
    tckPulse();                 // Go to Capture IR
    tmsLow();
    tckPulse();                 // Go to Shift IR
}


uint32_t bitReverse(uint32_t x)
{
    x = ((x >> 1) & 0x55555555u) | ((x & 0x55555555u) << 1);
    x = ((x >> 2) & 0x33333333u) | ((x & 0x33333333u) << 2);
    x = ((x >> 4) & 0x0f0f0f0fu) | ((x & 0x0f0f0f0fu) << 4);
    x = ((x >> 8) & 0x00ff00ffu) | ((x & 0x00ff00ffu) << 8);
    x = ((x >> 16) & 0xffffu) | ((x & 0xffffu) << 16);
    return x;
}





void getDeviceIDs(int number)
{
    restoreIdle();              // Reset TAP to Run-Test-Idle
    enterShiftDR();             // Go to Shift DR

    uint32_t tempValue;
    for(int x=0; x < number;x++)
    {  
        tempValue=0;
        for(int y=0; y<32; y++)
        {
            tempValue <<= 1;
            tempValue |= tdoRead();
        }
        tempValue = bitReverse(tempValue);
        deviceIDs[x]=tempValue;
    }

    restoreIdle();              // Reset TAP to Run-Test-Idle
}

void displayPinout(void)
{
    printf("     [  Pinout  ]  TDI=CH%d", xTDI);
    printf(" TDO=CH%d", xTDO);
    printf(" TCK=CH%d", xTCK);
    printf(" TMS=CH%d", xTMS);
    if(xTRST != 0)
    {
        printf(" TRST=CH%d \n\n", xTRST);
    }
    else
    {
        printf(" TRST=N/A \n\n");
    }
}

const char *jep106_table_manufacturer(unsigned int bank, unsigned int id)
{
	if (id < 1 || id > 126) {
		return "Unknown";
	}
	/* index is zero based */
	id--;
	if (bank >= ARRAY_SIZE(jep106) || jep106[bank][id] == 0)
		return "Unknown";
	return jep106[bank][id];
}

void displayDeviceDetails(void)
{
    for(int x=0; x < jDeviceCount; x++)
    {
        printf("     [ Device %d ]  0x%08X ", x, deviceIDs[x]);
        uint32_t idc = deviceIDs[x];
        long part = (idc & 0xffff000) >> 12;
        int bank=(idc & 0xf00) >> 8;
        int id=(idc & 0xfe) >> 1;
        int ver=(idc & 0xf0000000) >> 28;

        if (id > 1 && id <= 126 && bank <= 8) 
        {
            printf("(mfg: '%s' , part: 0x%x, ver: 0x%x)\n",jep106_table_manufacturer(bank,id), part, ver);
        }
    }
    printf("\n");
}



// Function to detect number of devices in the scan chain 
int detectDevices(void)
{
    int x;
    restoreIdle();
    enterShiftIR();

    tdiHigh();
    for(x = 0; x <= MAX_IR_CHAIN_LEN; x++)
    {
        tckPulse();
    }

    tmsHigh();
    tckPulse();     //Go to Exit1 IR

    tmsHigh();
    tckPulse();     //Go to Update IR, new instruction in effect

    tmsHigh();
    tckPulse();     //Go to Select DR

    tmsLow();
    tckPulse();     //Go to Capture DR

    tmsLow();
    tckPulse();     //Go to Shift DR

    for(x = 0; x <= MAX_DEVICES_LEN; x++)
    {
        tckPulse();
    }

    // We are now in BYPASS mode with all DR set
    // Send in a 0 on TDI and count until we see it on TDO
    tdiLow();
    for(x = 0; x < MAX_DEVICES_LEN; x++)
    {
        if(tdoRead() == false)
        {
            break;                      // Our 0 has propagated through the entire chain
                                        // 'x' holds the number of devices
        }
    }

    if (x > MAX_DEVICES_LEN - 1)
    {
        x = 0;
    }

    tmsHigh();
    tckPulse();
    tmsHigh();
    tckPulse();
    tmsHigh();
    tckPulse();                         // Go to Run-Test-Idle
    return(x);
}

uint32_t shiftArray(uint32_t array, int numBits)
{
    uint32_t tempData;
    tempData=0;

    for(int x=1;x <= numBits; x++)
    {
        if( x == numBits)
            tmsHigh();

        if (array & 1)
            tdiHigh();
        else
            tdiLow();

        array >>= 1 ;
        tempData <<= 1;
        tempData |=tdoRead();
    }
    return(tempData);
}


uint32_t sendData(uint32_t pattern, int num)
{
    uint32_t tempData;
    
    enterShiftDR();
    tempData=shiftArray(pattern, num);
    tmsHigh();
    tckPulse();             // Go to Update DR, new data in effect

    tmsLow();
    tckPulse();             // Go to Run-Test-Idle
    
    return(tempData);
}

uint32_t bypassTest(int num, uint32_t bPattern)
{
    if(num <= 0 || num > MAX_DEVICES_LEN)   // Discard false-positives
    {
        return(0);
    }

    int x;
    uint32_t value;
    restoreIdle();
    enterShiftIR();

    tdiHigh();
    for(x=0; x < num*MAX_IR_LEN; x++)      // Send in 1s
    {
        tckPulse();
    }

    tmsHigh();
    tckPulse();               // Go to Exit1 IR

    tmsHigh();
    tckPulse();              // Go to Update IR, new instruction in effect

    tmsLow();
    tckPulse();              // Go to Run-Test-Idle       

    value=sendData(bPattern, 32 + num);
    value=bitReverse(value);
    return(value);

}

uint32_t uint32Rand(void)
{
  static uint32_t Z;
  if (Z & 1) {
    Z = (Z >> 1);
  } else {
    Z = (Z >> 1) ^ 0x7FFFF159;
  }
  return Z;
}


// Combination of both IDCode & Bypass scans
void jtagScan(void)
{
    int channelCount;
    uint32_t tempDeviceId;
    bool foundPinout=false;
    jDeviceCount=0;
    channelCount = getChannels();            // First get the number of channels hooked
    //resetPins(channelCount);
    jTDI = unusedGPIO;            // Assign TDI to an unused Pin on Pico so it doesn't interfere with the scan
    jTDO, jTCK, jTMS, jTRST = 0;
    
    for(int p=0; p<channelCount; p++)
    {
        jTDO = p;
        for(int q=0; q < channelCount; q++)
        {
            jTCK = q;
            if (jTCK == jTDO)
            {
                continue;
            }

            for(int r=0; r < channelCount; r++)
            {
                jTMS = r;
                if (jTMS == jTCK || jTMS == jTDO)
                {
                    continue;
                }
                setPinsHigh(channelCount);
                jtagConfig(jTDI, jTDO, jTCK, jTMS);

                if (jPulsePins)
                {
                    pulsePins(channelCount);
                }
                getDeviceIDs(1);
                tempDeviceId=deviceIDs[0];

                if( (deviceIDs[0] != -1) && (deviceIDs[0] & 1) )         //Ignore if received Device ID is 0xFFFFFFFF or if bit 0 != 1
                {
                    // We found something by idcode scan. For future debugging uncomment these
                    
                    tempDeviceId=deviceIDs[0];
                    
                    //foundPinout=true;
                    //displayPinout();
                    //printf("[ 0x%08x ]\n", deviceIDs[0]);

                    // IDCode scan is complete & we found a valid pinout
                    // Let's mount Bypass scan to be thorough
                    for(int s=0; s < channelCount; s++)
                    {
                        // onBoard LED notification
                        gpio_put(onboardLED, 1);


                        jTDI = s;
                        if (jTDI == jTMS || jTDI == jTCK || jTDI == jTDO)
                        {
                            continue;
                        }

                        setPinsHigh(channelCount);
                        jtagConfig(jTDI, jTDO, jTCK, jTMS);
                        if (jPulsePins)
                        {
                            pulsePins(channelCount);
                        }

                        jDeviceCount=detectDevices();
                        //printf(" detected device number: %d\n", jDeviceCount);

                        uint32_t dataIn;
                        uint32_t dataOut;
                        dataIn=uint32Rand();
                        dataOut=bypassTest(jDeviceCount, dataIn);
                        if(dataIn == dataOut)
                        {
                            foundPinout=true;
                            //displayPinout();
                            //printf("\t[ 0x%08x ]\n", deviceIDs[0]);

                            // Bypass scan found TDI too, now let's enumerate devices in the scan chain
                            setPinsHigh(channelCount);
                            jtagConfig(jTDI, jTDO, jTCK, jTMS);
                            if (jPulsePins)
                            {
                                pulsePins(channelCount);
                            }
                            // populate global array with details of all devices in the chain
                            getDeviceIDs(jDeviceCount);
                            

                            // Found all pins except nTRST, so let's try
                            xTDI=jTDI;
                            xTDO=jTDO;
                            xTCK=jTCK;
                            xTMS=jTMS;
                            xTRST=0;
                            for(int t=0; t < channelCount; t++)
                            {
                                jTRST=t;
                                if (jTRST == jTMS || jTRST == jTCK || jTRST == jTDO || jTRST == jTDI)
                                {
                                    continue;
                                }
                                setPinsHigh(channelCount);
                                jtagConfig(jTDI, jTDO, jTCK, jTMS);
                                if (jPulsePins)
                                {
                                    pulsePins(channelCount);
                                }
                                gpio_put(jTRST, 1);
                                gpio_put(jTRST, 0);
                                sleep_ms(100);          // Give device time to react

                                
                                getDeviceIDs(1);
                                if (tempDeviceId != deviceIDs[0] )
                                {
                                    //printf("   Possible nTRST: %x", jTRST);
                                    deviceIDs[0]=tempDeviceId;
                                    xTRST=jTRST;
                                }
                            }
                            // Done enumerating everything. 
                            displayPinout();
                            displayDeviceDetails();
                        }
                        // onBoard LED notification
                        gpio_put(onboardLED, 0);
                    }
                }
            }
        }
    }
    // IDcode scan should definitely identify valid devices. 
    // If none are detected, the channels are inaccurate
    if( foundPinout == false )
    {
        printf("     No JTAG devices found. Please try again.\n\n");
    }

}


// Initialize all available channels & set them as output
void initChannels(void)
{
    for(int x=0; x < maxChannels ; x++)
    {
        gpio_init(x);
        gpio_set_dir(x, GPIO_OUT);
    }
}

//-------------------------------------SWD Scan [custom implementation]-----------------------------
     

#define LINE_RESET_CLK_CYCLES 52        // Atleast 50 cycles, selecting 52 
#define LINE_RESET_CLK_IDLE_CYCLES 2    // For Line Reset, have to send both of these
#define SWD_DELAY 5
#define JTAG_TO_SWD_CMD 0xE79E
#define SWDP_ACTIVATION_CODE 0x1A
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))

uint xSwdClk=0;
uint xSwdIO=1;
bool swdDeviceFound=false;
int getSwdChannels(void)
{
    char x;
    printf("     Enter number of channels hooked up (Min 2, Max %d): ", maxChannels);
    x = getc(stdin);
    printf("%c\n",x);
    x = x - 48;
    while(x < 2 || x > maxChannels)
    {
        printf("     Enter a valid value: ");
        x = getc(stdin);
        printf("%c\n",x);
        x = x - 48;
    }
    printf("     Number of channels set to: %d\n",x);
    printf("     Note: SWD scan is slow, this might take some time. \n\n");
    return(x);
}

void swdDisplayDeviceDetails(uint32_t idcode)
{
        printf("     [ Device 0 ]  0x%08X ",  idcode);
        uint32_t idc = idcode;
        long part = (idc & 0xffff000) >> 12;
        int bank=(idc & 0xf00) >> 8;
        int id=(idc & 0xfe) >> 1;
        int ver=(idc & 0xf0000000) >> 28;

        if (id > 1 && id <= 126 && bank <= 8) 
        {
            printf("(mfg: '%s' , part: 0x%x, ver: 0x%x)\n",jep106_table_manufacturer(bank,id), part, ver);
        }
    printf("\n");
}


void swdDisplayPinout(int swdio, int swclk, uint32_t idcode)
{
    printf("     [  Pinout  ]  SWDIO=CH%d", swdio);
    printf(" SWCLK=CH%d\n\n", swclk);
    swdDisplayDeviceDetails(idcode);
}


void initSwdPins(void)
{
    gpio_set_dir(xSwdClk,GPIO_OUT);
    gpio_set_dir(xSwdIO,GPIO_OUT);
}

void swdClockPulse(void)
{
    gpio_put(xSwdClk, 0);
    sleep_ms(SWD_DELAY);
    gpio_put(xSwdClk, 1);
    sleep_ms(SWD_DELAY);
}


void swdSetReadMode(void)
{
    gpio_set_dir(xSwdIO,GPIO_IN);
}

void swdTurnAround(void)
{
    swdSetReadMode();
    swdClockPulse();
}

void swdSetWriteMode(void)
{
    gpio_set_dir(xSwdIO,GPIO_OUT);
}

void swdIOHigh(void)
{
    gpio_put(xSwdIO, 1);
}

void swdIOLow(void)
{
    gpio_put(xSwdIO, 0);
}

void swdWriteHigh(void)
{
    gpio_put(xSwdIO, 1);
    swdClockPulse();
}

void swdWriteLow(void)
{
    gpio_put(xSwdIO, 0);
    swdClockPulse();
}

bool swdReadBit(void)
{
    bool value=gpio_get(xSwdIO);
    swdClockPulse();
    return(value);
}

void swdReadDPIDR(void)
{
    long buffer;
    bool value;
    for(int x=0; x< 32; x++)
    {
        value=swdReadBit();
        bitWrite(buffer, x, value);
    }
    swdDisplayPinout(xSwdIO, xSwdClk, buffer);
}

// Receive ACK response from SWD device & verify if OK
bool swdReadAck(void)
{
    bool bit1=swdReadBit();
    bool bit2=swdReadBit();
    bool bit3=swdReadBit();
    if(bit1 == true && bit2 == false && bit3 == false)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void swdWriteBit(bool value)
{
    gpio_put(xSwdIO, value);
    swdClockPulse();
}

void swdWriteBits(long value, int length)
{
    for (int i=0; i<length; i++)
    {
        swdWriteBit(bitRead(value, i));
    }
}

void swdResetLineSWDJ(void)
{
    swdIOHigh();
    for(int x=0; x < LINE_RESET_CLK_CYCLES+10; x++)
    {
        swdClockPulse();
    }
}

void swdResetLineSWD(void)
{
    swdIOHigh();
    for(int x=0; x < LINE_RESET_CLK_CYCLES+10; x++)
    {
        swdClockPulse();
    }
    swdIOLow();
    swdClockPulse();
    swdClockPulse();
    swdClockPulse();
    swdClockPulse();
    swdIOHigh();
}

// Leave dormant state
void swdArmWakeUp(void)
{
    swdSetWriteMode();
    swdIOHigh();
    for(int x=0;x < 8; x++)     // Reset to selection Alert Sequence
    {
        swdClockPulse();
    }

    // Send selection alert sequence 0x19BC0EA2 E3DDAFE9 86852D95 6209F392 (128 bits)
    swdWriteBits(0x92, 8);
    swdWriteBits(0xf3, 8);
    swdWriteBits(0x09, 8);
    swdWriteBits(0x62, 8);

    swdWriteBits(0x95, 8);
    swdWriteBits(0x2D, 8);
    swdWriteBits(0x85, 8);
    swdWriteBits(0x86, 8);

    swdWriteBits(0xE9, 8);
    swdWriteBits(0xAF, 8);
    swdWriteBits(0xDD, 8);
    swdWriteBits(0xE3, 8);

    swdWriteBits(0xA2, 8);
    swdWriteBits(0x0E, 8);
    swdWriteBits(0xBC, 8);
    swdWriteBits(0x19, 8);

    swdWriteBits(0x00, 4);   // idle bits
    swdWriteBits(SWDP_ACTIVATION_CODE, 8);
}


void swdTrySWDJ(void)
{
    swdSetWriteMode();
    swdArmWakeUp();                     // Needed for devices like RPi Pico
    swdResetLineSWDJ();
    swdWriteBits(JTAG_TO_SWD_CMD, 16);
    swdResetLineSWDJ();
    swdWriteBits(0x00, 4);

    swdWriteBits(0xA5, 8);             // readIdCode command 0b10100101
    swdTurnAround();
    
    if(swdReadAck() == true)           // Got ACK OK
    {
        swdDeviceFound=true;
        swdReadDPIDR();
    }
    swdTurnAround();
    swdSetWriteMode();
    swdWriteBits(0x00, 8);
}

bool swdBruteForce(void)
{
    // onBoard LED notification
    gpio_put(onboardLED, 1);
    swdTrySWDJ();
    gpio_put(onboardLED, 0);
    if(swdDeviceFound)
    { return(true); } else { return(false); }
}

void swdScan(void)
{ 
    
    swdDeviceFound = false;
    bool result = false;    
    int channelCount = getSwdChannels();
    for(uint clkPin=0; clkPin < channelCount; clkPin++)
    {
        xSwdClk = clkPin;
        for(uint ioPin=0; ioPin < channelCount; ioPin++)
        {
            xSwdIO = ioPin;
            if( xSwdClk == xSwdIO)
            {
                continue;
            }
            initSwdPins();
            result = swdBruteForce();
            if (result) break;
        }
        if (result) break; 
    }
    if(swdDeviceFound == false)
    {
        printf("     No devices found. Please try again.\n\n");
    }
}



//--------------------------------------------Main--------------------------------------------------

int main()
{
    stdio_init_all();

    // GPIO init
    gpio_init(onboardLED);
    gpio_set_dir(onboardLED, GPIO_OUT);
    initChannels();
    jPulsePins=true;
    
    //get user input to display splash & menu    
    cmd=getc(stdin);
    splashScreen();
    showMenu();
    showPrompt();

    while(1)
    {
        cmd=getc(stdin);
        printf("%c\n\n",cmd);
        switch(cmd)
        {
            // Help menu requested
            case 'h':
                showMenu();
                break;
            case 'v':
                printf(" Current version: %s\n\n", version);
                break;

            case 'j':
                jtagScan();
                break;

            case 's':
                swdScan();
                break;

            case 'p':
                jPulsePins=!jPulsePins;
                if(jPulsePins)
                {
                    printf("     Pin pulsing activated.\n\n");
                }
                else
                {
                    printf("     Pin pulsing deactivated.\n\n");
                }                
                break;


            case 'x':
                for(int x=0;x<=25;x++)
                {
                    gpio_put(onboardLED, 1);
                    sleep_ms(250);
                    gpio_put(onboardLED, 0);
                    sleep_ms(250);
                }
                break;

            default:
                printf(" Unknown command. \n\n");
                break;
        }
        showPrompt();
    }    
    return 0;
}

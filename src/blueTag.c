/* 
    [ blueTag - JTAGulator alternative based on Raspberry Pi Pico ]

        Inspired by JTAGulator. 

    [References & special thanks]
        https://github.com/grandideastudio/jtagulator
        https://research.kudelskisecurity.com/2019/05/16/swd-arms-alternative-to-jtag/
        https://github.com/jbentham/picoreg
        https://github.com/szymonh/SWDscan
        Yusufss4 (https://gist.github.com/amullins83/24b5ef48657c08c4005a8fab837b7499?permalink_comment_id=4554839#gistcomment-4554839)
        Arm Debug Interface Architecture Specification (debug_interface_v5_2_architecture_specification_IHI0031F.pdf)  
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "pirate.h"
#include "blueTag.h"

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
#define CR		    13
#define LF		    10
//#define ONBOARD_LED 25 //if not defined, onboard LED will not be used

#ifdef ONBOARD_LED
const uint onboardLED = ONBOARD_LED;
#endif
const uint unusedGPIO = 28;                               // Pins on Pico are accessed using GPIO names
const uint MAX_NUM_JTAG  = 32;
const uint startChannel = 0;                                // First GPIO pin to use 0 - 16 by default
const uint maxChannels = 16;                               // Max number of channels supported by Pico  
uint progressCount = 0;
uint maxPermutations = 0;


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

long int strtol(const char *str, char **endptr, int base);

void onboardLEDset(bool state)
{
    #ifdef ONBOARD_LED
    gpio_put(onboardLED, state);
    #endif
}

void onboardLEDinit(void)
{
    #ifdef ONBOARD_LED
    gpio_init(onboardLED);
    gpio_set_dir(onboardLED, GPIO_OUT);
    #endif
}

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
    printf("     \"p\" = Toggle 'pin pulsing' setting (Default:ON)\n");
    printf("     \"j\" = Perform JTAG pinout scan\n");
    printf("     \"s\" = Perform SWD pinout scan\n\n");
    printf(" [ Note 1: Disable 'local echo' in your terminal emulator program ]\n");
    printf(" [ Note 2: Try deactivating 'pin pulsing' (p) if valid pinout isn't found ]\n\n");
}

void printProgress(size_t count, size_t max) {
    const int bar_width = 50;

    float progress = (float) count / max;
    int bar_length = progress * bar_width;

    printf("\r     Progress: [");
    for (int i = 0; i < bar_length; ++i) {
        printf("#");
    }
    for (int i = bar_length; i < bar_width; ++i) {
        printf(" ");
    }
    printf("] %.2f%%", progress * 100);

    fflush(stdout);
}

int stringToInt(char * str)
{
   char *endptr;
   long int num;
   int res = 0;
   num = strtol(str, &endptr, 10);
   if (endptr == str) 
   {
      return 0;
   } 
   else if (*endptr != '\0') 
   {
      return 0;
   } 
   else 
   {
      return((int)num);
   }
   return 0;
}

int getIntFromSerial(void)
{
    char strg[3] = {0, 0, 0};
    char chr;
    int lp = 0;
    int value = 0;
    chr = getc(stdin);
    printf("%c",chr);
    if(chr == CR || chr == LF || chr < 48 || chr > 57)
    {
        value = 0;
    }
    else if (chr > 49) 
    {
      strg[0] = chr;
      value = stringToInt(strg);
    }
    else 
    {
      strg[0] = chr;
      chr = getc(stdin);
      printf("%c",chr);
      if(chr == CR || chr == LF || chr < 48 || chr > 57)
      {
          strg[1] = 0;
      }
      else
      {
          strg[1] = chr;
      }
      value = stringToInt(strg);       
    }
    printf("\n");
    return(value);
}

int getChannels(void)
{
    int x;
    printf("     Enter number of channels hooked up (Min 4, Max %d): ", maxChannels);
    x = getIntFromSerial();
    while(x < 4 || x > maxChannels)
    {
        printf("     Enter a valid value: ");
        x = getIntFromSerial();       
    }
    printf("     Number of channels set to: %d\n\n",x);
    return(x);
}

#if 0
// Function that sets all used channels to output high
void setPinsHigh(int channelCount)
{
    for(int x = 0; x < channelCount; x++)
    {
        gpio_put(x, 1);
    }
}

// Function that sets all used channels to output low
void setPinsLoW(int channelCount)
{
    for(int x = 0; x < channelCount; x++)
    {
        gpio_put(x, 0);
    }
}
#endif

// Function that sets all used channels to output high
static inline void resetPins(uint startChannel, uint channelCount)
{
    setPinsHigh(startChannel, channelCount);
    sleep_ms(5);
    setPinsLoW(startChannel, channelCount);
    sleep_ms(5);
    setPinsHigh(startChannel, channelCount);
    sleep_ms(5);
}

void pulsePins(uint startChannel, uint channelCount)
{
    setPinsLoW(startChannel, channelCount);
    sleep_ms(2);
    setPinsHigh(startChannel, channelCount);
    sleep_ms(2);
}
#if 0
// Initialize all available channels & set them as output
void initChannels(void)
{
    for(int x=0; x < maxChannels ; x++)
    {
        gpio_init(x);
        gpio_set_dir(x, GPIO_OUT);
    }   
}

void jtagConfig(uint tdiPin, uint tdoPin, uint tckPin, uint tmsPin)
{
    // Output
    gpio_set_dir(tdiPin, GPIO_OUT);
    gpio_set_dir(tckPin, GPIO_OUT);
    gpio_set_dir(tmsPin, GPIO_OUT);

    // Input
    gpio_set_dir(tdoPin, GPIO_IN);
    gpio_put(tckPin, 0);
}

// Generate one TCK pulse. Read TDO inside the pulse.
// Expects TCK to be low upon being called.
bool tdoRead(void)
{
    bool volatile tdoStatus;
    gpio_put(jTCK, 1);
    tdoStatus=gpio_get(jTDO);
    gpio_put(jTCK, 0);
    return(tdoStatus);
}
#endif

// Generates on TCK Pulse
// Expects TCK to be low when called & ignores TDO
void tckPulse(void)
{
    bool tdoStatus;
    //tdoStatus=tdoRead();
    tdoStatus=tdoRead(jTCK, jTDO);
}
#if 0
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
#endif
void restoreIdle(void)
{
    //tmsHigh();
    tmsHigh(jTMS);
    for(int x=0; x < 5; x++)    // 5 is sufficient, adding few more to be sure
    {
        tckPulse();
    }
    //tmsLow();
    tmsLow(jTMS);
    tckPulse();                 // Got to Run-Test-Idle
}

void enterShiftDR(void)
{
    //tmsHigh();
    tmsHigh(jTMS);
    tckPulse();                 // Go to Select DR
    //tmsLow();
    tmsLow(jTMS);
    tckPulse();                 // Go to Capture DR
    //tmsLow();
    tmsLow(jTMS);
    tckPulse();                 // Go to Shift DR
}

void enterShiftIR(void)
{
    //tmsHigh();
    tmsHigh(jTMS);
    tckPulse();                 // Go to Select DR
    //tmsHigh();
    tmsHigh(jTMS);
    tckPulse();                 // Go to Select IR
    //tmsLow();
    tmsLow(jTMS);
    tckPulse();                 // Go to Capture IR
    //tmsLow();
    tmsLow(jTMS);
    tckPulse();                 // Go to Shift IR
}

uint32_t bitReverse(uint32_t n)
{
    uint32_t reversed = 0;
    for (int i = 0; i < 32; i++) {
        reversed <<= 1;           // Shift reversed bits to the left
        reversed |= (n & 1);      // Add the least significant bit of n to reversed
        n >>= 1;                  // Shift n to the right
    }
    return reversed;
}

void getDeviceIDs(int number)
{
    restoreIdle();              // Reset TAP to Run-Test-Idle
    enterShiftDR();             // Go to Shift DR

    //tdiHigh();
    tdiHigh(jTDI);
    //tmsLow();
    tmsLow(jTMS);
    uint32_t tempValue;
    for(int x=0; x < number;x++)
    {  
        tempValue=0;
        for(int y=0; y<32; y++)
        {
            tempValue <<= 1;
            //tempValue |= tdoRead();
            tempValue=tdoRead(jTCK, jTDO);
        }
        tempValue = bitReverse(tempValue);
        deviceIDs[x]=tempValue;
    }

    restoreIdle();              // Reset TAP to Run-Test-Idle
}

void displayPinout(void)
{
    printProgress(maxPermutations, maxPermutations);
    printf("\n\n");
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

bool isValidDeviceID(uint32_t idc)
{        
        long part = (idc & 0xffff000) >> 12;
        int bank=(idc & 0xf00) >> 8;
        int id=(idc & 0xfe) >> 1;
        int ver=(idc & 0xf0000000) >> 28;

        if (id > 1 && id <= 126 && bank <= 8) 
        {
            return(true);
        }

    return(false);
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
    int volatile x;
    restoreIdle();
    enterShiftIR();

    //tdiHigh();
    tdiHigh(jTDI);
    for(x = 0; x < MAX_IR_CHAIN_LEN; x++)
    {
        tckPulse();
    }

    //tmsHigh();
    tmsHigh(jTMS);
    tckPulse();     //Go to Exit1 IR

    //tmsHigh();
    tmsHigh(jTMS);
    tckPulse();     //Go to Update IR, new instruction in effect

    //tmsHigh();
    tmsHigh(jTMS);
    tckPulse();     //Go to Select DR

    //tmsLow();
    tmsLow(jTMS);
    tckPulse();     //Go to Capture DR

    //tmsLow();
    tmsLow(jTMS);
    tckPulse();     //Go to Shift DR

    for(x = 0; x < MAX_DEVICES_LEN; x++)
    {
        tckPulse();
    }

    // We are now in BYPASS mode with all DR set
    // Send in a 0 on TDI and count until we see it on TDO
    //tdiLow();
    tdiLow(jTDI);
    for(x = 0; x < (MAX_DEVICES_LEN - 1); x++)
    {
        //if(tdoRead() == false)
        if(tdoRead(jTCK, jTDO) == false)
        {
            break;                      // Our 0 has propagated through the entire chain
                                        // 'x' holds the number of devices
        }
    }

    if (x > (MAX_DEVICES_LEN - 1))
    {
        x = 0;
    }
    
    //tmsHigh();
    tmsHigh(jTMS);
    tckPulse();
    //tmsHigh();
    tmsHigh(jTMS);
    tckPulse();
    //tmsLow();
    tmsLow(jTMS);
    tckPulse();                         // Go to Run-Test-Idle
    return(x);
}

uint32_t shiftArray(uint32_t array, int numBits)
{
    uint32_t tempData;
    int x;
    tempData=0;

    for(x=1;x <= numBits; x++)
    {
        if( x == numBits)
        {
          //tmsHigh();
          tmsHigh(jTMS);
        }  

        if (array & 1)
            //{tdiHigh();}
            {tdiHigh(jTDI);}
        else
            //{tdiLow();}
            {tdiLow(jTDI);}

        array >>= 1 ;
        tempData <<= 1;
        //tempData |=tdoRead();
        tempData |=tdoRead(jTCK, jTDO);
    }
    return(tempData);
}

uint32_t sendData(uint32_t pattern, int num)
{
    uint32_t tempData;
    tempData=0;
    enterShiftDR();
    tempData=shiftArray(pattern, num);
    //tmsHigh();
    tmsHigh(jTMS);
    tckPulse();             // Go to Update DR, new data in effect

    //tmsLow();
    tmsLow(jTMS);
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

    //tdiHigh();
    tdiHigh(jTDI);
    for(x=0; x < (num * MAX_IR_LEN); x++)      // Send in 1s
    {
        tckPulse();
    }

    //tmsHigh();
    tmsHigh(jTMS);
    tckPulse();               // Go to Exit1 IR

    //tmsHigh();
    tmsHigh(jTMS);
    tckPulse();              // Go to Update IR, new instruction in effect

    //tmsLow();
    tmsLow(jTMS);
    tckPulse();              // Go to Run-Test-Idle       

    value=sendData(bPattern, 32 + num); // This is correct, verified.
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
  return (Z);
}

int calculateJtagPermutations(uint totalChannels) 
{
    int result = 1;
    for (int i = 0; i < 4; i++)             // Minimum required pins == 4
    {
        result *= (totalChannels - i);
    }
    return result;
}

bool jtagScan(uint channelCount)
{
    //int channelCount;
    uint32_t tempDeviceId;
    bool volatile foundPinout=false;
    jDeviceCount=0;
    //channelCount = getChannels();            // First get the number of channels hooked
    progressCount = 0;
    maxPermutations = calculateJtagPermutations(channelCount);
    jTDO, jTCK, jTMS, jTDI,jTRST = 0;
    resetPins(startChannel, channelCount);
    for(jTDI=startChannel; jTDI<channelCount; jTDI++)
    {
        for(jTDO=startChannel; jTDO < channelCount; jTDO++)
        {
            if (jTDI == jTDO)
            {
                continue;
            }
            for(jTCK =startChannel; jTCK  < channelCount; jTCK++)
            {
                if (jTCK  == jTDO || jTCK == jTDI)
                {
                    continue;
                }
                for(jTMS=startChannel; jTMS < channelCount; jTMS++)
                {                      
                        if (jTMS == jTCK || jTMS == jTDO || jTMS == jTDI)
                        {
                            continue;
                        }
                        // onBoard LED notification
                        //gpio_put(onboardLED, 1);
                        onboardLEDset(1);
                        
                        progressCount = progressCount+1;
                        printProgress(progressCount, maxPermutations);
                        setPinsHigh(startChannel, channelCount);                       
                        if (jPulsePins)
                        {
                            pulsePins(startChannel, channelCount);
                        }
                        jtagConfig(jTDI, jTDO, jTCK, jTMS);
                        jDeviceCount=detectDevices();
                        
                        uint32_t dataIn;
                        uint32_t dataOut;
                        dataIn=uint32Rand();
                        dataOut=bypassTest(jDeviceCount, dataIn);          
                        if(dataIn == dataOut)
                        {
                            jDeviceCount=detectDevices();
                            getDeviceIDs(jDeviceCount);
                            tempDeviceId=deviceIDs[0];
                            if (isValidDeviceID(tempDeviceId) == false || jDeviceCount <= 0 )
                            {
                              continue;
                            }
                            else
                            {
                              foundPinout=true;
                            }
                            
                            // Found all pins except nTRST, so let's try
                            xTDI=jTDI;
                            xTDO=jTDO;
                            xTCK=jTCK;
                            xTMS=jTMS;
                            xTRST=0;
                            for(jTRST=startChannel; jTRST < channelCount; jTRST++)
                            {
                                if (jTRST == jTMS || jTRST == jTCK || jTRST == jTDO || jTRST == jTDI)
                                {
                                    continue;
                                }
                                progressCount = progressCount+1;
                                printProgress(progressCount, maxPermutations);
                                
                                setPinsHigh(startChannel, channelCount);
                                if (jPulsePins)
                                {
                                    pulsePins(startChannel, channelCount);
                                }
                                jtagConfig(jTDI, jTDO, jTCK, jTMS);
                                //gpio_put(jTRST, 1);
                                trstHigh(jTRST);
                                //gpio_put(jTRST, 0);
                                trstLow(jTRST);
                                sleep_ms(10);          // Give device time to react

                                getDeviceIDs(1);
                                if (tempDeviceId != deviceIDs[0] )
                                {
                                    deviceIDs[0]=tempDeviceId;
                                    xTRST=jTRST;
                                }
                            }
                            // Done enumerating everything. 
                            if(foundPinout==true)
                            {
                              displayPinout();
                              displayDeviceDetails();
                              // onBoard LED notification
                              //gpio_put(onboardLED, 0);
                              onboardLEDset(0);
                              return true;
                            }                            
                        }
                        // onBoard LED notification
                        //gpio_put(onboardLED, 0);
                        onboardLEDset(0);
                    }
            }
        }
    }

    return foundPinout;
}


//-------------------------------------SWD Scan [custom implementation]-----------------------------
     

#define LINE_RESET_CLK_CYCLES 52        // Atleast 50 cycles, selecting 52 
#define LINE_RESET_CLK_IDLE_CYCLES 2    // For Line Reset, have to send both of these
#define SWD_DELAY 5
#define JTAG_TO_SWD_CMD 0xE79E
#define SWD_TO_JTAG_CMD 0xE73C
#define SWDP_ACTIVATION_CODE 0x1A
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))

int getSwdChannels(void)
{
    char x;
    printf("     Enter number of channels hooked up (Min 2, Max %d): ", maxChannels);
    x = getIntFromSerial();
    while(x < 2 || x > maxChannels)
    {
        printf("     Enter a valid value: ");
        x = getIntFromSerial();
    }
    printf("     Number of channels set to: %d\n\n",x);
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

void swdDisplayPinout(struct swdScan_t *swd, uint32_t idcode)
{
    printProgress(swd->maxPermutations, swd->maxPermutations);
    printf("\n\n");
    printf("     [  Pinout  ]  SWDIO=CH%d", swd->xSwdIO);
    printf(" SWCLK=CH%d\n\n", swd->xSwdClk);
    swdDisplayDeviceDetails(idcode);
}
#if 0
void initSwdPins(void)
{
    gpio_set_dir(xSwdClk,GPIO_OUT);
    gpio_set_dir(xSwdIO,GPIO_OUT);
}

void swdClockPulse(void)
{
    gpio_put(xSwdClk, 0);
    sleep_us(SWD_DELAY);
    gpio_put(xSwdClk, 1);
    sleep_us(SWD_DELAY);
}

void swdSetReadMode(void)
{
    gpio_set_dir(xSwdIO,GPIO_IN);
}
#endif
void swdTurnAround(struct swdScan_t *swd)
{
    //swdSetReadMode();
    swdSetReadMode(swd->xSwdIO);
    //swdClockPulse();
    swdClockPulse(swd->xSwdClk, SWD_DELAY);
}
#if 0
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
#endif 

void swdReadDPIDR(struct swdScan_t *swd)
{
    long buffer;
    bool value;
    for(int x=0; x< 32; x++)
    {
        //value=swdReadBit();
        value=swdReadBit(swd->xSwdClk, swd->xSwdIO, SWD_DELAY);
        bitWrite(buffer, x, value);
    }
    swdDisplayPinout(swd, buffer);
}

// Receive ACK response from SWD device & verify if OK
bool swdReadAck(struct swdScan_t *swd)
{
    //bool bit1=swdReadBit();
    bool bit1=swdReadBit(swd->xSwdClk, swd->xSwdIO, SWD_DELAY);
    //bool bit2=swdReadBit();
    bool bit2=swdReadBit(swd->xSwdClk, swd->xSwdIO, SWD_DELAY);
    //bool bit3=swdReadBit();
    bool bit3=swdReadBit(swd->xSwdClk, swd->xSwdIO, SWD_DELAY);
    if(bit1 == true && bit2 == false && bit3 == false)
    {
        return true;
    }
    else
    {
        return false;
    }
}

#if 0
void swdWriteBit(uint xSwdIO, bool value, uint swd_delay)
{
    gpio_put(xSwdIO, value);
    //swdClockPulse();
    swdClockPulse(xSwdClk, SWD_DELAY);
}
#endif

void swdWriteBits(struct swdScan_t *swd, long value, int length)
{
    for (int i=0; i<length; i++)
    {
        swdWriteBit(swd->xSwdIO, swd->xSwdClk, bitRead(value, i), SWD_DELAY);
    }
}

void swdResetLineSWDJ(struct swdScan_t *swd)
{
    //swdIOHigh();
    swdIOHigh(swd->xSwdIO);
    for(int x=0; x < LINE_RESET_CLK_CYCLES+10; x++)
    {
        //swdClockPulse();
        swdClockPulse(swd->xSwdClk, SWD_DELAY);
    }
}

void swdResetLineSWD(struct swdScan_t *swd)
{
    //swdIOHigh();
    swdIOHigh(swd->xSwdIO);
    for(int x=0; x < LINE_RESET_CLK_CYCLES+10; x++)
    {
        //swdClockPulse();
        swdClockPulse(swd->xSwdClk, SWD_DELAY);
    }
    //swdIOLow();
    swdIOLow(swd->xSwdIO);
    //swdClockPulse();
    swdClockPulse(swd->xSwdClk, SWD_DELAY);
    //swdClockPulse();
    swdClockPulse(swd->xSwdClk, SWD_DELAY);
    //swdClockPulse();
    swdClockPulse(swd->xSwdClk, SWD_DELAY);
    //swdClockPulse();
    swdClockPulse(swd->xSwdClk, SWD_DELAY);
    //swdIOHigh();
    swdIOHigh(swd->xSwdIO);    
}

// Leave dormant state
void swdArmWakeUp(struct swdScan_t *swd)
{
    //swdSetWriteMode();
    swdSetWriteMode(swd->xSwdIO);
    //swdIOHigh();
    swdIOHigh(swd->xSwdIO);
    for(int x=0;x < 8; x++)     // Reset to selection Alert Sequence
    {
        //swdClockPulse();
        swdClockPulse(swd->xSwdClk, SWD_DELAY);
    }

    // Send selection alert sequence 0x19BC0EA2 E3DDAFE9 86852D95 6209F392 (128 bits)
    swdWriteBits(swd, 0x92, 8);
    swdWriteBits(swd, 0xf3, 8);
    swdWriteBits(swd, 0x09, 8);
    swdWriteBits(swd, 0x62, 8);

    swdWriteBits(swd, 0x95, 8);
    swdWriteBits(swd, 0x2D, 8);
    swdWriteBits(swd, 0x85, 8);
    swdWriteBits(swd, 0x86, 8);

    swdWriteBits(swd, 0xE9, 8);
    swdWriteBits(swd, 0xAF, 8);
    swdWriteBits(swd, 0xDD, 8);
    swdWriteBits(swd, 0xE3, 8);

    swdWriteBits(swd, 0xA2, 8);
    swdWriteBits(swd, 0x0E, 8);
    swdWriteBits(swd, 0xBC, 8);
    swdWriteBits(swd, 0x19, 8);

    swdWriteBits(swd, 0x00, 4);   // idle bits
    swdWriteBits(swd, SWDP_ACTIVATION_CODE, 8);
}

void swdToJTAG(struct swdScan_t *swd)
{
  swdResetLineSWDJ(swd);
  swdWriteBits(swd, SWD_TO_JTAG_CMD, 16);
}

bool swdTrySWDJ(struct swdScan_t *swd)
{
    //swdSetWriteMode();
    swdSetWriteMode(swd->xSwdIO);
    swdArmWakeUp(swd);                     // Needed for devices like RPi Pico
    swdResetLineSWDJ(swd);
    swdWriteBits(swd, JTAG_TO_SWD_CMD, 16);
    swdResetLineSWDJ(swd);
    swdWriteBits(swd, 0x00, 4);

    swdWriteBits(swd, 0xA5, 8);             // readIdCode command 0b10100101
    swdTurnAround(swd);
    
    if(swdReadAck(swd) == true)           // Got ACK OK
    {
        swd->swdDeviceFound=true;
        swdReadDPIDR(swd);
    }
    swdTurnAround(swd);
    //swdSetWriteMode();
    swdSetWriteMode(swd->xSwdIO);
    swdWriteBits(swd, 0x00, 8);
    return(swd->swdDeviceFound);
}

bool swdBruteForce(struct swdScan_t *swd)
{
    // onBoard LED notification
    //gpio_put(onboardLED, 1);
    onboardLEDset(1);
    bool result = swdTrySWDJ(swd);
    //gpio_put(onboardLED, 0);
    onboardLEDset(0);
    if(result)
    { return(true); } else { return(false); }
}

bool swdScan(struct swdScan_t *swd)
{ 
    swd->swdDeviceFound = false;
    bool result = false;    
    //int channelCount = getSwdChannels();
    swd->progressCount = 0;
    swd->maxPermutations = swd->channelCount * (swd->channelCount - 1);
    for(uint clkPin=startChannel; clkPin < (swd->channelCount+startChannel); clkPin++)
    {
        swd->xSwdClk = clkPin;
        for(uint ioPin=startChannel; ioPin < (swd->channelCount+startChannel); ioPin++)
        {
            swd->xSwdIO = ioPin;
            if( swd->xSwdClk == swd->xSwdIO)
            {
                continue;
            }
            printProgress(swd->progressCount, swd->maxPermutations);
            swd->progressCount++;
            initSwdPins(swd->xSwdClk, swd->xSwdIO);
            result = swdBruteForce(swd);
            if (result) break;
        }
        if (result) break; 
    }

    // Switch back to JTAG
    swdToJTAG(swd);

    // return success or fail
    return result;
}

//--------------------------------------------Main--------------------------------------------------

static int main()
{
    char cmd;
    stdio_init_all();

    // GPIO init
    //gpio_init(onboardLED);
    //gpio_set_dir(onboardLED, GPIO_OUT);
    onboardLEDinit();
    initChannels(startChannel, maxChannels);
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
                if(!jtagScan(getChannels()))
                {
                    printProgress(maxPermutations, maxPermutations);
                    printf("\n\n");
                    printf("     No JTAG devices found. Please try again.\n\n");
                }
                break;

            case 's': 
                struct swdScan_t swd;
                swd.channelCount = getSwdChannels();              
                if(!swdScan(&swd))
                {
                    printProgress(maxPermutations, maxPermutations);
                    printf("\n\n");
                    printf("     No devices found. Please try again.\n\n");
                }                
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
                    //gpio_put(onboardLED, 1);
                    onboardLEDset(1);
                    sleep_ms(250);
                    //gpio_put(onboardLED, 0);
                    onboardLEDset(0);
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

///////////////// BUS PIRATE ADDITIONS //////////////////////
void bluetag_jPulsePins_set(bool jPulsePins)
{
    jPulsePins = jPulsePins;
}

void bluetag_progressbar_cleanup(struct swdScan_t *swd)
{
    printProgress(swd->maxPermutations, swd->maxPermutations);
}
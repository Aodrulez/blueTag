/* 
    [ blueTag - Hardware hacker's multi-tool based on RP2040 dev boards ]

        Inspired by JTAGulator. 

    [References & special thanks]
        https://github.com/grandideastudio/jtagulator
        https://research.kudelskisecurity.com/2019/05/16/swd-arms-alternative-to-jtag/
        https://github.com/jbentham/picoreg
        https://github.com/szymonh/SWDscan
        Yusufss4 (https://gist.github.com/amullins83/24b5ef48657c08c4005a8fab837b7499?permalink_comment_id=4554839#gistcomment-4554839)
        Arm Debug Interface Architecture Specification (debug_interface_v5_2_architecture_specification_IHI0031F.pdf)
        
        Flashrom support : 
            https://www.flashrom.org/supported_hw/supported_prog/serprog/serprog-protocol.html
            https://github.com/stacksmashing/pico-serprog

        Openocd support  : 
            http://dangerousprototypes.com/blog/2009/10/09/bus-pirate-raw-bitbang-mode/
            http://dangerousprototypes.com/blog/2009/10/27/binary-raw-wire-mode/
            https://github.com/grandideastudio/jtagulator/blob/master/PropOCD.spin
            https://github.com/DangerousPrototypes/Bus_Pirate/blob/master/Firmware/binIO.c

        USB-to-Serial support :
            https://github.com/xxxajk/pico-uart-bridge
            https://github.com/Noltari/pico-uart-bridge

        CMSIS-DAP support :
            https://github.com/majbthrd/DapperMime
            https://github.com/raspberrypi/debugprobe
*/

#include "stdio.h"
#include "pico/stdlib.h"
#include "pico/stdio/driver.h"
#include "pico/multicore.h"
#include "tusb.h"
#include "modules/usb2serial/uartBridge.h"
#include "modules/flashProgrammer/serProg.h"
#include "modules/openocd/openocdHandler.h"

const char *banner=R"banner(
             _______ ___     __   __ _______ _______ _______ _______ 
            |  _    |   |   |  | |  |       |       |   _   |       |
            | |_|   |   |   |  | |  |    ___|_     _|  |_|  |    ___|
            |       |   |   |  |_|  |   |___  |   | |       |   | __ 
            |  _   ||   |___|       |    ___| |   | |       |   ||  |
            | |_|   |       |       |   |___  |   | |   _   |   |_| |
            |_______|_______|_______|_______| |___| |__| |__|_______|)banner";


char *version="2.0.0";
#define MAX_DEVICES_LEN      32                             // Maximum number of devices allowed in a single JTAG chain
#define MIN_IR_LEN           2                              // Minimum length of instruction register per IEEE Std. 1149.1
#define MAX_IR_LEN           32                             // Maximum length of instruction register
#define MAX_IR_CHAIN_LEN     MAX_DEVICES_LEN * MAX_IR_LEN   // Maximum total length of JTAG chain w/ IR selected
#define MAX_DR_LEN           4096                           // Maximum length of data register
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*array))
#define USB_HOST_RECOGNISE_TIME   (1000 * 1000 * 5)
#define USB_MODE_DEFAULT     0
#define USB_MODE_CMSISDAP    1
#define CR		             13
#define LF		             10
#define EOL                  "\r\n"
#define OFF                  "\033[0m"
#define BOLD                 "\033[1m"


#define onboardLED    25
#define unusedGPIO    21                               // Pins on Pico are accessed using GPIO names
#define MAX_NUM_JTAG  32
#define maxChannels   16                               // Max number of channels supported by Pico  

// Hardware boot mode pins
#define hwBootUSB2SerialPin  26
#define hwBootFlashromPin    27
#define hwBootOpenocdPin     28
#define hwBootCmsisdapPin    29

bool isChannelSafe[maxChannels];
volatile int usbMode    = USB_MODE_DEFAULT;
uint progressCount      = 0;
uint maxPermutations    = 0;
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

// function declarations (Needs clean-up later)
void resetUART(void);

// include file from openocd/src/helper
static const char * const jep106[][126] = {
#include "jep106.inc"
};

long int strtol(const char *str, char **endptr, int base);

void splashScreen(void)
{
    printf("%s%s",EOL, banner);
    printf("%s", EOL);
    printf("%s          [  Hardware hacker's multi-tool based on RP2040 dev boards  ]", EOL);
    printf("%s          +-----------------------------------------------------------+", EOL);
    printf("%s          | @Aodrulez             https://github.com/Aodrulez/blueTag |", EOL);
    printf("%s          +-----------------------------------------------------------+%s%s", EOL, EOL, EOL);                                                                                                                                     
}

void showPrompt(void)
{
    printf(" > ");
}

void showMenu(void)
{
    printf(" Supported commands:%s%s", EOL, EOL);
    printf(BOLD);
    printf("  [ Informational ]%s%s", EOL, EOL);
    printf(OFF);
    printf("     \"h\" = Show this menu%s", EOL);
    printf("     \"v\" = Show current version%s%s", EOL, EOL);
    printf(BOLD);
    printf("  [ Debug interface enumeration (JTAGulator function) ]%s%s", EOL, EOL);
    printf(OFF);
    printf("     \"p\" = Toggle 'pin pulsing' setting (Default:OFF)%s", EOL);
    printf("     \"j\" = Perform JTAG pinout scan%s", EOL);
    printf("     \"s\" = Perform SWD pinout scan%s%s", EOL, EOL);
    printf(BOLD);
    printf("  [ Hardware modes ]%s%s", EOL, EOL);
    printf(OFF);
    printf("     \"U\" = Activate USB-to-Serial mode%s", EOL);
    printf("     \"F\" = Activate Flashrom's serial programmer mode%s", EOL); 
    printf("     \"O\" = Activate Openocd JTAG/SWD debugger mode%s", EOL); 
    printf("     \"C\" = Activate CMSIS-DAP v1.0 debugger mode%s%s", EOL, EOL); 

    printf(BOLD);
    printf("  [ Hardware boot modes ]%s%s", EOL, EOL);
    printf(OFF);
    printf("     [     Only ONE boot mode can be active at a time     ]%s", EOL); 
    printf("     +----------------------------------------------------+%s", EOL);
    printf("     | RP2040 pin config   | Mode                         |%s", EOL);
    printf("     +----------------------------------------------------+%s", EOL);             
    printf("     | GPIO 26 -> GND      | USB-to-Serial                |%s", EOL);
    printf("     | GPIO 27 -> GND      | Flashrom's serial programmer |%s", EOL);
    printf("     | GPIO 28 -> GND      | Openocd JTAG/SWD debugger    |%s", EOL);
    printf("     | GPIO 29 -> GND      | CMSIS-DAP v1.0 debugger      |%s", EOL);          
    printf("     +----------------------------------------------------+%s%s", EOL, EOL);

    printf(BOLD);
    printf(" Note 1:");
    printf(OFF);
    printf(" Disable 'local echo' in your terminal emulator program %s", EOL);
    printf(BOLD);
    printf(" Note 2:");
    printf(OFF);
    printf(" Try deactivating 'pin pulsing' (p) if valid JTAG/SWD pinout isn't found %s", EOL);
    printf(BOLD);
    printf(" Note 3:");
    printf(OFF);
    printf(" Unplug blueTag and reconnect (or reset) to disable 'Hardware modes' %s%s", EOL, EOL);    
    printf(BOLD);
    printf(" --  MAKE SURE GPIO-0 TO GPIO-15 ARE NOT CONNECTED TO 'GND'  -- %s", EOL); 
    printf(" -- CONNECT ONLY TO TARGETS COMPATIBLE WITH 3.3V LOGIC LEVEL -- %s%s", EOL, EOL);   
    printf(OFF);
 
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
    printf("%s", EOL);
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
    printf("     Number of channels set to: %d%s%s", x, EOL, EOL);
    return(x);
}

void pulsePins(int channelCount)
{
    for(int x=0; x < channelCount ; x++)
    {
        gpio_set_dir(x, GPIO_IN);
        gpio_pull_up(x);
        gpio_pull_down(x);
    }   
}

// Initialize all available channels & set them as output
void initChannels(void)
{
    for(int x=0; x < maxChannels ; x++)
    {
        gpio_init(x);
        gpio_set_dir(x, GPIO_IN);
    }   
}

void jtagInitChannels(int channelCount)
{
    for(int x=0; x < channelCount ; x++)
    {
        gpio_set_dir(x, GPIO_IN);
        if (jPulsePins)
        {
            gpio_pull_down(x);
        }
        else
        {
            gpio_pull_up(x);
        }
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

void jtagResetChannels(int channelCount)
{
    // Input to be safe
    for(int x=0; x < channelCount ; x++)
    {
        gpio_set_dir(x, GPIO_IN);
        if (xTRST > 0 && xTRST == x)    //Skip altering state of TRST if found
        {
            gpio_set_dir(x, GPIO_IN);
            if (jPulsePins)
            {
                gpio_pull_down(jTRST);
            }
            else
            {
                gpio_pull_up(jTRST);          
            }            
        }
    }   
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
    for(int x=0; x < 5; x++)    // 5 is sufficient, adding few more to be sure
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

uint32_t bitReverse(uint32_t n)
{
    uint32_t reversed = 0;
    for (int i = 0; i < 32; i++) 
    {
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

    tdiHigh();
    tmsLow();
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
    printProgress(maxPermutations, maxPermutations);
    printf("%s%s", EOL, EOL);
    printf("     [  Pinout  ]  TDI=CH%d", xTDI);
    printf(" TDO=CH%d", xTDO);
    printf(" TCK=CH%d", xTCK);
    printf(" TMS=CH%d", xTMS);
    if(xTRST != 0)
    {
        printf(" TRST=CH%d %s%s", xTRST, EOL, EOL );
    }
    else
    {
        printf(" TRST=N/A %s%s", EOL, EOL);
    }
}

const char *jep106_table_manufacturer(unsigned int bank, unsigned int id)
{
	if (id < 1 || id > 126) 
    {
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

    if (id > 1 && id <= 126 && bank <= 8 && idc > 0) 
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
            printf("(mfg: '%s' , part: 0x%x, ver: 0x%x)%s", jep106_table_manufacturer(bank,id), part, ver, EOL);
        }
    }
    printf("%s%s", EOL, EOL);
    printf(" Enable hardware OpenOCD mode for JTAG debugging? (y/n): ");
    cmd = getc(stdin);
    if ( cmd == 'y')
    {
        printf("y%s%s", EOL, EOL);
        printf(" [ Ex. Openocd command ]%s%s", EOL, EOL);
        printf("   'openocd -f interface/buspirate.cfg -c \"buspirate port XXXXXXXXXX; transport select jtag;\" -f target/esp32.cfg'%s", EOL);
        printf("    Replace 'XXXXXXXXXX' with COM port of blueTag [Ex. '/dev/ttyACM0' (Linux) or 'COM4' (Win)]%s%s", EOL, EOL);                
        resetUART();  
        initOpenocdMode(jTCK, jTMS, jTDI, jTDO, OPENOCD_PIN_DEFAULT, OPENOCD_PIN_DEFAULT, OPENOCD_MODE_JTAG);
    }
    printf("%s%s", EOL, EOL);
}

// Function to detect number of devices in the scan chain 
int detectDevices(void)
{
    int volatile x;
    restoreIdle();
    enterShiftIR();

    tdiHigh();
    for(x = 0; x < MAX_IR_CHAIN_LEN; x++)
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

    for(x = 0; x < MAX_DEVICES_LEN; x++)
    {
        tckPulse();
    }

    // We are now in BYPASS mode with all DR set
    // Send in a 0 on TDI and count until we see it on TDO
    tdiLow();
    for(x = 0; x < (MAX_DEVICES_LEN - 1); x++)
    {
        if(tdoRead() == false)
        {
            break;                      // Our 0 has propagated through the entire chain
                                        // 'x' holds the number of devices
        }
    }

    if (x > (MAX_DEVICES_LEN - 1))
    {
        x = 0;
    }
    
    tmsHigh();
    tckPulse();
    tmsHigh();
    tckPulse();
    tmsLow();
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
          tmsHigh();
        }  

        if (array & 1)
            {tdiHigh();}
        else
            {tdiLow();}

        array >>= 1 ;
        tempData <<= 1;
        tempData |=tdoRead();
    }
    return(tempData);
}

uint32_t sendData(uint32_t pattern, int num)
{
    uint32_t tempData;
    tempData=0;
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
    for(x=0; x < (num * MAX_IR_LEN); x++)      // Send in 1s
    {
        tckPulse();
    }

    tmsHigh();
    tckPulse();               // Go to Exit1 IR

    tmsHigh();
    tckPulse();              // Go to Update IR, new instruction in effect

    tmsLow();
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

int calculateJtagPermutations(int totalChannels) 
{
    int result = 1;
    for (int i = 0; i < 4; i++)             // Minimum required pins == 4
    {
        result *= (totalChannels - i);
    }
    return result;
}

void jtagScan(void)
{
    int channelCount;
    uint32_t tempDeviceId;
    bool volatile foundPinout=false;
    jDeviceCount=0;
    channelCount = getChannels();            // First get the number of channels hooked
    progressCount = 0;
    maxPermutations = calculateJtagPermutations(channelCount);
    jTDO, jTCK, jTMS, jTDI,jTRST = 0;
    for(jTDI=0; jTDI<channelCount; jTDI++)
    {
        for(jTDO=0; jTDO < channelCount; jTDO++)
        {
            if (jTDI == jTDO)
            {
                continue;
            }
            for(jTCK =0; jTCK  < channelCount; jTCK++)
            {
                if (jTCK  == jTDO || jTCK == jTDI)
                {
                    continue;
                }
                for(jTMS=0; jTMS < channelCount; jTMS++)
                {                      
                        if (jTMS == jTCK || jTMS == jTDO || jTMS == jTDI)
                        {
                            continue;
                        }
                        // onBoard LED notification
                        gpio_put(onboardLED, 0);
                        
                        progressCount = progressCount+1;
                        printProgress(progressCount, maxPermutations);
                        jtagInitChannels(channelCount);                      
                        if (jPulsePins)
                        {
                            pulsePins(channelCount);
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
                            for(jTRST=0; jTRST < channelCount; jTRST++)
                            {
                                if (jTRST == jTMS || jTRST == jTCK || jTRST == jTDO || jTRST == jTDI)
                                {
                                    continue;
                                }
                                progressCount = progressCount+1;
                                printProgress(progressCount, maxPermutations);
                                
                                gpio_set_dir(jTRST, GPIO_IN);
                                if (jPulsePins)
                                {
                                    gpio_pull_up(jTRST);
                                }
                                else
                                {
                                    gpio_pull_down(jTRST);
                                }

                                sleep_us(10);          // Give device time to react

                                getDeviceIDs(1);
                                if (tempDeviceId != deviceIDs[0] )
                                {
                                    deviceIDs[0]=tempDeviceId;
                                    xTRST=jTRST;
                                    break;
                                }
                            }
                            // Done enumerating everything. 
                            if(foundPinout==true)
                            {
                              jtagResetChannels(channelCount);
                              displayPinout();
                              displayDeviceDetails();
                              // onBoard LED notification
                              gpio_put(onboardLED, 1);
                              return;
                            }                            
                        }
                        // onBoard LED notification
                        jtagResetChannels(channelCount);
                        gpio_put(onboardLED, 1);
                    }
            }
        }
    }
    if( foundPinout == false )
    {
        printProgress(maxPermutations, maxPermutations);
        printf("%s%s", EOL, EOL);
        printf("     No JTAG devices found. Please try again.%s%s", EOL, EOL);
    }

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

uint xSwdClk=0;
uint xSwdIO=1;
bool swdDeviceFound=false;
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
    printf("     Number of channels set to: %d%s%s", x, EOL, EOL);
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
        printf("(mfg: '%s' , part: 0x%x, ver: 0x%x)%s", jep106_table_manufacturer(bank,id), part, ver, EOL);
    }

    printf("%s%s", EOL, EOL);
    printf(" Enable hardware OpenOCD mode for SWD debugging? (y/n): ");
    cmd = getc(stdin);
    if ( cmd == 'y')
    {
        printf("y%s%s", EOL, EOL);
        printf(" [ Ex. Openocd command ]%s%s", EOL, EOL);
        printf("   'openocd -f interface/buspirate.cfg -c \"buspirate port XXXXXXXXXX; transport select swd;\" -f target/stm32f1x.cfg'%s%s", EOL, EOL);
        printf("    Replace 'XXXXXXXXXX' with COM port of blueTag [Ex. '/dev/ttyACM0' (Linux) or 'COM4' (Win)]%s%s", EOL, EOL);    
        resetUART(); 
        initOpenocdMode(OPENOCD_PIN_DEFAULT, OPENOCD_PIN_DEFAULT, OPENOCD_PIN_DEFAULT, OPENOCD_PIN_DEFAULT, xSwdClk, xSwdIO, OPENOCD_MODE_SWD);
    }
    printf("%s%s", EOL, EOL);
}

void swdDisplayPinout(int swdio, int swclk, uint32_t idcode)
{
    printProgress(maxPermutations, maxPermutations);
    printf("%s%s", EOL, EOL);
    printf("     [  Pinout  ]  SWDIO=CH%d", swdio);
    printf(" SWCLK=CH%d%s%s", swclk, EOL, EOL);
    swdDisplayDeviceDetails(idcode);
}

void initSwdPins(void)
{
    gpio_set_dir(xSwdClk,GPIO_OUT);
    gpio_set_dir(xSwdIO,GPIO_OUT);
}

void resetSwdPins(void)
{
    gpio_set_dir(xSwdClk,GPIO_IN);
    gpio_set_dir(xSwdIO,GPIO_IN);
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
    if (buffer > 0)
    {
        swdDisplayPinout(xSwdIO, xSwdClk, buffer);
    }
    else
    {
        swdDeviceFound=false;
    }
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

void swdToJTAG(void)
{
  swdResetLineSWDJ();
  swdWriteBits(SWD_TO_JTAG_CMD, 16);
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
    gpio_put(onboardLED, 0);
    swdTrySWDJ();
    gpio_put(onboardLED, 1);
    if(swdDeviceFound)
    { 
        swdToJTAG();
        return(true); 
    } else 
    { 
        return(false);
    }
}

void swdScan(void)
{     
    swdDeviceFound = false;
    bool result = false;    
    int channelCount = getSwdChannels();
    progressCount = 0;
    maxPermutations = channelCount * (channelCount - 1);
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
            printProgress(progressCount, maxPermutations);
            progressCount++;
            initSwdPins();
            result = swdBruteForce();
            resetSwdPins();
            if (result) break;
        }
        if (result) break; 
    }
    if(swdDeviceFound == false)
    {
        printProgress(maxPermutations, maxPermutations);
        printf("%s%s", EOL, EOL);
        printf("     No devices found. Please try again.%s%s", EOL, EOL);
    }
    // Switch back to JTAG
    swdToJTAG();
}

//--------------------------------------------Main--------------------------------------------------

static void busyLoop(size_t count) 
{
    for (volatile size_t i = 0; i < count; i++);
}

void resetUART(void)
{
    printf(" Press any key to activate... %s%s", EOL, EOL);
    char cmd = getc(stdin);
    printf(BOLD);
    printf(" -- Activated --%s%s", EOL, EOL);
    sleep_ms(4);
    printf(OFF);
    fflush(stdout);
    busyLoop(USB_HOST_RECOGNISE_TIME);
    tud_disconnect();
    busyLoop(USB_HOST_RECOGNISE_TIME);
    tud_connect();
}


void hardwareModeBoot(void)
{
    // Init hardware boot mode pins
    gpio_init(hwBootUSB2SerialPin);
    gpio_init(hwBootFlashromPin);
    gpio_init(hwBootOpenocdPin);
    gpio_init(hwBootCmsisdapPin);    

    // Setup hardware boot mode pins
    gpio_set_dir(hwBootUSB2SerialPin, GPIO_IN);
    gpio_set_dir(hwBootFlashromPin, GPIO_IN);
    gpio_set_dir(hwBootOpenocdPin, GPIO_IN);
    gpio_set_dir(hwBootCmsisdapPin, GPIO_IN);

    // Set pull-ups to avoid false positives
    gpio_set_pulls (hwBootUSB2SerialPin, true, false);
    gpio_set_pulls (hwBootFlashromPin, true, false);
    gpio_set_pulls (hwBootOpenocdPin, true, false);
    gpio_set_pulls (hwBootCmsisdapPin, true, false);

    sleep_ms(50);
    if (gpio_is_pulled_down(hwBootUSB2SerialPin))
    {
        stdio_set_driver_enabled(&stdio_usb, false);
        sleep_ms(50);
        usbMode = USB_MODE_DEFAULT;
        uartMode();
    }
    else if (gpio_is_pulled_down(hwBootFlashromPin))
    {
        initSerProg();
    }
    else if (gpio_is_pulled_down(hwBootOpenocdPin))
    {
        jTCK = OPENOCD_PIN_DEFAULT;
        jTMS = OPENOCD_PIN_DEFAULT;
        jTDI = OPENOCD_PIN_DEFAULT;
        jTDO = OPENOCD_PIN_DEFAULT;
        xSwdClk = OPENOCD_PIN_DEFAULT;
        xSwdIO = OPENOCD_PIN_DEFAULT;
        initOpenocdMode(jTCK, jTMS, jTDI, jTDO, xSwdClk, xSwdIO, OPENOCD_MODE_GENERIC);
    }
    else if (gpio_is_pulled_down(hwBootCmsisdapPin))
    {
        usbMode = USB_MODE_CMSISDAP;
        stdio_set_driver_enabled(&stdio_usb, false);
        tud_disconnect();
        busyLoop(USB_HOST_RECOGNISE_TIME);
        busyLoop(USB_HOST_RECOGNISE_TIME);
        multicore_reset_core1(); 
        busyLoop(USB_HOST_RECOGNISE_TIME);  // Oddly more reliable than busyLoop(USB_HOST_RECOGNISE_TIME * 4)
        busyLoop(USB_HOST_RECOGNISE_TIME);
        busyLoop(USB_HOST_RECOGNISE_TIME);
        busyLoop(USB_HOST_RECOGNISE_TIME);
        cmsisDapInit();
    }
}



int main()
{
    initUART();
    stdio_usb_init();
    gpio_init(onboardLED);
    gpio_set_dir(onboardLED, GPIO_OUT);
    initChannels();
    jPulsePins=false;

    // Check for Hardware modes
    hardwareModeBoot();
    
    //get user input to display splash & menu    
    cmd=getc(stdin);
    splashScreen();
    showMenu();
    showPrompt();

    while(1)
    {
        cmd=getc(stdin);
        printf("%c%s%s", cmd, EOL, EOL);
        switch(cmd)
        {
            // Help menu requested
            case 'h':
                showMenu();
                break;
            case 'v':
                printf(" Current version: %s%s%s", version, EOL, EOL);
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
                    printf("     Pin pulsing activated.%s%s", EOL, EOL);
                }
                else
                {
                    printf("     Pin pulsing deactivated.%s%s", EOL, EOL);
                }                
                break;


            case 'l':
                for(int x=0;x<=25;x++)
                {
                    gpio_put(onboardLED, 1);
                    sleep_ms(250);
                    gpio_put(onboardLED, 0);
                    sleep_ms(250);
                }
                break;

            case 'U':
                printf(BOLD);
                printf(" USB-to-Serial mode%s%s", EOL, EOL);
                printf(OFF);
                printf(" [ Pin configuration to target UART interface ]%s%s", EOL, EOL);               
                printf("          +-------------------------+%s", EOL);
                printf("          | RP2040 pin  | Target    |%s", EOL);
                printf("          +-------------------------+%s", EOL);
                printf("          | GPIO 0 (TX) | RX        |%s", EOL);
                printf("          | GPIO 1 (RX) | TX        |%s", EOL);
                printf("          | GND         | GND       |%s", EOL);
                printf("          |-------------------------|%s", EOL);
                printf("          | Optional:               |%s", EOL);
                printf("          |-------------------------|%s", EOL);
                printf("          | 3V3 Out     | VCC       |%s", EOL);
                printf("          +-------------------------+%s%s", EOL, EOL);
                printf(" [ Information ]%s%s", EOL, EOL);
                printf("   In this mode, blueTag functions like a standard USB-to-Serial programmer,%s", EOL);
                printf("   simply (re)open the COM port and configure the UART settings as required.%s%s", EOL, EOL);
                printf(BOLD);
                printf(" Note:");
                printf(OFF);
                printf(" Connect blueTag's '3V3 Out' pin to the target UART's 'VCC' pin only if the target%s", EOL); 
                printf("       isn't externally powered%s%s", EOL, EOL); 

                printf(" Press any key to activate... %s%s", EOL, EOL);
                char cc = getc(stdin);
                printf(BOLD);
                printf(" -- Activated --%s%s", EOL, EOL);
                printf(OFF);

                stdio_set_driver_enabled(&stdio_usb, false);
                sleep_ms(500);
                usbMode = USB_MODE_DEFAULT;
                uartMode();
                break;

            case 'F':
                printf(BOLD);           
                printf(" Flashrom's Serial Programmer mode %s%s", EOL, EOL);
                printf(OFF);
                printf(" [ Pin configuration to target SPI Flash IC ]%s%s", EOL, EOL);                
                printf("         +-------------------------+%s", EOL);
                printf("         | RP2040 pin  | SPI Flash |%s", EOL);
                printf("         +-------------------------+%s", EOL);
                printf("         | GPIO 1      | CS        |%s", EOL);
                printf("         | GPIO 2      | CLK       |%s", EOL);
                printf("         | GPIO 3      | MOSI / DI |%s", EOL);
                printf("         | GPIO 4      | MISO / DO |%s", EOL);
                printf("         | GND         | GND       |%s", EOL);
                printf("         |-------------------------|%s", EOL);
                printf("         | Optional:               |%s", EOL);
                printf("         |-------------------------|%s", EOL);
                printf("         | 3V3 Out     | VCC       |%s", EOL);
                printf("         +-------------------------+%s%s", EOL, EOL);
                printf(" [ Ex. Flashrom commands ]%s%s", EOL, EOL);
                printf("   Read  : 'flashrom -p serprog:dev=XXXXXXXXXX:115200,spispeed=12M -r flashBackup.bin'%s", EOL);
                printf("   Write : 'flashrom -p serprog:dev=XXXXXXXXXX:115200,spispeed=12M -w flash.bin'%s%s", EOL, EOL);
                printf("   Replace 'XXXXXXXXXX' with COM port of blueTag [Ex. '/dev/ttyACM0' (Linux) or 'COM4' (Win)]%s%s", EOL, EOL);
                printf(BOLD);
                printf(" Note:");
                printf(OFF);
                printf(" Connect blueTag's '3V3 Out' pin to target SPI Flash IC's 'VCC' pin only if the target%s", EOL); 
                printf("       isn't externally powered%s%s", EOL, EOL); 
                resetUART();
                initSerProg();
                break;

            case 'O':
                printf(BOLD);
                printf(" Openocd JTAG/SWD debugger mode %s%s", EOL, EOL);
                printf(OFF);
                printf(" [ Pin configuration to Target ]%s%s", EOL, EOL);
                printf("   +-------------------------+%s", EOL);
                printf("   | RP2040 pin  | Target    |%s", EOL);
                printf("   +-------------------------+%s", EOL);
                printf("   | JTAG mode:              |%s", EOL);
                printf("   |-------------------------|%s", EOL);                
                printf("   | GPIO 0      | TCK       |%s", EOL);
                printf("   | GPIO 1      | TMS       |%s", EOL);
                printf("   | GPIO 2      | TDI       |%s", EOL);
                printf("   | GPIO 3      | TDO       |%s", EOL);
                printf("   | GND         | GND       |%s", EOL);                
                printf("   |-------------------------|%s", EOL);
                printf("   | SWD mode:               |%s", EOL);
                printf("   |-------------------------|%s", EOL);    
                printf("   | GPIO 2      | SWDCLK    |%s", EOL);
                printf("   | GPIO 3      | SWDIO     |%s", EOL);                                            
                printf("   | GND         | GND       |%s", EOL);
                printf("   +-------------------------+%s%s", EOL, EOL);
                printf(" [ Ex. Openocd commands ]%s%s", EOL, EOL);
                printf("   JTAG : 'openocd -f interface/buspirate.cfg -c \"buspirate port XXXXXXXXXX; transport select jtag;\" -f target/esp32.cfg'%s", EOL);
                printf("   SWD  : 'openocd -f interface/buspirate.cfg -c \"buspirate port XXXXXXXXXX; transport select swd;\" -f target/stm32f1x.cfg'%s%s", EOL, EOL);
                printf("   Replace 'XXXXXXXXXX' with COM port of blueTag [Ex. '/dev/ttyACM0' (Linux) or 'COM4' (Win)]%s%s", EOL, EOL);
                resetUART();                
     
                jTCK = OPENOCD_PIN_DEFAULT;
                jTMS = OPENOCD_PIN_DEFAULT;
                jTDI = OPENOCD_PIN_DEFAULT;
                jTDO = OPENOCD_PIN_DEFAULT;
                xSwdClk = OPENOCD_PIN_DEFAULT;
                xSwdIO = OPENOCD_PIN_DEFAULT;
                initOpenocdMode(jTCK, jTMS, jTDI, jTDO, xSwdClk, xSwdIO, OPENOCD_MODE_GENERIC);
                break;

            case 'C':
                printf(BOLD);
                printf(" CMSIS-DAP v1.0 debugger mode %s%s", EOL, EOL);
                printf(OFF);
                printf(" [ Pin configuration to Target ]%s%s", EOL, EOL);                
                printf("   +-------------------------+%s", EOL);
                printf("   | RP2040 pin  | Target    |%s", EOL);
                printf("   +-------------------------+%s", EOL);             
                printf("   | GPIO 0 (TX) | UART RX   |%s", EOL);
                printf("   | GPIO 1 (RX) | UART TX   |%s", EOL);
                printf("   | GPIO 2      | SWCLK     |%s", EOL);
                printf("   | GPIO 3      | SWDIO     |%s", EOL);
                printf("   | GND         | GND       |%s", EOL);                
                printf("   +-------------------------+%s%s", EOL, EOL);
                printf(" [ Ex. Openocd command ]%s%s", EOL, EOL);
                printf("   CMSIS-DAP : 'openocd -f interface/cmsis-dap.cfg -f target/stm32f1x.cfg'%s%s", EOL, EOL);

                printf(" Press any key to activate... %s%s", EOL, EOL);
                char cmd = getc(stdin);
                printf(BOLD);
                printf(" -- Activated --%s%s", EOL, EOL);
                sleep_ms(4);
                printf(OFF);
                fflush(stdout);
                busyLoop(USB_HOST_RECOGNISE_TIME);
                usbMode = USB_MODE_CMSISDAP;
                stdio_set_driver_enabled(&stdio_usb, false);
                sleep_ms(50);
                tud_disconnect();
                multicore_reset_core1(); 
                busyLoop(USB_HOST_RECOGNISE_TIME);
                cmsisDapInit();
                break;

            default:
                printf(" Unknown command %s%s", EOL, EOL);
                break;
        }
        showPrompt();
    }    
    return 0;
}

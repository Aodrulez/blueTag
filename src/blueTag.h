#define  MAX_DEVICES_LEN    32                             // Maximum number of devices allowed in a single JTAG chain
#define  MIN_IR_LEN          2                             // Minimum length of instruction register per IEEE Std. 1149.1
#define  MAX_IR_LEN         32                             // Maximum length of instruction register
#define  MAX_IR_CHAIN_LEN   MAX_DEVICES_LEN * MAX_IR_LEN   // Maximum total length of JTAG chain w/ IR selected
#define  MAX_DR_LEN         4096                           // Maximum length of data register
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*array))
#define CR		    13
#define LF		    10
//#define ONBOARD_LED 25 // If not defined, onboard LED will not be used
#define UNUSED_GPIO 28 // Unused in source
#define MAX_NUM_JTAG 32 // Unused in source
#define START_CHANNEL 0 // First GPIO pin to use 0 - 16 by default
#define MAX_CHANNELS 16 // Max number of channels supported by Pico  (upto 16 on a bare board)

struct jtagScan_t
{
    bool jPulsePins;
    uint jTDI;           
    uint jTDO;
    uint jTCK;
    uint jTMS;
    uint jTRST;
    uint xTDI;           
    uint xTDO;
    uint xTCK;
    uint xTMS;
    uint xTRST;
    uint channelCount;
    uint progressCount;
    uint maxPermutations;
    uint jDeviceCount;
    uint32_t deviceIDs[MAX_DEVICES_LEN]; // Array to store identified device IDs
};


struct swdScan_t
{
    uint xSwdClk;
    uint xSwdIO;
    uint channelCount;
    uint progressCount;
    uint maxPermutations;
    bool swdDeviceFound;
};

void bluetag_jPulsePins_set(bool jPulsePins);
void bluetag_progressbar_cleanup(uint maxPermutations);
bool jtagScan(uint channelCount);
bool swdScan(struct swdScan_t *swd);
extern char *version;

// JTAG IO functions

// Function that sets all used channels to output high
static inline void setPinsHigh(uint startChannel, uint channelCount)
{
    for(int x = startChannel; x < (channelCount+startChannel); x++)
    {
        gpio_put(x, 1);
    }
}

static inline void setPinsLoW(uint startChannel, uint channelCount)
{
    for(int x = startChannel; x < (channelCount+startChannel); x++)
    {
        gpio_put(x, 0);
    }
}


// Initialize all available channels & set them as output
static inline void initChannels(uint startChannel, uint maxChannels)
{
    for(int x=startChannel; x < (maxChannels+startChannel) ; x++)
    {
        gpio_init(x);
        gpio_set_dir(x, GPIO_OUT);
    }   
}

static inline void jtagConfig(uint tdiPin, uint tdoPin, uint tckPin, uint tmsPin)
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
static inline bool tdoRead(uint jTCK, uint jTDO)
{
    bool volatile tdoStatus;
    gpio_put(jTCK, 1);
    tdoStatus=gpio_get(jTDO);
    gpio_put(jTCK, 0);
    return(tdoStatus);
}

static inline void tdiHigh(uint jTDI)
{
    gpio_put(jTDI, 1);
}

static inline void tdiLow(uint jTDI)
{
    gpio_put(jTDI, 0);
}

static inline void tmsHigh(uint jTMS)
{
    gpio_put(jTMS, 1);
}

static inline void tmsLow(uint jTMS)
{
    gpio_put(jTMS, 0);
}

static inline void trstHigh(uint jTRST)
{
    gpio_put(jTRST, 1);
}

static inline void trstLow(uint jTRST)
{
    gpio_put(jTRST, 0);
}

// SWD IO functions

static inline void initSwdPins(uint xSwdClk, uint xSwdIO)
{
    gpio_set_dir(xSwdClk,GPIO_OUT);
    gpio_set_dir(xSwdIO,GPIO_OUT);
}

static inline void swdClockPulse(uint xSwdClk, uint swd_delay)
{
    gpio_put(xSwdClk, 0);
    sleep_us(swd_delay);
    gpio_put(xSwdClk, 1);
    sleep_us(swd_delay);
}

static inline void swdSetReadMode(uint xSwdIO)
{
    gpio_set_dir(xSwdIO,GPIO_IN);
}

static inline void swdSetWriteMode(uint xSwdIO)
{
    gpio_set_dir(xSwdIO,GPIO_OUT);
}

static inline void swdIOHigh(uint xSwdIO)
{
    gpio_put(xSwdIO, 1);
}

static inline void swdIOLow(uint xSwdIO)
{
    gpio_put(xSwdIO, 0);
}

static inline void swdWriteHigh(uint xSwdClk,uint xSwdIO, uint swd_delay)
{
    gpio_put(xSwdIO, 1);
    swdClockPulse(xSwdClk, swd_delay);
}

static inline void swdWriteLow(uint xSwdClk,uint xSwdIO, uint swd_delay)
{
    gpio_put(xSwdIO, 0);
    swdClockPulse(xSwdClk, swd_delay);
}

static inline void swdWriteBit(uint xSwdIO, uint xSwdClk, bool value, uint swd_delay)
{
    gpio_put(xSwdIO, value);
    //swdClockPulse();
    swdClockPulse(xSwdClk, swd_delay);
}

static inline bool swdReadBit(uint xSwdClk, uint xSwdIO, uint swd_delay)
{
    bool value=gpio_get(xSwdIO);
    swdClockPulse(xSwdClk, swd_delay);
    return(value);
}


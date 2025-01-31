void bluetag_jPulsePins_set(bool jPulsePins);
void bluetag_progressbar_cleanup(void);
bool jtagScan(int channelCount);
bool swdScan(int channelCount);
extern char *version;

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

static inline bool swdReadBit(uint xSwdClk, uint xSwdIO, uint swd_delay)
{
    bool value=gpio_get(xSwdIO);
    swdClockPulse(xSwdClk, swd_delay);
    return(value);
}

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


char *version="1.0";
#define  MAX_DEVICES_LEN    32                             // Maximum number of devices allowed in a single JTAG chain
#define  MIN_IR_LEN          2                             // Minimum length of instruction register per IEEE Std. 1149.1
#define  MAX_IR_LEN         32                             // Maximum length of instruction register
#define  MAX_IR_CHAIN_LEN   MAX_DEVICES_LEN * MAX_IR_LEN   // Maximum total length of JTAG chain w/ IR selected
#define  MAX_DR_LEN         4096                           // Maximum length of data register



const uint onboardLED = 25;
const uint unusedGPIO = 28;                               // Pins on Pico are accessed using GPIO names
const uint MAX_NUM_JTAG  = 32;
const uint maxChannels = 9;                               // Max number of channels supported by Pico  

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


// MFG details for JTAG based on jep106 (old - known issue)
char * mfg[9][126]={{"AMD","AMI","Fairchild","Fujitsu","GTE","Harris","Hitachi","Inmos","Intel","I.T.T.","Intersil","Monolithic Memories","Mostek","Freescale (Motorola)","National","NEC","RCA","Raytheon","Conexant (Rockwell)","Seeq","NXP (Philips)","Synertek","Texas Instruments","Toshiba","Xicor","Zilog","Eurotechnique","Mitsubishi","Lucent (AT&T)","Exel","Atmel","STMicroelectronics","Lattice Semi.","NCR","Wafer Scale Integration","IBM","Tristar","Visic","Intl. CMOS Technology","SSSI","MicrochipTechnology","Ricoh Ltd.","VLSI","Micron Technology","SK Hynix","OKI Semiconductor","ACTEL","Sharp","Catalyst","Panasonic","IDT","Cypress","DEC","LSI Logic","Zarlink (Plessey)","UTMC","Thinking Machine","Thomson CSF","Integrated CMOS (Vertex)","Honeywell","Tektronix","Oracle Corporation","Silicon Storage Technology","ProMos/Mosel Vitelic","Infineon (Siemens)","Macronix","Xerox","Plus Logic","SanDisk Corporation","Elan Circuit Tech.","European Silicon Str.","Apple Computer","Xilinx","Compaq","Protocol Engines","SCI","Seiko Instruments","Samsung","I3 Design System","Klic","Crosspoint Solutions","Alliance Semiconductor","Tandem","Hewlett-Packard","Integrated Silicon Solutions","Brooktree","New Media","MHS Electronic","Performance Semi.","Winbond Electronic","Kawasaki Steel","Bright Micro","TECMAR","Exar","PCMCIA","LG Semi (Goldstar)","Northern Telecom","Sanyo","Array Microsystems","Crystal Semiconductor","Analog Devices","PMC-Sierra","Asparix","Convex Computer","Quality Semiconductor","Nimbus Technology","Transwitch","Micronas (ITT Intermetall)","Cannon","Altera","NEXCOM","Qualcomm","Sony","Cray Research","AMS(Austria Micro)","Vitesse","Aster Electronics","Bay Networks (Synoptic)","Zentrum/ZMD","TRW","Thesys","Solbourne Computer","Allied-Signal","Dialog Semiconductor","Media Vision","Numonyx Corporation",},{"Cirrus Logic","National Instruments","ILC Data Device","Alcatel Mietec","Micro Linear","Univ. of NC","JTAG Technologies","BAE Systems (Loral)","Nchip","Galileo Tech","Bestlink Systems","Graychip","GENNUM","VideoLogic","Robert Bosch","Chip Express","DATARAM","United Microelectronics Corp.","TCSI","Smart Modular","Hughes Aircraft","Lanstar Semiconductor","Qlogic","Kingston","Music Semi","Ericsson Components","SpaSE","Eon Silicon Devices","Integrated Silicon Solution (ISSI)","DoD","Integ. Memories Tech.","Corollary Inc.","Dallas Semiconductor","Omnivision","EIV(Switzerland)","Novatel Wireless","Zarlink (Mitel)","Clearpoint","Cabletron","STEC (Silicon Tech)","Vanguard","Hagiwara Sys-Com","Vantis","Celestica","Century","Hal Computers","Rohm Company Ltd.","Juniper Networks","Libit Signal Processing","Mushkin Enhanced Memory","Tundra Semiconductor","Adaptec Inc.","LightSpeed Semi.","ZSP Corp.","AMIC Technology","Adobe Systems","Dynachip","PNY Technologies, Inc.","Newport Digital","MMC Networks","T Square","Seiko Epson","Broadcom","Viking Components","V3 Semiconductor","Flextronics (Orbit Semiconductor)","Suwa Electronics","Transmeta","Micron CMS","American Computer & Digital","Enhance 3000 Inc.","Tower Semiconductor","CPU Design","Price Point","Maxim Integrated Product","Tellabs","Centaur Technology","Unigen Corporation","Transcend Information","Memory Card Technology","CKD Corporation Ltd.","Capital Instruments, Inc.","Aica Kogyo, Ltd.","Linvex Technology","MSC Vertriebs GmbH","AKM Company, Ltd.","Dynamem, Inc.","NERA ASA","GSI Technology","Dane-Elec (C Memory)","Acorn Computers","Lara Technology","Oak Technology, Inc.","Itec Memory","Tanisys Technology","Truevision","Wintec Industries","Super PC Memory","MGV Memory","Galvantech","Gadzoox Networks","Multi Dimensional Cons.","GateField","Integrated Memory System","Triscend","XaQti","Goldenram","Clear Logic","Cimaron Communications","Nippon Steel Semi. Corp.","Advantage Memory","AMCC","LeCroy","Yamaha Corporation","Digital Microwave","NetLogic Microsystems","MIMOS Semiconductor","Advanced Fibre","BF Goodrich Data.","Epigram","Acbel Polytech Inc.","Apacer Technology","Admor Memory","FOXCONN","Quadratics Superconductor","3COM",},{"Solectron","Optosys Technologies","Buffalo (Formerly Melco)","TriMedia Technologies","Cyan Technologies","Global Locate","Optillion","Terago Communications","Ikanos Communications","Princeton Technology","Nanya Technology","Elite Flash Storage","Mysticom","LightSand Communications","ATI Technologies","Agere Systems","NeoMagic","AuroraNetics","Golden Empire","Mushkin","Tioga Technologies","Netlist","TeraLogic","Cicada Semiconductor","Centon Electronics","Tyco Electronics","Magis Works","Zettacom","Cogency Semiconductor","Chipcon AS","Aspex Technology","F5 Networks","Programmable Silicon Solutions","ChipWrights","Acorn Networks","Quicklogic","Kingmax Semiconductor","BOPS","Flasys","BitBlitz Communications","eMemory Technology","Procket Networks","Purple Ray","Trebia Networks","Delta Electronics","Onex Communications","Ample Communications","Memory Experts Intl","Astute Networks","Azanda Network Devices","Dibcom","Tekmos","API NetWorks","Bay Microsystems","Firecron Ltd","Resonext Communications","Tachys Technologies","Equator Technology","Concept Computer","SILCOM","3Dlabs","c’t Magazine","Sanera Systems","Silicon Packets","Viasystems Group","Simtek","Semicon Devices Singapore","Satron Handelsges","Improv Systems","INDUSYS GmbH","Corrent","Infrant Technologies","Ritek Corp","empowerTel Networks","Hypertec","Cavium Networks","PLX Technology","Massana Design","Intrinsity","Valence Semiconductor","Terawave Communications","IceFyre Semiconductor","Primarion","Picochip Designs Ltd","Silverback Systems","Jade Star Technologies","Pijnenburg Securealink","takeMS - Ultron AG","Cambridge Silicon Radio","Swissbit","Nazomi Communications","eWave System","Rockwell Collins","Picocel Co. Ltd. (Paion)","Alphamosaic Ltd","Sandburst","SiCon Video","NanoAmp Solutions","Ericsson Technology","PrairieComm","Mitac International","Layer N Networks","MtekVision (Atsana)","Allegro Networks","Marvell Semiconductors","Netergy Microelectronic","NVIDIA","Internet Machines","Memorysolution GmbH","Litchfield Communication","Accton Technology","Teradiant Networks","Scaleo Chip","Cortina Systems","RAM Components","Raqia Networks","ClearSpeed","Matsushita Battery","Xelerated","SimpleTech","Utron Technology","Astec International","AVM gmbH","Redux Communications","Dot Hill Systems","TeraChip",},{"T-RAM Incorporated","Innovics Wireless","Teknovus","KeyEye Communications","Runcom Technologies","RedSwitch","Dotcast","Silicon Mountain Memory","Signia Technologies","Pixim","Galazar Networks","White Electronic Designs","Patriot Scientific","Neoaxiom Corporation","3Y Power Technology","Scaleo Chip","Potentia Power Systems","C-guys Incorporated","Digital Communications Technology","Silicon-Based Technology","Fulcrum Microsystems","Positivo Informatica Ltd","XIOtech Corporation","PortalPlayer","Zhiying Software","ParkerVision, Inc.","Phonex Broadband","Skyworks Solutions","Entropic Communications","I’M Intelligent Memory Ltd.","Zensys A/S","Legend Silicon Corp.","Sci-worx GmbH","SMSC (Standard Microsystems)","Renesas Electronics","Raza Microelectronics","Phyworks","MediaTek","Non-cents Productions","US Modular","Wintegra Ltd.","Mathstar","StarCore","Oplus Technologies","Mindspeed","Just Young Computer","Radia Communications","OCZ","Emuzed","LOGIC Devices","Inphi Corporation","Quake Technologies","Vixel","SolusTek","Kongsberg Maritime","Faraday Technology","Altium Ltd.","Insyte","ARM Ltd.","DigiVision","Vativ Technologies","Endicott Interconnect Technologies","Pericom","Bandspeed","LeWiz Communications","CPU Technology","Ramaxel Technology","DSP Group","Axis Communications","Legacy Electronics","Chrontel","Powerchip Semiconductor","MobilEye Technologies","Excel Semiconductor","A-DATA Technology","VirtualDigm","G Skill Intl","Quanta Computer","Yield Microelectronics","Afa Technologies","KINGBOX Technology Co. Ltd.","Ceva","iStor Networks","Advance Modules","Microsoft","Open-Silicon","Goal Semiconductor","ARC International","Simmtec","Metanoia","Key Stream","Lowrance Electronics","Adimos","SiGe Semiconductor","Fodus Communications","Credence Systems Corp.","Genesis Microchip Inc.","Vihana, Inc.","WIS Technologies","GateChange Technologies","High Density Devices AS","Synopsys","Gigaram","Enigma Semiconductor Inc.","Century Micro Inc.","Icera Semiconductor","Mediaworks Integrated Systems","O’Neil Product Development","Supreme Top Technology Ltd.","MicroDisplay Corporation","Team Group Inc.","Sinett Corporation","Toshiba Corporation","Tensilica","SiRF Technology","Bacoc Inc.","SMaL Camera Technologies","Thomson SC","Airgo Networks","Wisair Ltd.","SigmaTel","Arkados","Compete IT gmbH Co. KG","Eudar Technology Inc.","Focus Enhancements","Xyratex",},{"Specular Networks","Patriot Memory (PDP Systems)","U-Chip Technology Corp.","Silicon Optix","Greenfield Networks","CompuRAM GmbH","Stargen, Inc.","NetCell Corporation","Excalibrus Technologies Ltd","SCM Microsystems","Xsigo Systems, Inc.","CHIPS & Systems Inc","Tier 1 Multichip Solutions","CWRL Labs","Teradici","Gigaram, Inc.","g2 Microsystems","PowerFlash Semiconductor","P.A. Semi, Inc.","NovaTech Solutions, S.A.","c2 Microsystems, Inc.","Level5 Networks","COS Memory AG","Innovasic Semiconductor","02IC Co. Ltd","Tabula, Inc.","Crucial Technology","Chelsio Communications","Solarflare Communications","Xambala Inc.","EADS Astrium","Terra Semiconductor, Inc.","Imaging Works, Inc.","Astute Networks, Inc.","Tzero","Emulex","Power-One","Pulse~LINK Inc.","Hon Hai Precision Industry","White Rock Networks Inc.","Telegent Systems USA, Inc.","Atrua Technologies, Inc.","Acbel Polytech Inc.","eRide Inc.","ULi Electronics Inc.","Magnum Semiconductor Inc.","neoOne Technology, Inc.","Connex Technology, Inc.","Stream Processors, Inc.","Focus Enhancements","Telecis Wireless, Inc.","uNav Microelectronics","Tarari, Inc.","Ambric, Inc.","Newport Media, Inc.","VMTS","Enuclia Semiconductor, Inc.","Virtium Technology Inc.","Solid State System Co., Ltd.","Kian Tech LLC","Artimi","Power Quotient International","Avago Technologies","ADTechnology","Sigma Designs","SiCortex, Inc.","Ventura Technology Group","eASIC","M.H.S. SAS","Micro Star International","Rapport Inc.","Makway International","Broad Reach Engineering Co.","Semiconductor Mfg Intl Corp","SiConnect","FCI USA Inc.","Validity Sensors","Coney Technology Co. Ltd.","Spans Logic","Neterion Inc.","Qimonda","New Japan Radio Co. Ltd.","Velogix","Montalvo Systems","iVivity Inc.","Walton Chaintech","AENEON","Lorom Industrial Co. Ltd.","Radiospire Networks","Sensio Technologies, Inc.","Nethra Imaging","Hexon Technology Pte Ltd","CompuStocx (CSX)","Methode Electronics, Inc.","Connect One Ltd.","Opulan Technologies","Septentrio NV","Goldenmars Technology Inc.","Kreton Corporation","Cochlear Ltd.","Altair Semiconductor","NetEffect, Inc.","Spansion, Inc.","Taiwan Semiconductor Mfg","Emphany Systems Inc.","ApaceWave Technologies","Mobilygen Corporation","Tego","Cswitch Corporation","Haier (Beijing) IC Design Co.","MetaRAM","Axel Electronics Co. Ltd.","Tilera Corporation","Aquantia","Vivace Semiconductor","Redpine Signals","Octalica","InterDigital Communications","Avant Technology","Asrock, Inc.","Availink","Quartics, Inc.","Element CXI","Innovaciones Microelectronicas","VeriSilicon Microelectronics","W5 Networks",},{"MOVEKING","Mavrix Technology, Inc.","CellGuide Ltd.","Faraday Technology","Diablo Technologies, Inc.","Jennic","Octasic","Molex Incorporated","3Leaf Networks","Bright Micron Technology","Netxen","NextWave Broadband Inc.","DisplayLink","ZMOS Technology","Tec-Hill","Multigig, Inc.","Amimon","Euphonic Technologies, Inc.","BRN Phoenix","InSilica","Ember Corporation","Avexir Technologies Corporation","Echelon Corporation","Edgewater Computer Systems","XMOS Semiconductor Ltd.","GENUSION, Inc.","Memory Corp NV","SiliconBlue Technologies","Rambus Inc.","Andes Technology Corporation","Coronis Systems","Achronix Semiconductor","Siano Mobile Silicon Ltd.","Semtech Corporation","Pixelworks Inc.","Gaisler Research AB","Teranetics","Toppan Printing Co. Ltd.","Kingxcon","Silicon Integrated Systems","I-O Data Device, Inc.","NDS Americas Inc.","Solomon Systech Limited","On Demand Microelectronics","Amicus Wireless Inc.","SMARDTV SNC","Comsys Communication Ltd.","Movidia Ltd.","Javad GNSS, Inc.","Montage Technology Group","Trident Microsystems","Super Talent","Optichron, Inc.","Future Waves UK Ltd.","SiBEAM, Inc.","Inicore,Inc.","Virident Systems","M2000, Inc.","ZeroG Wireless, Inc.","Gingle Technology Co. Ltd.","Space Micro Inc.","Wilocity","Novafora, Inc.","iKoa Corporation","ASint Technology","Ramtron","Plato Networks Inc.","IPtronics AS","Infinite-Memories","Parade Technologies Inc.","Dune Networks","GigaDevice Semiconductor","Modu Ltd.","CEITEC","Northrop Grumman","XRONET Corporation","Sicon Semiconductor AB","Atla Electronics Co. Ltd.","TOPRAM Technology","Silego Technology Inc.","Kinglife","Ability Industries Ltd.","Silicon Power Computer &","Augusta Technology, Inc.","Nantronics Semiconductors","Hilscher Gesellschaft","Quixant Ltd.","Percello Ltd.","NextIO Inc.","Scanimetrics Inc.","FS-Semi Company Ltd.","Infinera Corporation","SandForce Inc.","Lexar Media","Teradyne Inc.","Memory Exchange Corp.","Suzhou Smartek Electronics","Avantium Corporation","ATP Electronics Inc.","Valens Semiconductor Ltd","Agate Logic, Inc.","Netronome","Zenverge, Inc.","N-trig Ltd","SanMax Technologies Inc.","Contour Semiconductor Inc.","TwinMOS","Silicon Systems, Inc.","V-Color Technology Inc.","Certicom Corporation","JSC ICC Milandr","PhotoFast Global Inc.","InnoDisk Corporation","Muscle Power","Energy Micro","Innofidei","CopperGate Communications","Holtek Semiconductor Inc.","Myson Century, Inc.","FIDELIX","Red Digital Cinema","Densbits Technology","Zempro","MoSys","Provigent","Triad Semiconductor, Inc.",},{"Siklu Communication Ltd.","A Force Manufacturing Ltd.","Strontium","ALi Corp (Abilis Systems)","Siglead, Inc.","Ubicom, Inc.","Unifosa Corporation","Stretch, Inc.","Lantiq Deutschland GmbH","Visipro.","EKMemory","Microelectronics Institute ZTE","u-blox AG","Carry Technology Co. Ltd.","Nokia","King Tiger Technology","Sierra Wireless","HT Micron","Albatron Technology Co. Ltd.","Leica Geosystems AG","BroadLight","AEXEA","ClariPhy Communications, Inc.","Green Plug","Design Art Networks","Mach Xtreme Technology Ltd.","ATO Solutions Co. Ltd.","Ramsta","Greenliant Systems, Ltd.","Teikon","Antec Hadron","NavCom Technology, Inc.","Shanghai Fudan Microelectronics","Calxeda, Inc.","JSC EDC Electronics","Kandit Technology Co. Ltd.","Ramos Technology","Goldenmars Technology","XeL Technology Inc.","Newzone Corporation","ShenZhen MercyPower Tech","Nanjing Yihuo Technology","Nethra Imaging Inc.","SiTel Semiconductor BV","SolidGear Corporation","Topower Computer Ind Co Ltd.","Wilocity","Profichip GmbH","Gerad Technologies","Ritek Corporation","Gomos Technology Limited","Memoright Corporation","D-Broad, Inc.","HiSilicon Technologies","Syndiant Inc..","Enverv Inc.","Cognex","Xinnova Technology Inc.","Ultron AG","Concord Idea Corporation","AIM Corporation","Lifetime Memory Products","Ramsway","Recore Systems B.V.","Haotian Jinshibo Science Tech","Being Advanced Memory","Adesto Technologies","Giantec Semiconductor, Inc.","HMD Electronics AG","Gloway International (HK)","Kingcore","Anucell Technology Holding","Accord Software & Systems Pvt. Ltd.","Active-Semi Inc.","Denso Corporation","TLSI Inc.","Qidan","Mustang","Orca Systems","Passif Semiconductor","GigaDevice Semiconductor (Beijing)","Memphis Electronic","Beckhoff Automation GmbH","Harmony Semiconductor Corp","Air Computers SRL","TMT Memory","Eorex Corporation","Xingtera","Netsol","Bestdon Technology Co. Ltd.","Baysand Inc.","Uroad Technology Co. Ltd.","Wilk Elektronik S.A.","AAI","Harman","Berg Microelectronics Inc.","ASSIA, Inc.","Visiontek Products LLC","OCMEMORY","Welink Solution Inc.","Shark Gaming","Avalanche Technology","R&D Center ELVEES OJSC","KingboMars Technology Co. Ltd.","High Bridge Solutions Industria","Transcend Technology Co. Ltd.","Everspin Technologies","Hon-Hai Precision","Smart Storage Systems","Toumaz Group","Zentel Electronics Corporation","Panram International Corporation","Silicon Space Technology","LITE-ON IT Corporation","Inuitive","HMicro","BittWare, Inc.","GLOBALFOUNDRIES","ACPI Digital Co. Ltd.","Annapurna Labs","AcSiP Technology Corporation","Idea! Electronic Systems","Gowe Technology Co. Ltd.","Hermes Testing Solutions, Inc.","Positivo BGH","Intelligence Silicon Technology",},{"3D PLUS","Diehl Aerospace","Fairchild","Mercury Systems","Sonics, Inc.","GE Intelligent Platforms GmbH & Co.","Shenzhen Jinge Information Co. Ltd.","SCWW","Silicon Motion Inc.","Anurag","King Kong","FROM30 Co. Ltd.","Gowin Semiconductor Corp","Fremont Micro Devices Ltd.","Ericsson Modems","Exelis","Satixfy Ltd.","Galaxy Microsystems Ltd.","Gloway International Co. Ltd.","Lab","Smart Energy Instruments","Approved Memory Corporation","Axell Corporation","Essencore Limited","Phytium","Xi’an SinoChip Semiconductor","Ambiq Micro","eveRAM Technology, Inc.","Infomax","Butterfly Network, Inc.","Shenzhen City Gcai Electronics","Stack Devices Corporation","ADK Media Group","TSP Global Co., Ltd.","HighX","Shenzhen Elicks Technology","ISSI/Chingis","Google, Inc.","Dasima International Development","Leahkinn Technology Limited","HIMA Paul Hildebrandt GmbH Co KG","Keysight Technologies","Techcomp International (Fastable)","Ancore Technology Corporation","Nuvoton","Korea Uhbele International Group Ltd.","Ikegami Tsushinki Co Ltd.","RelChip, Inc.","Baikal Electronics","Nemostech Inc.","Memorysolution GmbH","Silicon Integrated Systems Corporation","Xiede","Multilaser Components","Flash Chi","Jone","GCT Semiconductor Inc.","Hong Kong Zetta Device Technology","Unimemory Technology(s) Pte Ltd.","Cuso","Kuso","Uniquify Inc.","Skymedi Corporation","Core Chance Co. Ltd.","Tekism Co. Ltd.","Seagate Technology PLC","Hong Kong Gaia Group Co. Limited","Gigacom Semiconductor LLC","V2 Technologies","TLi","Neotion","Lenovo","Shenzhen Zhongteng Electronic Corp. Ltd.","Compound Photonics","in2H2 inc","Shenzhen Pango Microsystems Co. Ltd","Vasekey","Cal-Comp Industria de Semicondutores","Eyenix Co., Ltd.","Heoriady","Accelerated Memory Production Inc.","INVECAS, Inc.","AP Memory","Douqi Technology","Etron Technology, Inc.","Indie Semiconductor","Socionext Inc.","HGST","EVGA","Audience Inc.","EpicGear","Vitesse Enterprise Co.","Foxtronn International Corporation","Bretelon Inc.","Graphcore","Eoplex Inc","MaxLinear, Inc.","ETA Devices","LOKI","IMS Electronics Co., Ltd.","Dosilicon Co., Ltd.","Dolphin Integration","Shenzhen Mic Electronics Technology","Boya Microelectronics Inc.","Geniachip (Roche)","Axign","Kingred Electronic Technology Ltd.","Chao Yue Zhuo Computer Business Dept.","Guangzhou Si Nuo Electronic Technology.","Crocus Technology Inc.","Creative Chips GmbH","GE Aviation Systems LLC.","Asgard","Good Wealth Technology Ltd.","TriCor Technologies","Nova-Systems GmbH","JUHOR","Zhuhai Douke Commerce Co. Ltd.","DSL Memory","Anvo-Systems Dresden GmbH","Realtek","AltoBeam","Wave Computing","Beijing TrustNet Technology Co. Ltd.","Innovium, Inc.","Starsway Technology Limited",},{"Weltronics Co. LTD","VMware, Inc.","Hewlett Packard Enterprise","INTENSO","Puya Semiconductor","MEMORFI","MSC Technologies GmbH","Txrui","SiFive, Inc.","Spreadtrum Communications","Paragon Technology (Shenzhen) Ltd.","UMAX Technology","Shenzhen Yong Sheng Technology","SNOAMOO (Shenzhen Kai Zhuo Yue)","Daten Tecnologia LTDA","Shenzhen XinRuiYan Electronics","Eta Compute","Energous","Raspberry Pi Trading Ltd.","Shenzhen Chixingzhe Tech Co. Ltd.",},};


void splashScreen(void)
{
    printf("\n%s",banner);
    printf("\n");
    printf("\n          [   JTAGulator alternative based on Raspberry Pi Pico   ]");
    printf("\n          +-------------------------------------------------------+");
    printf("\n          | @Aodrulez         https://github.com/Aodrulez/blueTag |");
    printf("\n          +-------------------------------------------------------+\n\n");   
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
            //printf("%s", mfg[bank][id-1]);
            //printf("(mfg: '"+String(mfg[bank][id-1])+"' , part: 0x"+String(part,HEX)+", ver: 0x"+String(ver,HEX)+")");
            printf("(mfg: '%s' , part: 0x%x, ver: 0x%x)\n",mfg[bank][id-1], part, ver);
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
                printf("bypass scan");
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

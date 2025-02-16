// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2021 Álvaro Fernández Rojas <noltari@gmail.com>
 *
 * This file is based on a file originally part of the
 * MicroPython project, http://micropython.org/
 *
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2019 Damien P. George
 */

#include <hardware/flash.h>
#include <pico/stdlib.h>
#include <pico/rand.h> 
#include <tusb.h>

#define DESC_STR_MAX 20

#define USBD_VID 0xCAFE 
#define USBD_PID 0xD00D 

#define USBD_DESC_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN * CFG_TUD_CDC)
#define USBD_MAX_POWER_MA 500

#define USBD_ITF_CDC_0 0
#define USBD_ITF_CDC_1 2
#define USBD_ITF_MAX 2

#define USBD_CDC_0_EP_CMD 0x81
#define USBD_CDC_0_EP_OUT 0x01
#define USBD_CDC_0_EP_IN 0x82


#define USBD_CDC_CMD_MAX_SIZE 8
#define USBD_CDC_IN_OUT_MAX_SIZE 64

#define USBD_STR_0 0x00
#define USBD_STR_MANUF 0x01
#define USBD_STR_PRODUCT 0x02
#define USBD_STR_SERIAL 0x03
#define USBD_STR_SERIAL_LEN 17
#define USBD_STR_CDC 0x04

#define USB_MODE_DEFAULT    0
#define USB_MODE_CMSISDAP   1
extern volatile int usbMode;

char usbdSerial[USBD_STR_SERIAL_LEN] = "000000000000";

// Helps avoid data corruption when switching hardware modes
void generateRandomSerial(void) 
{
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int charsetSize = sizeof(charset) - 1;
    for (int i = 0; i < 17; i++) {
        uint32_t randomValue = get_rand_32();  
        usbdSerial[i] = charset[randomValue % charsetSize];
    }
    usbdSerial[17] = '\0';
}

static const tusb_desc_device_t desc_device_default = 
{
	.bLength = sizeof(tusb_desc_device_t),
	.bDescriptorType    = TUSB_DESC_DEVICE,
	.bcdUSB             = 0x0200,
	.bDeviceClass       = TUSB_CLASS_MISC,
	.bDeviceSubClass    = MISC_SUBCLASS_COMMON,
	.bDeviceProtocol    = MISC_PROTOCOL_IAD,
	.bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
	.idVendor           = USBD_VID,
	.idProduct          = USBD_PID,
	.bcdDevice          = 0x0100,
	.iManufacturer      = USBD_STR_MANUF,
	.iProduct           = USBD_STR_PRODUCT,
	.iSerialNumber      = USBD_STR_SERIAL,
	.bNumConfigurations = 1,
};

static const uint8_t usbd_desc_cfg_default[USBD_DESC_LEN] = 
{
	TUD_CONFIG_DESCRIPTOR(1, USBD_ITF_MAX, USBD_STR_0, USBD_DESC_LEN,TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, USBD_MAX_POWER_MA),
	TUD_CDC_DESCRIPTOR(USBD_ITF_CDC_0, USBD_STR_CDC, USBD_CDC_0_EP_CMD, USBD_CDC_CMD_MAX_SIZE, USBD_CDC_0_EP_OUT, USBD_CDC_0_EP_IN, USBD_CDC_IN_OUT_MAX_SIZE),    
};

//--------- CMSIS-DAP -------------------

tusb_desc_device_t const desc_device_cmsisdap =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USBD_VID,
    .idProduct          = USBD_PID,
    .bcdDevice          = 0x0102,
    .iManufacturer      = USBD_STR_MANUF,
    .iProduct           = USBD_STR_PRODUCT,
    .iSerialNumber      = USBD_STR_SERIAL,
    .bNumConfigurations = 0x01
};

enum
{
  ITF_NUM_HID,
  ITF_NUM_CDC_COM,
  ITF_NUM_CDC_DATA,
  ITF_NUM_TOTAL
};

#define  CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

#define EPNUM_HID       0x01
#define EPNUM_CDC_NOTIF 0x83
#define EPNUM_CDC_OUT   0x02
#define EPNUM_CDC_IN    0x82

static uint8_t const desc_hid_report[] =
{
  TUD_HID_REPORT_DESC_GENERIC_INOUT(CFG_TUD_HID_EP_BUFSIZE)
};

uint8_t const * tud_hid_descriptor_report_cb(uint8_t itf)
{
  (void) itf;
  return desc_hid_report;
}

uint8_t desc_configuration_cmsisdap[] =
{
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
  TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID, 0x80 | EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 1),
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_COM, 0, EPNUM_CDC_NOTIF, 64, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
};

enum
{
  STRID_LANGID = 0,
  STRID_MANUFACTURER,
  STRID_PRODUCT,
  STRID_SERIAL,
};

static const char *const usbd_desc_str_default[] = {
	[USBD_STR_MANUF]   = "Aodrulez",
	[USBD_STR_PRODUCT] = "blueTag",
	[USBD_STR_SERIAL]  = usbdSerial,
	[USBD_STR_CDC]     = "blueTag CDC",
};

char const* string_desc_arr_cmsisdap[] =
{
  [STRID_LANGID]       = (const char[]) { 0x09, 0x04 }, 
  [STRID_MANUFACTURER] = "Aodrulez",                     
  [STRID_PRODUCT]      = "blueTag CMSIS-DAP",    
};


uint8_t const * tud_descriptor_device_cb(void)
{
  if (usbMode == USB_MODE_CMSISDAP)
  {
    return (uint8_t const*) &desc_device_cmsisdap;
  }
  else
  {
    return (uint8_t const*) &desc_device_default;
  }
}

static uint8_t get_unique_id(uint16_t *desc_str)
{
  uint8_t i;
  for(i=0; i<TU_ARRAY_SIZE(usbdSerial); i++)
  {
    desc_str[i] = usbdSerial[i];
  }
  return i;
}

uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
  (void) index; 
  if (usbMode == USB_MODE_CMSISDAP)
  {
    return desc_configuration_cmsisdap;
  }
  else
  {
    return usbd_desc_cfg_default;
  }
}

const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  static uint16_t _desc_str[32];
  //usbdSerial_init();
  generateRandomSerial();
  if (usbMode == USB_MODE_CMSISDAP)
  {      
      (void) langid;
      uint8_t chr_count = 0;
      if (0 == index)
      {
        memcpy(&_desc_str[1], string_desc_arr_cmsisdap[0], 2);
        chr_count = 1;
      } else if (STRID_SERIAL == index)
      {
        chr_count = get_unique_id(_desc_str + 1);
      } else
      {
        if ( !(index < sizeof(string_desc_arr_cmsisdap)/sizeof(string_desc_arr_cmsisdap[0])) ) return NULL;
        const char* str = string_desc_arr_cmsisdap[index];
        chr_count = TU_MIN(strlen(str), 31);
        for(uint8_t i=0; i<chr_count; i++)
        {
          _desc_str[1+i] = str[i];
        }
      }
      _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);      
  }
  else
  {      
      uint8_t len;
      if (index == 0) {
        _desc_str[1] = 0x0409;
        len = 1;
      } else {
        const char *str;
        char serial[USBD_STR_SERIAL_LEN];

        if (index >= sizeof(usbd_desc_str_default) / sizeof(usbd_desc_str_default[0]))
          return NULL;

        str = usbd_desc_str_default[index];
        for (len = 0; len < DESC_STR_MAX - 1 && str[len]; ++len)
          _desc_str[1 + len] = str[len];
      }
      _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * len + 2);
  }
  return _desc_str;
}
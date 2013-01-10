#ifndef _DEVO8_TARGET_H_
#define _DEVO8_TARGET_H_

#define VECTOR_TABLE_LOCATION 0x3000
#define SPIFLASH_SECTOR_OFFSET 0
#define SPIFLASH_SECTORS 512

#define NO_STANDARD_GUI     1
#define NO_LANGUAGE_SUPPORT 1
#define NO_INI_SUPPORT      1

#define MIN_BRIGHTNESS 0
#define HAS_TOUCH 0
#define HAS_VIBRATINGMOTOR 1
#define HAS_LOGICALVIEW 1
#define DEFAULT_BATTERY_ALARM 4100
#define DEFAULT_BATTERY_CRITICAL 3900


//Protocols
#define PROTO_HAS_CYRF6936
//#define PROTO_HAS_A7105

#define NUM_OUT_CHANNELS 12
#define NUM_VIRT_CHANNELS 10

#define NUM_TRIMS 4
#define MAX_POINTS 13
#define NUM_MIXERS ((NUM_OUT_CHANNELS + NUM_VIRT_CHANNELS) * 4)

#define INP_HAS_CALIBRATION 4

#define CHAN_ButtonMask(btn) (btn ? (1 << (btn - 1)) : 0)
#endif //_DEVO8_TARGET_H_

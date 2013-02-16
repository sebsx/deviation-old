#ifndef _DEVO10_TARGET_H_
#define _DEVO10_TARGET_H_

#define VECTOR_TABLE_LOCATION 0x4000
#define SPIFLASH_SECTOR_OFFSET 54
#define SPIFLASH_SECTORS 1024

#define MIN_BRIGHTNESS 0
#define HAS_TOUCH 0
#define HAS_VIBRATINGMOTOR 1
#define HAS_LOGICALVIEW 1
#define DEFAULT_BATTERY_ALARM 8000
#define DEFAULT_BATTERY_CRITICAL 7500
#define MAX_BATTERY_ALARM 12000
#define MIN_BATTERY_ALARM 5500

//Protocols
#define PROTO_HAS_CYRF6936
#define PROTO_HAS_A7105

#define NUM_OUT_CHANNELS 12
#define NUM_VIRT_CHANNELS 10

#define NUM_TRIMS 6
#define MAX_POINTS 13
#define NUM_MIXERS ((NUM_OUT_CHANNELS + NUM_VIRT_CHANNELS) * 4)

#define INP_HAS_CALIBRATION 6

#define CHAN_ButtonMask(btn) (btn ? (1 << (btn - 1)) : 0)
#endif //_DEVO10_TARGET_H_

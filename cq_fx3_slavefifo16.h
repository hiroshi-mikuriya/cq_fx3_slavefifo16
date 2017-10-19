
#ifndef _INCLUDED_CQ_FX_SLAVEFIFO16_H_
#define _INCLUDED_CQ_FX_SLAVEFIFO16_H_

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3externcstart.h"

#define DMA_BUF_COUNT	2
#define DMA_TX_SIZE	0
#define THREAD_STACK		0x1000
#define THREAD_PRIORITY		8
#define BURST_LENGTH		8

#define EP_CONSUMER		0x82

/* Extern definitions for the USB Descriptors */
extern const uint8_t USB20DeviceDscr[];
extern const uint8_t USB30DeviceDscr[];
extern const uint8_t USBDeviceQualDscr[];
extern const uint8_t USBFSConfigDscr[];
extern const uint8_t USBHSConfigDscr[];
extern const uint8_t USBBOSDscr[];
extern const uint8_t USBSSConfigDscr[];
extern const uint8_t USBStringLangIDDscr[];
extern const uint8_t USBManufactureDscr[];
extern const uint8_t USBProductDscr[];

#include "cyu3externcend.h"

#endif /* _INCLUDED_CQ_FX_SLAVEFIFO16_H_ */

/*[]*/

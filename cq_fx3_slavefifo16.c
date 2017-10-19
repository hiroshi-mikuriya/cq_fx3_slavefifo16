#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cq_fx3_slavefifo16.h"
#include "cyu3usb.h"
#include "cyu3gpif.h"
#include "cyu3spi.h"
#include "sync_slave_fifo_2bit.cydsn/cyfxgpif2config.h"

CyU3PThread gThread;
CyU3PDmaChannel gDma;
CyBool_t gActive = CyFalse;

void ThreadEntry (uint32_t);
void InitGpif(void);
void InitUsb(void);
void InitSpi(void);
CyBool_t SetupCallback(uint32_t, uint32_t);
void EventCallback(CyU3PUsbEventType_t, uint16_t);
CyBool_t LPMRequestCallback(CyU3PUsbLinkPowerMode);
void InitDma (void);
void DeinitDma (void);

int main (void)
{
  CyU3PDeviceInit (0);
  CyU3PDeviceCacheControl (CyTrue, CyFalse, CyFalse);
  CyU3PIoMatrixConfig_t io_cfg = {
    .isDQ32Bit = CyFalse,
    .s0Mode = CY_U3P_SPORT_INACTIVE,
    .s1Mode = CY_U3P_SPORT_INACTIVE,
    .useUart   = CyFalse,
    .useI2C    = CyFalse,
    .useI2S    = CyFalse,
    .useSpi    = CyTrue,
    .lppMode   = CY_U3P_IO_MATRIX_LPP_DEFAULT,
    .gpioSimpleEn[0]  = 0,
    .gpioSimpleEn[1]  = 0,
    .gpioComplexEn[0] = 0,
    .gpioComplexEn[1] = 0,
  };
  CyU3PDeviceConfigureIOMatrix (&io_cfg);
  CyU3PKernelEntry ();
  return 0;
}

void CyFxApplicationDefine ()
{
  uint32_t retThrdCreate = CyU3PThreadCreate (
    &gThread,
    "1:slavefifo16",
    ThreadEntry,
    0,
    CyU3PMemAlloc (THREAD_STACK),
    THREAD_STACK,
    THREAD_PRIORITY,
    THREAD_PRIORITY,
    CYU3P_NO_TIME_SLICE,
    CYU3P_AUTO_START
  );
  if (retThrdCreate != 0)
  {
    while(1);
  }
}

void ThreadEntry (uint32_t input)
{
  InitGpif();
  InitUsb();
  InitSpi();
  for(;;){
    CyU3PThreadSleep(1000);
  }
}

void InitGpif()
{
  CyU3PGpifLoad(&CyFxGpifConfig);
  CyU3PGpifSocketConfigure(2, CY_U3P_PIB_SOCKET_2, 4, CyFalse, 1);
  CyU3PGpifSMStart(RESET, ALPHA_RESET);
}

void InitUsb()
{
  CyU3PUsbStart();
  CyU3PUsbRegisterSetupCallback(SetupCallback, CyTrue);
  CyU3PUsbRegisterEventCallback(EventCallback);
  CyU3PUsbRegisterLPMRequestCallback(LPMRequestCallback);
  CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_DEVICE_DESCR, 0, (uint8_t *)USB30DeviceDscr);
  CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_DEVICE_DESCR, 0, (uint8_t *)USB20DeviceDscr);
  CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR, 0, (uint8_t *)USBBOSDscr);
  CyU3PUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR, 0, (uint8_t *)USBDeviceQualDscr);
  CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR, 0, (uint8_t *)USBSSConfigDscr);
  CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR, 0, (uint8_t *)USBHSConfigDscr);
  CyU3PUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR, 0, (uint8_t *)USBFSConfigDscr);
  CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *)USBStringLangIDDscr);
  CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *)USBManufactureDscr);
  CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *)USBProductDscr);
  CyU3PConnectState(CyTrue, CyTrue);
}

void InitSpi()
{
  CyU3PSpiInit();
  CyU3PSpiConfig_t spiConfig = {
    .isLsbFirst = CyFalse,
    .cpol = CyFalse,
    .ssnPol = CyFalse,
    .cpha = CyFalse,
    .leadTime = CY_U3P_SPI_SSN_LAG_LEAD_HALF_CLK,
    .lagTime = CY_U3P_SPI_SSN_LAG_LEAD_HALF_CLK,
    .ssnCtrl = CY_U3P_SPI_SSN_CTRL_FW,
    .clock = 10 * 1000 * 1000,
    .wordLen = 8,
  };
  CyU3PSpiSetConfig(&spiConfig, 0);
}

CyBool_t SetupCallback (uint32_t setupdat0, uint32_t setupdat1)
{
  CyBool_t isHandled = CyFalse;
  uint8_t bReqType = setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK;
  uint8_t bType = bReqType & CY_U3P_USB_TYPE_MASK;
  uint8_t bTarget = bReqType & CY_U3P_USB_TARGET_MASK;
  uint8_t bRequest
    = (setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS;
  uint16_t wValue
   = (setupdat0 & CY_U3P_USB_VALUE_MASK) >> CY_U3P_USB_VALUE_POS;
  uint16_t wIndex
   =((setupdat1 & CY_U3P_USB_INDEX_MASK) >> CY_U3P_USB_INDEX_POS;

  if (bType == CY_U3P_USB_STANDARD_RQT) {
    if ((bTarget == CY_U3P_USB_TARGET_INTF)
        && ((bRequest == CY_U3P_USB_SC_SET_FEATURE)
        || (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE))
        && (wValue == 0)) {
      if (gActive)
        CyU3PUsbAckSetup();
      else
        CyU3PUsbStall(0, CyTrue, CyFalse);
      isHandled = CyTrue;
    }
    if ((bTarget == CY_U3P_USB_TARGET_ENDPT)
        && (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)
        && (wValue == CY_U3P_USBX_FS_EP_HALT)) {
      if (wIndex == EP_CONSUMER) {
        if (gActive) {
          DeinitDma();
          CyU3PThreadSleep(1);
          InitDma();
          CyU3PUsbStall(wIndex, CyFalse, CyTrue);
          CyU3PUsbAckSetup();
          isHandled = CyTrue;
        }
      }
    }
  }
  if (bRequest == 0xA0 && bReqType == 0x40) {
    uint8_t buf[64] = { 0 };
    uint16_t len = 0;
    CyU3PUsbGetEP0Data(sizeof(buf), buf, &len);
    CyU3PSpiSetSsnLine(CyFalse);
    CyU3PSpiTransmitWords(buf, len);
    CyU3PSpiSetSsnLine(CyTrue);
    CyU3PUsbAckSetup();
    isHandled = CyTrue;
  }
  return isHandled;
}

void EventCallback (CyU3PUsbEventType_t evtype, uint16_t evdata)
{
  switch (evtype) {
    case CY_U3P_USB_EVENT_SETCONF:
      if (gActive) {
        DeinitDma();
      }
      InitDma();
      break;
    case CY_U3P_USB_EVENT_RESET:
    case CY_U3P_USB_EVENT_DISCONNECT:
      if (gActive) {
        DeinitDma();
      }
      break;
    default:
      break;
  }
}

CyBool_t LPMRequestCallback(CyU3PUsbLinkPowerMode link_mode)
{
  return CyTrue;
}

void InitDma ()
{
  uint16_t size = 0;
  CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed();
  switch (usbSpeed) {
    case CY_U3P_FULL_SPEED:
      size = 64;
      break;
    case CY_U3P_HIGH_SPEED:
      size = 512;
      break;
    case CY_U3P_SUPER_SPEED:
      size = 1024;
      break;
    default:
      while (1) {
        CyU3PThreadSleep(100);
      }
      break;
  }
  CyU3PEpConfig_t epCfg = {
    .enable = CyTrue,
    .epType = CY_U3P_USB_EP_BULK,
    .burstLen
      = (CY_U3P_SUPER_SPEED == usbSpeed) ? BURST_LENGTH : 1,
    .streams = 0,
    .pcktSize = size
  };
  CyU3PSetEpConfig(EP_CONSUMER, &epCfg);
  CyU3PUsbFlushEp(EP_CONSUMER);
  CyU3PDmaChannelConfig_t dmaCfg = {
    .size  = size * epCfg.burstLen,
    .count = DMA_BUF_COUNT,
    .dmaMode = CY_U3P_DMA_MODE_BYTE,
    .notification = 0,
    .cb = 0,
    .prodHeader = 0,
    .prodFooter = 0,
    .consHeader = 0,
    .prodAvailCount = 0,
    .prodSckId = CY_U3P_PIB_SOCKET_2,
    .consSckId = CY_U3P_UIB_SOCKET_CONS_2
  };
  CyU3PDmaChannelCreate(&gDma, CY_U3P_DMA_TYPE_AUTO, &dmaCfg);
  CyU3PDmaChannelSetXfer(&gDma, DMA_TX_SIZE);
  gActive = CyTrue;
}

void DeinitDma ()
{
  gActive = CyFalse;
  CyU3PDmaChannelDestroy (&gDma);
  CyU3PUsbFlushEp(EP_CONSUMER);
  CyU3PEpConfig_t epCfg = { 0 };
  CyU3PSetEpConfig(EP_CONSUMER, &epCfg);
}

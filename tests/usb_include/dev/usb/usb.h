/* Host-test subset of the FreeBSD USB types used by hci_usb.c. These are not
 * ABI definitions and are never used in the PS5 build. */
#ifndef TEST_USB_H
#define TEST_USB_H
#include <stdint.h>
#define UT_WRITE_CLASS_DEVICE 0x20
#define USETW(p,v) do { (p)[0]=(uint8_t)(v); (p)[1]=(uint8_t)((v)>>8); } while (0)
#define UGETW(p) ((unsigned)(p)[0] | (unsigned)(p)[1]<<8)
struct usb_device_descriptor {
    uint8_t idVendor[2], idProduct[2], bcdUSB[2];
    uint8_t bDeviceClass, bDeviceSubClass, bDeviceProtocol, bMaxPacketSize;
};
struct usb_device_request {
    uint8_t bmRequestType, bRequest, wValue[2], wIndex[2], wLength[2];
};
#endif

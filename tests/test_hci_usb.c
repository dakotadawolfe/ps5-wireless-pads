/* Production USB transport with a 16-byte endpoint and a competing reader.
 * A short request loses the remainder; a whole-event request receives it. */
#include "../src/hci_usb.h"
#include <dev/usb/usb_ioctl.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static struct usb_fs_endpoint *endpoints;
static unsigned event_capacity;
static int pending, delivered, closed, length;
static unsigned char payload[257];

int mock_open(const char *path, int flags, ...)
{ (void)flags; assert(strcmp(path, "/dev/ugen0.2") == 0); return 42; }
int mock_close(int fd) { assert(fd == 42); closed++; return 0; }
int mock_ioctl(int fd, unsigned long request, ...)
{
    va_list ap;
    void *arg;
    assert(fd == 42);
    va_start(ap, request);
    arg = va_arg(ap, void *);
    va_end(ap);
    switch (request) {
    case USB_GET_DEVICE_DESC: {
        struct usb_device_descriptor *d = arg;
        memset(d, 0, sizeof *d);
        USETW(d->idVendor, 0x0e8d);
        USETW(d->idProduct, 0x3603);
        return 0;
    }
    case USB_GET_FULL_DESC: {
        static const uint8_t cfg[] = {
            9,4,0,0,3,0xe0,1,1,0,
            7,5,0x81,3,16,0,1,
            7,5,0x82,2,0,2,0,
            7,5,0x01,2,0,2,0
        };
        struct usb_gen_descriptor *d = arg;
        assert(d->ugd_maxlen >= sizeof cfg);
        memcpy(d->ugd_data, cfg, sizeof cfg);
        d->ugd_actlen = sizeof cfg;
        return 0;
    }
    case USB_FS_INIT:
        endpoints = ((struct usb_fs_init *)arg)->pEndpoints;
        return 0;
    case USB_FS_OPEN: {
        struct usb_fs_open *op = arg;
        if (op->ep_no == 0x81) event_capacity = op->max_bufsize;
        return 0;
    }
    case USB_FS_START:
        if (((struct usb_fs_start *)arg)->ep_index == 0) pending = 1;
        return 0;
    case USB_FS_COMPLETE:
        if (pending && !delivered) {
            unsigned n = endpoints[0].pLength[0];
            if (n > (unsigned)length) n = (unsigned)length;
            assert(n <= event_capacity);
            memcpy(endpoints[0].ppBuffer[0], payload, n);
            endpoints[0].pLength[0] = n;
            endpoints[0].aFrames = 1;
            endpoints[0].status = 0;
            ((struct usb_fs_complete *)arg)->ep_index = 0;
            pending = 0;
            delivered = 1;
            return 0;
        }
        errno = EAGAIN;
        return -1;
    case USB_FS_UNINIT:
        return 0;
    default:
        errno = ENOTTY;
        return -1;
    }
}

int main(void)
{
    const int sizes[] = { 44, 68, 257 };
    unsigned i;
    for (i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
        hci_t hci;
        unsigned char received[300];
        length = sizes[i];
        memset(payload, 0x55, sizeof payload);
        payload[0] = 0x3E;
        payload[1] = (unsigned char)(length - 2);
        pending = delivered = 0;
        assert(hci_usb_open(&hci));
        assert(hci.ops->pump(hci.ctx, 0) == 1);
        assert(hci.ops->next_event(hci.ctx, received, sizeof received) == length);
        assert(memcmp(received, payload, (size_t)length) == 0);
        hci.ops->close(hci.ctx);
    }
    assert(closed == 3);
    puts("whole-event USB transport checks passed");
    return 0;
}

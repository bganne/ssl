#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include "tun.h"

/*
 * from linux/Documentation/networking/tuntap.txt
 * dev must be big enough to hold IFNAMSIZ
 */
int tun_alloc(const char *fmt, char *name)
{
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        perror("open(/dev/net/tun) failed");
        return -1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
     *        IFF_TAP   - TAP device
     *
     *        IFF_NO_PI - Do not provide packet information
     */
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    if (*fmt) strncpy(ifr.ifr_name, fmt, IFNAMSIZ);

    int err = ioctl(fd, TUNSETIFF, (void *)&ifr);
    if (err < 0) {
        perror("ioctl(TUNSETIFF) failed");
        close(fd);
        return err;
    }

    strcpy(name, ifr.ifr_name);
    return fd;
}

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <unistd.h>

int tun_alloc(char *dev)
{
    struct ifreq ifr;
    int fd, err;

    if( (fd = open("/dev/net/tun", O_RDWR)) < 0 ) {
        fprintf(stderr, "Can't open /dev/net/tun\n");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));

    /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
     *        IFF_TAP   - TAP device
     *
     *        IFF_NO_PI - Do not provide packet information
     */
    ifr.ifr_flags = IFF_TUN;
    if(*dev)
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
        fprintf(stderr, "Can't setup tun\n");
        close(fd);
        return err;
    }
    fprintf(stderr, "Created tun: %s\n", ifr.ifr_name);
    return fd;
}

int main(int argc, char **argv) {
    int tun_fd;
    struct pollfd poll_fds[2];
    char tun_buf[65535];
    char stdin_buf[65537];
    size_t stdin_buf_len = 0;

    if(argc != 2) {
        fprintf(stderr, "Usage: tunfilter <tun_name>\n");
        return 2;
    }

    tun_fd = tun_alloc(argv[1]);
    if(tun_fd < 0)
        return 1;

    poll_fds[0].fd = tun_fd;
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = 0; // stdin
    poll_fds[1].events = POLLIN;

    // Forward from stdin to tun, from tun to stdout
    while(1) {
        if(poll(poll_fds, 2, -1) == -1) {
            fprintf(stderr, "poll error\n");
            return 1;
        }

        if((poll_fds[0].revents * POLLIN) != 0) {
            uint16_t encoded_len;
            ssize_t len = read(tun_fd, tun_buf, 65535); // read tun
            if(len < 0) {
                fprintf(stderr, "Error reading from tun\n");
                return 1;
            }
            encoded_len = htons((uint16_t)len);
            if(write(1, &encoded_len, 2) == -1) {
                fprintf(stderr, "Error writing to stdout\n");
                return 1;
            }
            if(write(1, tun_buf, len) == -1) { // write stdout
                fprintf(stderr, "Error writing to stdout\n");
                return 1;
            }
            fprintf(stderr, ">[%d]", (int)len);
        }

        if((poll_fds[1].revents & POLLIN) != 0) {
            // read stdin
            ssize_t len;
            if(stdin_buf_len < 2) {
                // read length of packet
                len = read(0, stdin_buf + stdin_buf_len, 2 - stdin_buf_len);
                if(len < 0) {
                    fprintf(stderr, "Error reading from stdin\n");
                    return 1;
                }
                stdin_buf_len += len;
                if(stdin_buf_len == 2) {
                    uint16_t decoded_len =
                        ((uint16_t)(uint8_t)stdin_buf[1])
                        | (((uint16_t)(uint8_t)stdin_buf[0]) << 8);
                    fprintf(stderr, "<[%d..]", (int)decoded_len);
                }
            } else {
                // read packet
                uint16_t decoded_len =
                    ((uint16_t)(uint8_t)stdin_buf[1])
                    | (((uint16_t)(uint8_t)stdin_buf[0]) << 8);
                len = read(0, stdin_buf + stdin_buf_len, decoded_len + 2 - stdin_buf_len);
                if(len < 0) {
                    fprintf(stderr, "Error reading from stdin\n");
                    return 1;
                }
                stdin_buf_len += len;
                if(stdin_buf_len == (size_t)decoded_len + 2) {
                    if(write(tun_fd, stdin_buf + 2, decoded_len) == -1) { // write tun
                        fprintf(stderr, "Error writing to tun\n");
                    }
                    fprintf(stderr, "<[%d]", (int)decoded_len);
                    stdin_buf_len = 0;
                }
            }
        }
    }
}

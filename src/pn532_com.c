#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "pn532_com.h"


typedef struct {
    uint8_t preamble;
    uint8_t start_code1;
    uint8_t start_code2;
    uint8_t length;
    uint8_t length_checksum;
    uint8_t frame_data[256];
    uint8_t data_checksum;
    uint8_t postamble;
} pn532_frame_t;


#define PN532_PREAMBLE      0x00
#define PN532_STARTCODE1    0x00
#define PN532_STARTCODE2    0xFF
#define PN532_POSTAMBLE     0x00

#define PN532_HOSTTOPN532   0xD4
#define PN532_TFI_RECEIVE   0xD5
#define PN532_ACK_FRAME    {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00}

#define PN532_SERIAL_SPEED B115200

/* Function to open serial port */
int pn532_open(pn532_t *pn532, const char *device) {
    struct termios options;
    int flags;

    pn532->fd = open(device, O_RDWR | O_NOCTTY);
    if (pn532->fd == -1) {
        perror("Unable to open serial port");
        return -1;
    }

    tcgetattr(pn532->fd, &options);

    options.c_cflag = CS8 | CLOCAL | CREAD | HUPCL;
    cfsetispeed(&options, PN532_SERIAL_SPEED);
    cfsetospeed(&options, PN532_SERIAL_SPEED);

    options.c_iflag = 0;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(pn532->fd, TCIFLUSH);

    // We do blocking I/O
    options.c_cc[VMIN] = 1;   // Must read at least 1 byte before returning
    options.c_cc[VTIME] = 0;  // No timeout (wait indefinitely)

    tcsetattr(pn532->fd, TCSANOW, &options);

    // Set DTR and RTS
//    flags = TIOCM_DTR;
//    ioctl(pn532->fd, TIOCMBIS, &flags); // Enable DTR
    flags = TIOCM_RTS;
    ioctl(pn532->fd, TIOCMBIS, &flags); // Enable RTS

    // Flush input buffer
    tcflush(pn532->fd, TCIFLUSH);

    flags = TIOCM_DTR;
    ioctl(pn532->fd, TIOCMBIC, &flags); // Clear DTR
    flags = TIOCM_RTS;
    ioctl(pn532->fd, TIOCMBIC, &flags); // Clear RTS

    usleep(100000);

    tcgetattr(pn532->fd, &options);

    return 0;
}

/* Function to close serial port */
void pn532_close(pn532_t *pn532) {
    if (pn532->fd != -1) {
        close(pn532->fd);
        pn532->fd = -1;
    }
}

int pn532_is_pn532killer(pn532_t *pn532)
{
    int ret;

    if (ret = pn532_send_command(pn532, checkPn532Killer, NULL, 0)) return ret;
    if (ret = pn532_wait_response(pn532, checkPn532Killer)) return ret;

    if (ret == 0) return 1;
    return 0;
}

int pn532_set_normal_mode(pn532_t *pn532)
{
    int ret;
    uint8_t cmd[14] = {0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    if (ret = pn532_write(pn532, cmd, sizeof(cmd))) return ret;
    cmd[0] = 0x01;
    if (ret = pn532_send_command(pn532, SAMConfiguration, cmd, 1)) return ret;
    if (pn532_read(pn532, cmd, 1) < 1) return -1;

    return 0;
}

/* Function to write data to PN532 */
int pn532_write(pn532_t *pn532, uint8_t *data, size_t len) {
    ssize_t written = write(pn532->fd, data, len);
    if (written < 0) {
        perror("Write error");
        return -1;
    }
    return 0;
}

/* Function to read data from PN532 */
int pn532_read(pn532_t *pn532, uint8_t *buffer, size_t len) {
    ssize_t read_bytes = 0, ret;

    do {
        ret = read(pn532->fd, buffer+read_bytes, len);
        if (ret < 0) {
            perror("Read error");
            return -1;
        }
        read_bytes += ret;
    } while (ret && read_bytes < len);
    return (int)read_bytes;
}

/* Function to send command and get response */
int pn532_send_command(pn532_t *pn532, uint8_t cmd, uint8_t *data, size_t data_len) {
    uint8_t packet[data_len + 10];
    uint8_t checksum, len_byte;
    size_t idx = 0, i;

    packet[idx++] = PN532_PREAMBLE;
    packet[idx++] = PN532_STARTCODE1;
    packet[idx++] = PN532_STARTCODE2;

    len_byte = 2 + data_len;
    packet[idx++] = len_byte;
    packet[idx++] = ~len_byte + 1;

    packet[idx++] = PN532_HOSTTOPN532;
    packet[idx++] = cmd;

    checksum = PN532_HOSTTOPN532 + cmd;
    for (i = 0; i < data_len; i++) {
        packet[idx++] = data[i];
        checksum += data[i];
    }

    packet[idx++] = ~checksum + 1;
    packet[idx++] = PN532_POSTAMBLE;

    if (pn532_write(pn532, packet, idx) < 0) return -1;

    return 0;
}

// response should be uint8_t buffer[260];
//  0 Success
// -1 Read error
// -2 
// -3 Length checksum error
// -4 Data checksum error
// -5 Postamble error
// -6 Data frame TFI error
// -7 Data frame length error
int pn532_read_response(pn532_t *pn532, pn532_result_t *response)
{
    int i, ret = 0;
    uint8_t data_checksum;
    pn532_frame_t frame;

    if (!response) response = &pn532->result;

    do {
        if (pn532_read(pn532, &frame.preamble, 1) != 1)
            return -1;
    }
    while (frame.preamble != PN532_PREAMBLE);

    if (pn532_read(pn532, &frame.start_code1, 3) != 3)
        return -1;

    if (pn532_read(pn532, &frame.length_checksum, frame.length+1) != frame.length+1)
        return -1;

    if (frame.length && (frame.length + frame.length_checksum) & 0xFF  != 0) {
        return -3;    // Length checksum error
    }

    if (frame.length)
    {
        for (i=0, data_checksum = 0; i < frame.length; i++) {
            data_checksum += frame.frame_data[i];
        }

        if (pn532_read(pn532, &frame.data_checksum, 1) != 1)
            return -1;

        if ((frame.data_checksum + data_checksum) & 0xFF != 0)
            return -4; // Data checksum error
    }

    if (pn532_read(pn532, &frame.postamble, 1) != 1)
        return -1;

    if (frame.postamble != 0)
        return -5; // Postamble error

    if (ret || frame.length == 0) return ret;

    if (frame.frame_data[0] != PN532_TFI_RECEIVE)
        return -6;

    if (frame.length < 2) return -7;

    response->cmd = frame.frame_data[1] - 1;
    response->status = SUCCESS;
    response->len = frame.length-2;
    memcpy(response->data, frame.frame_data+2, response->len);

    if (response->cmd == InCommunicateThru || response->cmd == InDataExchange && frame.length > 2) {
        response->status = frame.frame_data[2];
        if (frame.frame_data[2] == 0 && frame.length > 16)
            memcpy(response->data, frame.frame_data+3, --response->len);
    }

    return 0;
}

int pn532_wait_response(pn532_t *pn532, uint8_t cmd)
{
    int ret;

    pn532->result.cmd = 0;
    while ((ret = pn532_read_response(pn532, NULL)) == 0) {
        if (pn532->result.cmd == cmd) break;
    }

    return ret;
}

char *pn532_strerror(int ret)
{
    switch (ret)
    {
    case  0: return "Success";
    case -1: return "Read/Write error";
    case -3: return "Length checksum error";
    case -4: return "Data checksum error";
    case -5: return "Postamble error";
    case -6: return "Data frame TFI error";
    case -7: return "Data frame length error";
    default: return "Unknown error";
    }
}

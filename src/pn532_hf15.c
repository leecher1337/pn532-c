/* pn532_hf15.c - ANSI C implementation of hf_15 commands */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pn532_com.h"
#include "pn532_hf15.h"
#include "crc16.h"


#define PN532_CMD_INLISTPASSIVETARGET 0x4A
#define PN532_CMD_INDATAEXCHANGE      0x40

/* Read single block */
int hf15_read_block(pn532_t *pn532, uint8_t block_num, uint8_t *response, uint8_t response_len) {
    uint8_t cmd[3] = {0x01, 0x20, 0x00};
    int ret;

    cmd[2] = block_num;
    if (ret = pn532_send_command(pn532, InDataExchange, cmd, 3)) return ret;
    if (ret = pn532_wait_response(pn532, InDataExchange)) return ret;

    if (pn532->result.len == 5 && pn532->result.data[0] == HF_TAG_OK)
    {
        memcpy(response, &pn532->result.data[1], pn532->result.len-1);
        return 0;
    }

    return -1;
}

/* Write single block */
int hf15_write_block(pn532_t *pn532, uint8_t block_num, uint8_t *data, uint8_t len) {
    uint8_t cmd[len + 3];
    int ret;

    cmd[0] = 0x01;
    cmd[1] = 0x21;
    cmd[2] = block_num;
    memcpy(&cmd[3], data, len);
    if (ret = pn532_send_command(pn532, InDataExchange, cmd, len+3)) return ret;
    if (ret = pn532_wait_response(pn532, InDataExchange)) return ret;

    return pn532->result.len != 1 || pn532->result.data[0] != HF_TAG_OK;
}

// 1 - A Command failed
// 2 - Verify failed
int hf15_write_block_verify(pn532_t *pn532, uint8_t block_num, uint8_t *data, uint8_t len) {
    int ret;
    uint8_t buffer[len];

    if (ret = hf15_write_block(pn532, block_num, data, len)) return ret;
    if (ret = hf15_read_block(pn532, block_num, buffer, len)) return ret;
    if (memcmp(data, buffer, len)) return 2;

    return 0;
}

/* Raw command */
int hf15_raw(pn532_t *pn532, uint8_t *cmd_data, size_t cmd_len, int select_tag, int append_crc, int no_check_response) {
    uint8_t cmd[cmd_len + 4];
    size_t len;
    hf15_tag_scan scan;
    int ret;

    if (select_tag) hf15_scan(pn532, &scan);

    cmd[0] = no_check_response?0x00:0x80;
    cmd[1] = 0x00;
    memcpy(cmd + 2, cmd_data, cmd_len);
    len = cmd_len + 2;
    if (append_crc) {
        uint16_t crc = crc16(cmd_data, cmd_len);
        cmd[len] = (crc>>8) & 0xFF;
        cmd[len+1] = crc & 0xFF;
        len += 2;
    }

    if ((ret = pn532_send_command(pn532, InCommunicateThru, cmd, len))) return ret;
    if ((ret = pn532_wait_response(pn532, InCommunicateThru))) return ret;

    return ret;
}

/* Set Generation 1 UID */
int hf15_set_gen1_uid(pn532_t *pn532, uint8_t *uid, uint8_t block_size) {
    int ret;

    if ((ret = hf15_write_block(pn532, block_size, &uid[4], 4))) return ret;
    if ((ret = hf15_write_block(pn532, block_size +1, uid, 4))) return ret;

    return ret;
}

/* Set Generation 2 UID */
int hf15_set_gen2_uid(pn532_t *pn532, uint8_t *uid) {
    uint8_t cmd[8] = {0x02, 0xE0, 0x09, 0x40, 0x00, 0x00, 0x00, 0x00};
    int ret;

    memcpy(cmd + 4, &uid[4], 4);
    if ((ret = hf15_raw(pn532, cmd, sizeof(cmd), 0, 1, 1))) return ret;
    cmd[3] = 0x41;
    memcpy(cmd + 4, &uid[0], 4);
    if ((ret = hf15_raw(pn532, cmd, sizeof(cmd), 0, 1, 1))) return ret;

    return ret;
}

/* Set Generation 2 Configuration */
int hf15_set_gen2_config(pn532_t *pn532, uint8_t size, uint8_t adi, uint8_t dsfid, uint8_t ic_reference) {
    uint8_t cmd[8] = {0x02, 0xE0, 0x09, 0x46, 0x00, 0x00, 0x00, 0x00};
    int ret;

    cmd[6] = adi;
    cmd[7] = dsfid;
    if ((ret = hf15_raw(pn532, cmd, sizeof(cmd), 0, 1, 1))) return ret;
    cmd[3] = 0x47;
    cmd[4] = size - 1;
    cmd[5] = 0x03;
    cmd[6] = ic_reference;
    cmd[7] = 0x00;
    if ((ret = hf15_raw(pn532, cmd, sizeof(cmd), 0, 1, 1))) return ret;

    return ret;
}

/* Emulator Commands */

static int upload_data_block(pn532_t *pn532, uint8_t type, uint8_t slot, uint16_t index, uint8_t *extra, int extra_len) {
    uint8_t cmd[4 + extra_len];
    int ret;

    cmd[0] = type;
    cmd[1] = slot;
    cmd[2] = (index>>8) & 0xFF;
    cmd[3] = index & 0xFF;
    if (extra_len) memcpy(&cmd[4], extra, extra_len);
    
    if (ret = pn532_send_command(pn532, setEmulatorData, cmd, sizeof(cmd))) return ret;
    if (ret = pn532_wait_response(pn532, setEmulatorData)) return ret;

    return 0;
}

int hf15_eset_uid(pn532_t *pn532, uint8_t slot, uint8_t *new_uid) {
    int ret;

    if ((ret = upload_data_block(pn532, 0x03, slot + 0x1A, 0xFE00, new_uid, 8)) == 0)
        ret &= hf15_esave(pn532, slot);

    return ret;
}

int hf15_eset_block(pn532_t *pn532, uint8_t slot, int8_t block_index, uint8_t *block_data) {
    int ret;

    if ((ret = upload_data_block(pn532, 0x03, slot + 0x1A, block_index, block_data, 4)) == 0)
        ret &= hf15_esave(pn532, slot);

    return ret;
}

int hf15_eset_dump(pn532_t *pn532, uint8_t slot, uint8_t *bin_data, size_t bin_len) {
    int ret;
    size_t block_index;
    uint8_t block_data[4];  // Each block is 4 bytes

    for (block_index = 0; block_index < bin_len; block_index += 4) {
        memcpy(block_data, bin_data + block_index, 4);

        // Upload data block
        ret = upload_data_block(pn532, 0x03, slot + 0x1A, block_index / 4, block_data, 4);
        if (ret != 0) {
            printf("Failed to set block %zu: Error %d\n", block_index / 4, ret);
            return ret;
        }

        printf("Set block %02zu %02X%02X%02X%02X\n", block_index / 4, block_data[0], block_data[1], block_data[2], block_data[3]);
    }

    // Save the dump after uploading all blocks
    ret = hf15_esave(pn532, slot);
    if (ret != 0) {
        printf("Failed to save dump. Error %d\n", ret);
    }

    return ret;
}

int hf15_eset_resv_eas_afi_dsfid(pn532_t *pn532, uint8_t slot, uint8_t resv, uint8_t eas, uint8_t afi, uint8_t dsfid) {
    int ret;
    uint8_t data[4];

    data[0] = resv;
    data[1] = eas;
    data[2] = afi;
    data[3] = dsfid;

    if ((ret = upload_data_block(pn532, 0x03, slot + 0x1A, 0xFC00, data, 4)) == 0)
        ret &= hf15_esave(pn532, slot);

    return ret;
}

int hf15_eset_write_protect(pn532_t *pn532, uint8_t slot, uint8_t *data, size_t data_len) {
    int ret;

    if ((ret = upload_data_block(pn532, 0x03, slot + 0x1A, 0xFB00, data, data_len)) == 0)
        ret &= hf15_esave(pn532, slot);

    return ret;
}


int hf15_esave(pn532_t *pn532, uint8_t slot) {
    uint8_t extra[4] = { 0 };

    return upload_data_block(pn532, 0x03, slot + 0x1A, 0xFFFF, extra, sizeof(extra));
}

/* Scan for cards */
int hf15_scan(pn532_t *pn532, hf15_tag_scan *scan) {
    uint8_t cmd[2] = {0x01, 0x05};
    int ret;

    if (ret = pn532_send_command(pn532, InListPassiveTarget, cmd, sizeof(cmd))) return ret;
    if (ret = pn532_wait_response(pn532, InListPassiveTarget)) return ret;

    memcpy(scan, pn532->result.data,  pn532->result.len);
    return pn532->result.status != SUCCESS;
}

/* Get card info */
int hf15_info(pn532_t *pn532, hf15_tag_info *info) {
    uint8_t cmd[2] = {0x02, 0x2B};
    int ret;

    if ((ret = hf15_raw(pn532, cmd, sizeof(cmd), 0, 1, 0))) return ret;
//    if (ret = pn532_read_response(pn532, NULL)) return ret;


    memcpy(info, pn532->result.data,  pn532->result.len);
    return pn532->result.status != HF_TAG_OK || pn532->result.len <= 15;
}


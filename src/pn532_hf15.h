#pragma pack(1)

typedef struct
{
    uint8_t status;
    uint8_t flags;
    uint8_t uid[8];
    uint8_t dsfid;
    uint8_t afi;
    uint8_t pages;
    uint8_t block_size;
    uint8_t ic_reference;
} hf15_tag_info;

typedef struct
{
    uint8_t tagType;
    uint8_t tagNum;
    uint8_t uid[8];
} hf15_tag_scan;

#pragma pack()

int hf15_read_block(pn532_t *pn532, uint8_t block_num, uint8_t *response, uint8_t response_len);
int hf15_write_block(pn532_t *pn532, uint8_t block_num, uint8_t *data, uint8_t len);
int hf15_write_block_verify(pn532_t *pn532, uint8_t block_num, uint8_t *data, uint8_t len);
int hf15_scan(pn532_t *pn532, hf15_tag_scan *scan);
int hf15_info(pn532_t *pn532, hf15_tag_info *info);

int hf15_set_gen1_uid(pn532_t *pn532, uint8_t *uid, uint8_t block_size);
int hf15_set_gen2_uid(pn532_t *pn532, uint8_t *uid);
int hf15_set_gen2_config(pn532_t *pn532, uint8_t size, uint8_t adi, uint8_t dsfid, uint8_t ic_reference);

int hf15_raw(pn532_t *pn532, uint8_t *cmd_data, size_t cmd_len, int select_tag, int append_crc, int no_check_response);

int hf15_eset_uid(pn532_t *pn532, uint8_t slot, uint8_t *new_uid);
int hf15_eset_block(pn532_t *pn532, uint8_t slot, int8_t block_index, uint8_t *block_data);
int hf15_eset_dump(pn532_t *pn532, uint8_t slot, uint8_t *bin_data, size_t bin_len);
int hf15_eset_resv_eas_afi_dsfid(pn532_t *pn532, uint8_t slot, uint8_t resv, uint8_t eas, uint8_t afi, uint8_t dsfid);
int hf15_eset_write_protect(pn532_t *pn532, uint8_t slot, uint8_t *data, size_t data_len);
int hf15_esave(pn532_t *pn532, uint8_t slot);

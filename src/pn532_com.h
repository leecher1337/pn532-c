#include <stdint.h>

enum Command
{
    Diagnose = 0x00,
    GetFirmwareVersion = 0x02,
    GetGeneralStatus = 0x04,
    ReadRegister = 0x06,
    WriteRegister = 0x08,
    ReadGPIO = 0x0C,
    WriteGPIO = 0x0E,
    SetSerialBaudRate = 0x10,
    SetParameters = 0x12,
    SAMConfiguration = 0x14,
    PowerDown = 0x16,
    RFConfiguration = 0x32,
    RFRegulationTest = 0x58,
    InJumpForDEP = 0x56,
    InJumpForPSL = 0x46,
    InListPassiveTarget = 0x4A,
    InATR = 0x50,
    InPSL = 0x4E,
    InDataExchange = 0x40,
    InCommunicateThru = 0x42,
    InDeselect = 0x44,
    InRelease = 0x52,
    InSelect = 0x54,
    InAutoPoll = 0x60,
    TgInitAsTarget = 0x8C,
    TgGetData = 0x86,
    TgSetData = 0x8E
};

enum Pn532KillerCommand
{
    getEmulatorData = 0x1C,
    setEmulatorData = 0x1E,
    checkPn532Killer = 0xAA,
    SetWorkMode = 0xAC,
    GetSnifferLog = 0x20,
    ClearSnifferLog = 0x22
};

enum Status
{
    TimeoutError = -1,
    HF_TAG_OK = 0x00,     // IC card operation is successful
    HF_TAG_NO = 0x01,     // IC card not found
    HF_ERR_STAT = 0x02,   // Abnormal IC card communication
    HF_ERR_CRC = 0x03,    // IC card communication verification abnormal
    HF_COLLISION = 0x04,  // IC card conflict
    HF_ERR_BCC = 0x05,    // IC card BCC error
    MF_ERR_AUTH = 0x06,   // MF card verification failed
    HF_ERR_PARITY = 0x07, // IC card parity error
    HF_ERR_ATS = 0x08,    // ATS should be present but card NAKed, or ATS too large

    // Some operations with low frequency cards succeeded!
    LF_TAG_OK = 0x40,
    // Unable to search for a valid EM410X label
    EM410X_TAG_NO_FOUND = 0x41,

    // The parameters passed by the BLE instruction are wrong, or the parameters passed
    // by calling some functions are wrong
    PAR_ERR = 0x60,
    // The mode of the current device is wrong, and the corresponding API cannot be called
    DEVICE_MODE_ERROR = 0x66,
    INVALID_CMD = 0x67,
    SUCCESS = 0x68,
    NOT_IMPLEMENTED = 0x69,
    FLASH_WRITE_FAIL = 0x70,
    FLASH_READ_FAIL = 0x71,
    INVALID_SLOT_TYPE = 0x72
};

typedef struct
{
    uint8_t cmd;
    enum Status status;
    uint8_t len;
    uint8_t data[255];
} pn532_result_t;

typedef struct {
    int fd;
    pn532_result_t result;
} pn532_t;

int pn532_open(pn532_t *pn532, const char *device);
void pn532_close(pn532_t *pn532);
int pn532_write(pn532_t *pn532, uint8_t *data, size_t len);
int pn532_read(pn532_t *pn532, uint8_t *buffer, size_t len);
int pn532_send_command(pn532_t *pn532, uint8_t cmd, uint8_t *data, size_t data_len);
int pn532_wait_response(pn532_t *pn532, uint8_t cmd);
int pn532_read_response(pn532_t *pn532, pn532_result_t *response);
int pn532_is_pn532killer(pn532_t *pn532);
int pn532_set_normal_mode(pn532_t *pn532);
char *pn532_strerror(int ret);

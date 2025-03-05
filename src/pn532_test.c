#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include "pn532_com.h"
#include "pn532_hf15.h"

#define CHK(x) ({ ret = (x); pn532chk(#x, ret); ret; })

static int pn532chk(char *pszFunc, int ret)
{
    if (ret<0) fprintf (stderr, "%s: %s", pszFunc, pn532_strerror(ret));
    return ret;
}

static int test_scan(pn532_t *pn532)
{
    int ret, i;
    hf15_tag_scan scan;

    if ((ret = CHK(hf15_scan(pn532, &scan))) == 0)
    {
        printf ("Tag found:\nUID:\t");
        for (i=0; i<sizeof(scan.uid); i++) printf ("%02X ", scan.uid[i]);
        printf ("\n");
    }
    return ret;
}

static int test_info(pn532_t *pn532)
{
    int ret, i;
    hf15_tag_info taginfo = {0};

    if ((ret = CHK(hf15_info(pn532, &taginfo))) == 0)
    {
        printf ("Tag found:\nUID:\t");
        for (i=0; i<sizeof(taginfo.uid); i++) printf ("%02X ", taginfo.uid[i]);
        printf ("\n");
    }

    return ret;
}

static int test_read(pn532_t *pn532, uint8_t block_num)
{
    int ret, i;
    uint8_t block[4];

    if ((ret = CHK(hf15_read_block(pn532, block_num, block, sizeof(block)))) == 0)
    {
        printf ("Block %d: ", block_num);
        for (i=0; i<sizeof(block); i++) printf ("%02X ", block[i]);
        printf ("\n");
    }
    else
    {
        printf ("Reading block %d failed: %02X\n", block_num, pn532->result.data[0]);
    }

    return ret;
}

static int test_write(pn532_t *pn532, uint8_t block_num)
{
    int ret, i;
    uint8_t block[4] = {0xDE, 0xAD, 0xBE, 0xEF};

    if ((ret = CHK(hf15_write_block(pn532, block_num, block, sizeof(block)))) == 0)
    {
        printf ("Block %d written\n", block_num);
    }
    else
    {
        printf ("Writing block %d failed: %02X (len: %d)\n", block_num, pn532->result.data[0], pn532->result.len);
    }

    return ret;
}

static int test_write_verify(pn532_t *pn532, uint8_t block_num)
{
    int ret, i;
    uint8_t block[4] = {0xDE, 0xAD, 0xBE, 0xEF};

    if ((ret = CHK(hf15_write_block_verify(pn532, block_num, block, sizeof(block)))) == 0)
    {
        printf ("Block %d written and verified\n", block_num);
    }
    else
    {
        if (ret == 2)
            printf ("Writing block %d succeeded, but verify failed!\n", block_num);
        else
            printf ("Writing block %d failed: %02X (len: %d)\n", block_num, pn532->result.data[0], pn532->result.len);
    }

    return ret;
}

/* Example usage */
int main() {
    pn532_t pn532;
    int ret, i;
    char *pszDev;

    if (pn532_open(&pn532, "/dev/ttyACM0") != 0) {
        perror("Error opening device");
        return -1;
    }

    if ((ret = CHK(pn532_set_normal_mode(&pn532))) == 0)
    {
        usleep(10000);
        if ((ret = CHK(pn532_is_pn532killer(&pn532))) >= 0) {
            pszDev = ret?"PN532Killer":"PN532";
            printf ("Device: %s\n", pszDev);

            printf("Press <RETURN> to scan card..");
        }
        getchar();
        test_scan(&pn532);
        test_info(&pn532);
        test_read(&pn532, 0);
        test_write(&pn532, 0);
        test_write_verify(&pn532, 0);
    }

    pn532_close(&pn532);
    return 0;
}

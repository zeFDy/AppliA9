/******************************************************************************
*
* Copyright 2014 Altera Corporation. All Rights Reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* 3. The name of the author may not be used to endorse or promote products
* derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED. IN NO
* EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
*
******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "alt_sdmmc.h"
#include "alt_globaltmr.h"
#include "socal/hps.h"

/* Determine size of an array */
#define ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))

/* Maximum size of test data */
#define MAX_TEST_BYTES  (1 * 1024 * 1024)

// Card information
ALT_SDMMC_CARD_INFO_t Card_Info;

// SD/MMC Device size - hardcode max supported size for now
uint64_t Sdmmc_Device_Size;

// SD/MMC Block size
uint32_t Sdmmc_Block_Size;

// Buffers used for testing
uint32_t Write_Buffer[MAX_TEST_BYTES / sizeof(uint32_t)] ;
uint32_t Read_Buffer[MAX_TEST_BYTES / sizeof(uint32_t)];

// System initialization
ALT_STATUS_CODE system_init(void);

// System shutdown
ALT_STATUS_CODE system_uninit(void);

/******************************************************************************/
/*!
 * System initialization
 * \return      result of the function
 */
ALT_STATUS_CODE system_init(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    ALT_SDMMC_CARD_MISC_t card_misc_cfg;

    printf("INFO: System Initialization.\n");

    // Initialize global timer
    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: Setting up Global Timer.\n");
        if(!alt_globaltmr_int_is_enabled())
        {
            status = alt_globaltmr_init();
        }
    }

    // Setting up SD/MMC
    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: Setting up SDMMC.\n");
        status = alt_sdmmc_init();
    }

    if(status == ALT_E_SUCCESS)
    {
        status = alt_sdmmc_card_pwr_on();
    }

    if(status == ALT_E_SUCCESS)
    {
        status = alt_sdmmc_card_identify(&Card_Info);
    }

    if(status == ALT_E_SUCCESS)
    {
        switch(Card_Info.card_type)
        {
        case ALT_SDMMC_CARD_TYPE_MMC:
            printf("INFO: MMC Card detected.\n");
            break;
        case ALT_SDMMC_CARD_TYPE_SD:
            printf("INFO: SD Card detected.\n");
            break;
        case ALT_SDMMC_CARD_TYPE_SDIOIO:
            printf("INFO: SDIO Card detected.\n");
            break;
        case ALT_SDMMC_CARD_TYPE_SDIOCOMBO:
            printf("INFO: SDIOCOMBO Card detected.\n");
            break;
        case ALT_SDMMC_CARD_TYPE_SDHC:
            printf("INFO: SDHC Card detected.\n");
            break;
        default:
            printf("INFO: Card type unknown.\n");
            status = ALT_E_ERROR;
            break;
        }
    }

    if(status == ALT_E_SUCCESS)
    {
        status = alt_sdmmc_card_bus_width_set(&Card_Info, ALT_SDMMC_BUS_WIDTH_4);
    }

    if(status == ALT_E_SUCCESS)
    {
        alt_sdmmc_fifo_param_set((ALT_SDMMC_FIFO_NUM_ENTRIES >> 3) - 1,
            (ALT_SDMMC_FIFO_NUM_ENTRIES >> 3), ALT_SDMMC_MULT_TRANS_TXMSIZE1);
    }

    if (status == ALT_E_SUCCESS)
    {
        alt_sdmmc_card_misc_get(&card_misc_cfg);
    }

    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: Card width = %d.\n", card_misc_cfg.card_width);
        printf("INFO: Card block size = %d.\n", (int)card_misc_cfg.block_size);
        Sdmmc_Block_Size = card_misc_cfg.block_size;
        Sdmmc_Device_Size = ((uint64_t)Card_Info.blk_number_high << 32) + Card_Info.blk_number_low;
        Sdmmc_Device_Size *= Card_Info.max_r_blkln;
        printf("INFO: Card size = %lld.\n", Sdmmc_Device_Size);
    }

    if(status == ALT_E_SUCCESS)
    {
        status = alt_sdmmc_dma_enable();
    }

    // Initialize random number generator
    if(status == ALT_E_SUCCESS)
    {
        int random_seed = alt_globaltmr_counter_get_low32();
        printf("INFO: Using random seed = 0x%04x.\n", random_seed);
        srand(random_seed);
    }

    printf("\n");

    return status;
}

/******************************************************************************/
/*!
 * System shutdown
 *
 * \return      result of the function
 */
ALT_STATUS_CODE system_uninit(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    printf("INFO: System shutdown.\n");

    printf("\n");

    return status;
}

/******************************************************************************/
/*!
 * Generate random test data and fill the write buffer
 *
 * \param address [out] SDMMC Address
 * \param size    [out] Size of data
 * \return        result of the function
 */
void generate_test_data(uint32_t * address, uint32_t * size)
{
    // Generate random flash address
    *address = rand() % (Sdmmc_Device_Size - MAX_TEST_BYTES);
    *address &= ~(Sdmmc_Block_Size-1);

    // Generate random size
    *size = rand() % MAX_TEST_BYTES;
    *size &= ~(Sdmmc_Block_Size-1);

    // Initialize write buffer accordingly
    for(int i = 0; i < ARRAY_COUNT(Write_Buffer); i++)
    {
        Write_Buffer[i] = rand();
    }
}

/******************************************************************************/
/*!
 * Demo the SDMMC Block I/O Functions
 * \param address [out] SDMMC Address
 * \param size    [out] Size of data
 * \return        result of the function
 */
ALT_STATUS_CODE sdmmc_demo_blockio(uint32_t address, uint32_t size)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    printf("INFO: Demo block I/O using address = 0x%08x, size = %d bytes.\n", (int)address, (int)size);

    // Write 'write_buffer' to flash
    if(status ==  ALT_E_SUCCESS)
    {
        printf("INFO: Writing to address 0x%08x, size = %d bytes using block I/O.\n", (int)address, (int)size);
        status = alt_sdmmc_write(&Card_Info, (void*)address, Write_Buffer, size);
    }

    //  Read 'read_buffer' from flash
    if(status ==  ALT_E_SUCCESS)
    {
        printf("INFO: Reading from address 0x%08x, size = %d bytes using block I/O.\n", (int)address, (int)size);
        memset(Read_Buffer, 0, size);
        status = alt_sdmmc_read(&Card_Info, Read_Buffer, (void*)address, size);
    }

    // Compare buffers
    if(status ==  ALT_E_SUCCESS)
    {
        printf("INFO: Comparing written data with read back data.\n");
        if(memcmp(Read_Buffer, Write_Buffer, size) != 0)
        {
            status = ALT_E_ERROR;
        }
    }

    // Display result
    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: Demo block I/O succeeded.\n");
    }
    else
    {
        printf("ERROR: Demo block I/O failed.\n");
    }

    printf("\n");

    return status;
}

/******************************************************************************/
/*!
 * Program entrypoint
 * \return        result of the function
 */
int main(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    uint32_t address;
    uint32_t size;

    // System init
    if(status == ALT_E_SUCCESS)
    {
        status = system_init();
    }

    // Block I/O
    if(status == ALT_E_SUCCESS)
    {
        generate_test_data(&address, &size);
        status = sdmmc_demo_blockio(address, size);
    }

    // System teardown
    if(status == ALT_E_SUCCESS)
    {
        status = system_uninit();
    }

    // Report status
    if (status == ALT_E_SUCCESS)
    {
        printf("RESULT: All tests successful.\n");
        return 0;
    }
    else
    {
        printf("RESULT: Some failures detected.\n");
        return 1;
    }
}

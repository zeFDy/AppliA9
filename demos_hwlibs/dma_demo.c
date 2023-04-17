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
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include "alt_dma.h"
#include "alt_globaltmr.h"
#include "socal/hps.h"
#include "socal/socal.h"

// Determine the minimum of two values
#define MIN(a, b) ((a) > (b) ? (b) : (a))

// Determine array size
#define ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))

// Maximum size of test data
#define MAX_TEST_BYTES  (1 * 1024 * 1024)

// Buffers used for testing
uint8_t Write_Buffer[MAX_TEST_BYTES];
uint8_t Read_Buffer[MAX_TEST_BYTES];

// DMA channel to be used
ALT_DMA_CHANNEL_t Dma_Channel;

// System initialization
ALT_STATUS_CODE system_init(void);

// System shutdown
ALT_STATUS_CODE system_uninit(void);

// Generate random test data
void generate_test_data(uint32_t * src_offs, uint32_t * dst_offs, uint32_t * size);

// Demo memory to memory transfer
ALT_STATUS_CODE dma_demo_memory_to_memory(void * src, void * dst, uint32_t size);

// Demo zero to memory transfer
ALT_STATUS_CODE dma_demo_zero_to_memory(void * dst, uint32_t size);

/******************************************************************************/
/*!
 * System Initialization
 * \return      result of the function
 */
ALT_STATUS_CODE system_init(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    printf("INFO: System Initialization.\n");

    // Initialize global timer
    if(status == ALT_E_SUCCESS)
    {
        if(!alt_globaltmr_int_is_enabled())
        {
            status = alt_globaltmr_init();
        }
        // Printf duration is not deterministic, and will induce some element of randomness
        printf("INFO: Setting up Global Timer.\n");
    }


    printf("INFO: Setting up DMA.\n");

    // Uninit DMA
    if(status == ALT_E_SUCCESS)
    {
        status = alt_dma_uninit();
    }

    // Configure everything as defaults.
    if (status == ALT_E_SUCCESS)
    {
        ALT_DMA_CFG_t dma_config;
        dma_config.manager_sec = ALT_DMA_SECURITY_DEFAULT;
        for (int i = 0; i < 8; ++i)
        {
            dma_config.irq_sec[i] = ALT_DMA_SECURITY_DEFAULT;
        }
        for (int i = 0; i < 32; ++i)
        {
            dma_config.periph_sec[i] = ALT_DMA_SECURITY_DEFAULT;
        }
        for (int i = 0; i < 4; ++i)
        {
            dma_config.periph_mux[i] = ALT_DMA_PERIPH_MUX_DEFAULT;
        }

        status = alt_dma_init(&dma_config);
    }

    // Allocate DMA Channel
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Allocating DMA channel.\n");
        status = alt_dma_channel_alloc_any(&Dma_Channel);
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
 * System Cleanup
 *
 * \return      result of the function
 */
ALT_STATUS_CODE system_uninit(void)
{
    printf("INFO: System shutdown.\n");
    printf("\n");
    return ALT_E_SUCCESS;
}

/******************************************************************************/
/*!
 * Generate random test data and fill the write buffer
 *
 * \param src_offs [out] Offset in source buffer
 * \param dst_offs [out] Offset in destination buffer
 * \param size     [out] Size of data
 * \return         result of the function
 */
void generate_test_data(uint32_t * src_offs, uint32_t * dst_offs, uint32_t * size)
{
    // Generate random size
    *size = rand() % MAX_TEST_BYTES;

    // Generate random src address
    *src_offs = rand() % (MAX_TEST_BYTES - *size);

    // Generate random dst address
    *dst_offs = rand() % (MAX_TEST_BYTES - *size);

    // Initialize write buffer with random data
    for(int i = 0; i < ARRAY_COUNT(Write_Buffer); i++)
    {
        Write_Buffer[i] = (uint8_t)rand();
        Read_Buffer[i] = (uint8_t)rand();
    }
}

/******************************************************************************/
/*!
 * Demo memory to memory transfer
 *
 * \param src  [in]  Input buffer
 * \param dst  [out] Output buffer
 * \param size [in]  Data size
 * \return      result of the function
 */
ALT_STATUS_CODE dma_demo_memory_to_memory(void * src, void * dst, uint32_t size)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    ALT_DMA_PROGRAM_t program;

    printf("INFO: Demo DMA memory to memory transfer.\n");

    // Copy source buffer over destination buffer
    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: Copying from 0x%08x to 0x%08x size = %d bytes.\n", (int)src, (int)dst, (int)size);
        status = alt_dma_memory_to_memory(Dma_Channel, &program, dst, src, size, false, (ALT_DMA_EVENT_t)0);
    }

    // Wait for transfer to complete
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Waiting for DMA transfer to complete.\n");
        ALT_DMA_CHANNEL_STATE_t channel_state = ALT_DMA_CHANNEL_STATE_EXECUTING;
        while((status == ALT_E_SUCCESS) && (channel_state != ALT_DMA_CHANNEL_STATE_STOPPED))
        {
            status = alt_dma_channel_state_get(Dma_Channel, &channel_state);
            if(channel_state == ALT_DMA_CHANNEL_STATE_FAULTING)
            {
                ALT_DMA_CHANNEL_FAULT_t fault;
                 alt_dma_channel_fault_status_get(Dma_Channel, &fault);
                 printf("ERROR: DMA CHannel Fault: %d\n", (int)fault);
                 status = ALT_E_ERROR;
            }
        }
    }

    // Compare results
    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: Comparing source and destination buffers.\n");
        if(0  != memcmp(src, dst, size))
        {
            status = ALT_E_ERROR;
        }
    }

    // Display result
    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: Demo DMA memory to memory succeeded.\n");
    }
    else
    {
        printf("ERROR: Demo DMA memory to memory failed.\n");
    }

    printf("\n");

    return status;
}

/******************************************************************************/
/*!
 * Demo zero to memory transfer
 *
 * \param dst  [out] Output buffer
 * \param size [in]  Data size
 * \return      result of the function
 */
ALT_STATUS_CODE dma_demo_zero_to_memory(void * dst, uint32_t size)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    ALT_DMA_PROGRAM_t program;

    printf("INFO: Demo DMA zero to memory transfer.\n");

    // Zero out the destination buffer
    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: Zeroing buffer at 0x%08x, size = %d bytes.\n", (int)dst, (int)size);
        status = alt_dma_zero_to_memory(Dma_Channel, &program, dst,size, false, (ALT_DMA_EVENT_t)0);
    }

    // Wait for transfer to complete
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Waiting for DMA transfer to complete.\n");
        ALT_DMA_CHANNEL_STATE_t channel_state = ALT_DMA_CHANNEL_STATE_EXECUTING;
        while((status == ALT_E_SUCCESS) && (channel_state != ALT_DMA_CHANNEL_STATE_STOPPED))
        {
            status = alt_dma_channel_state_get(Dma_Channel, &channel_state);
            if(channel_state == ALT_DMA_CHANNEL_STATE_FAULTING)
            {
                ALT_DMA_CHANNEL_FAULT_t fault;
                 alt_dma_channel_fault_status_get(Dma_Channel, &fault);
                 printf("ERROR: DMA CHannel Fault: %d\n", (int)fault);
                 status = ALT_E_ERROR;
            }
        }
    }

    // Compare results
    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: Testing destination buffer for zero.\n");
        for(int i=0; i<size; i++)
        {
            if(0 != ((uint8_t*)dst)[i])
            {
                status = ALT_E_ERROR;
            }
        }
    }

    // Display result
    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: Demo DMA zero to memory succeeded.\n");
    }
    else
    {
        printf("ERROR: Demo DMA zero to memory failed.\n");
    }

    printf("\n");

    return status;
}


/******************************************************************************/
/*!
 * Program entrypoint
 *
 * \return result of the program
 */
int main(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    // System init
    if(status == ALT_E_SUCCESS)
    {
        status = system_init();
    }


    // Demo memory to memory DMA transfers
    if(status == ALT_E_SUCCESS)
    {
        uint32_t src_offs;
        uint32_t dst_offs;
        uint32_t size;
        generate_test_data(&src_offs, &dst_offs, &size);
        status = dma_demo_memory_to_memory(&Write_Buffer[src_offs], &Read_Buffer[dst_offs], size);
    }

    // Demo zero to memory DMA transfers
    if(status == ALT_E_SUCCESS)
    {
        uint32_t src_offs;
        uint32_t dst_offs;
        uint32_t size;
        generate_test_data(&src_offs, &dst_offs, &size);
        status = dma_demo_zero_to_memory(&Read_Buffer[dst_offs], size);
    }

    // System uninit
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

    return 0;
}

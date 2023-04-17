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
#include <stdlib.h>
#include <inttypes.h>
#include "alt_cache.h"
#include "alt_clock_manager.h"
#include "alt_ecc.h"
#include "alt_interrupt.h"
#include "alt_mmu.h"
#include "alt_pt.h"

// Determine size of an array
#define ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))

// Initialize interrupt subsystem 
ALT_STATUS_CODE socfpga_int_l2_setup(ALT_INT_INTERRUPT_t int_id, ALT_INT_TRIGGER_t trigger);

// Setup ECC L2 interrupt
ALT_STATUS_CODE l2_int_setup();

// Cleanup specific ECC L2 interrupt 
ALT_STATUS_CODE socfpga_int_l2_cleanup(ALT_INT_INTERRUPT_t int_id);

// Cleanup interrupt subsystem 
ALT_STATUS_CODE l2_int_cleanup();   

// L2 ECC ISR Callback 
void l2_ecc_isr(uint32_t icciar, void * context);     

// Initialize and setup L2
ALT_STATUS_CODE l2_setup(void);

// Setup MMU page table
ALT_STATUS_CODE pt_setup(void);

// L2 ECC Test
ALT_STATUS_CODE l2_ecc_test(void);

// ECC L2 demo
ALT_STATUS_CODE ecc_demo_l2(void);
 
// Scratch RAM for ECC
ALT_STATUS_CODE alt_ecc_scratch_ram_get(void ** pblock, size_t * psize);

// Interrupt counters
static uint32_t serr = 0;
static uint32_t derr = 0;
static uint32_t bytewr = 0;

static uint32_t buffer[1024 * 1024 / sizeof(uint32_t)] __attribute__ ((aligned (ALT_MMU_SMALL_PAGE_SIZE)));

// read value from L2 cache
static uint32_t value = 0;

/******************************************************************************/
/*!
 * Initialize interrupt subsystem 
 *
 * \param       int_id interrupt id
 * \param       trigger type of trigger to be used for the interrupt
 * \return      result of the function
 */
ALT_STATUS_CODE socfpga_int_l2_setup(ALT_INT_INTERRUPT_t int_id, ALT_INT_TRIGGER_t trigger)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    // Set interrupt distributor target
    if (status == ALT_E_SUCCESS)
    {
        int target = 0x3;
        status = alt_int_dist_target_set(int_id, target);
    }

    // Set interrupt trigger type
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_dist_trigger_set(int_id, trigger);
    }

    // Enable interrupt at the distributor level
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_dist_enable(int_id);
    }

    return status;
}

/******************************************************************************/
/*!
 * Setup ECC L2 interrupt
 *
 * \return      result of the function
 */
ALT_STATUS_CODE l2_int_setup()
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
  
    // Setup interrupts for SERR and DERR
    if (status == ALT_E_SUCCESS)
    {
        status = socfpga_int_l2_setup(ALT_INT_INTERRUPT_L2_ECC_CORRECTED_IRQ, ALT_INT_TRIGGER_AUTODETECT);
        status = socfpga_int_l2_setup(ALT_INT_INTERRUPT_L2_ECC_UNCORRECTED_IRQ, ALT_INT_TRIGGER_AUTODETECT);
        status = socfpga_int_l2_setup(ALT_INT_INTERRUPT_L2_ECC_BYTE_WR_IRQ, ALT_INT_TRIGGER_AUTODETECT);
    }

    // Register interrupt handlers for SERR and DERR
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_isr_register(ALT_INT_INTERRUPT_L2_ECC_CORRECTED_IRQ, l2_ecc_isr, &serr);
        status = alt_int_isr_register(ALT_INT_INTERRUPT_L2_ECC_UNCORRECTED_IRQ, l2_ecc_isr, &derr);
        status = alt_int_isr_register(ALT_INT_INTERRUPT_L2_ECC_BYTE_WR_IRQ, l2_ecc_isr, &bytewr);
    }
    
    // Enable CPU interrupts
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_cpu_enable();
    }

    // Enable global interrupts
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_global_enable();
    }

    return status;
}

/******************************************************************************/
/*!
 * Cleanup specific ECC L2 interrupt 
 *
 * \param       int_id interrupt id
 * \return      result of the function
 */
ALT_STATUS_CODE socfpga_int_l2_cleanup(ALT_INT_INTERRUPT_t int_id)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    // Disable ECC L2 interrupt at the distributor level
    if(status == ALT_E_SUCCESS)
    {
        status = alt_int_dist_disable(int_id);
    }
    
    // Unregister ECC L2 ISR
    if(status == ALT_E_SUCCESS)
    {
        status = alt_int_isr_unregister(int_id);
    }
    
    return status;
}

/******************************************************************************/
/*!
 * Cleanup interrupt subsystem 
 *
 * \param       int_id interrupt id
 * \return      result of the function
 */
ALT_STATUS_CODE l2_int_cleanup()
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    
    // Disable global interrupts
    if(status == ALT_E_SUCCESS)
    {
        status = alt_int_global_disable();
    }

    // Disable CPU interrupts
    if(status == ALT_E_SUCCESS)
    {
        status = alt_int_cpu_disable();
    }
    
    // Cleanup interrupts for SERR and DERR
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Cleaning up L2 ECC interrupts.\n");
        status = socfpga_int_l2_cleanup(ALT_INT_INTERRUPT_L2_ECC_CORRECTED_IRQ);
        status = socfpga_int_l2_cleanup(ALT_INT_INTERRUPT_L2_ECC_UNCORRECTED_IRQ);
        status = socfpga_int_l2_cleanup(ALT_INT_INTERRUPT_L2_ECC_BYTE_WR_IRQ);
    }
    
    return status;
}

/******************************************************************************/
/*!
 * L2 ECC ISR Callback 
 *
 * \param       icciar
 * \param       context ISR context.
 * \return      none
 */
void l2_ecc_isr(uint32_t icciar, void * context)
{
    ALT_INT_INTERRUPT_t int_id = (ALT_INT_INTERRUPT_t)ALT_INT_ICCIAR_ACKINTID_GET(icciar);

    volatile uint32_t * count = (volatile uint32_t *)context;

    switch (int_id)
    {
    case ALT_INT_INTERRUPT_L2_ECC_CORRECTED_IRQ:
        alt_ecc_status_clear(ALT_ECC_RAM_L2_DATA, ALT_ECC_ERROR_L2_SERR);
        break;
    case ALT_INT_INTERRUPT_L2_ECC_UNCORRECTED_IRQ:
        alt_ecc_status_clear(ALT_ECC_RAM_L2_DATA, ALT_ECC_ERROR_L2_DERR);
        break;
    case ALT_INT_INTERRUPT_L2_ECC_BYTE_WR_IRQ:
        alt_ecc_status_clear(ALT_ECC_RAM_L2_DATA, ALT_ECC_ERROR_L2_BYTE_WR);
        break;
    default:
        break;
    }
    ++(*count);
    
    if (*count >= 20)
    {
        printf("ISR : Count over 20. Disabling interrupt.\n");
        alt_int_dist_disable(int_id);
    }
}

/******************************************************************************/
/*!
 * Initialize and setup L2
 *
 * \return      result of the function
 */
ALT_STATUS_CODE l2_setup(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Initialize L2 cache ...\n");
        status = alt_cache_l2_init();
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_cache_l2_parity_enable();
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_cache_l2_enable();
    }

    return status;
}

/******************************************************************************/
/*!
 * Setup MMU page table
 *
 * \return      result of the function
 */
ALT_STATUS_CODE pt_setup(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Enable MMU page tables.\n");

        void * block;
        size_t size;
        alt_ecc_scratch_ram_get(&block, &size);

        ALT_PT_SEGMENT_t segments[2];

        if ((uintptr_t)block < (uintptr_t)buffer)
        {
            printf("INFO: Scratch at %p, size = 0x%x.\n", block,  size);
            printf("INFO: Buffer  at %p, size = 0x%x.\n", buffer, sizeof(buffer));

            segments[0] = (ALT_PT_SEGMENT_t)
            {
                .addr = block,
                .size = size
            };
            segments[1] = (ALT_PT_SEGMENT_t)
            {
                .addr = buffer,
                .size = sizeof(buffer)
            };
        }
        else
        {
            printf("INFO: buffer  at %p, size = 0x%x.\n", buffer, sizeof(buffer));
            printf("INFO: scratch at %p, size = 0x%x.\n", block,  size);

            segments[0] = (ALT_PT_SEGMENT_t)
            {
                .addr = buffer,
                .size = sizeof(buffer)
            };
            segments[1] = (ALT_PT_SEGMENT_t)
            {
                .addr = block,
                .size = size
            };
        }

        status = alt_pt_init(segments, 2);
    }

    return status;
}

/******************************************************************************/
/*!
 * L2 ECC Test
 *
 * \return      result of the function
 */
ALT_STATUS_CODE l2_ecc_test(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
 
    uint32_t patterns[] =
    {
        0x33333333,
        0x55555555,
        0x99999999,
    };

    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Cached buffer IO test starts ...\n");
    }

    for (int p = 0; p < ARRAY_COUNT(patterns); ++p)
    {
        size_t pattern = patterns[p];

        if (status == ALT_E_SUCCESS)
        {
            // Sweep cached memory area. Write all the data, then read it all back.
            // Verify they both match and that no SERR or DERR are triggered.

            printf("INFO: Writing to cached buffer ...\n");

            uint32_t * sweep = buffer;

            for (size_t i = 0; i < sizeof(buffer); i += ALT_CACHE_LINE_SIZE)
            {
                uint32_t pattern2 = ((i & 0x1) == 0) ? pattern : ~pattern;

                for (size_t j = 0; j < ALT_CACHE_LINE_SIZE; j += sizeof(uint32_t))
                {
                    *sweep = pattern2;
                    ++sweep;
                }

                if (bytewr | serr | derr)
                {
                    printf("ERROR: ECC ISR for L2 triggered [i = 0x%x].\n", i);
                    status = ALT_E_ERROR;
                    break;
                }
            }
        }

        if (status == ALT_E_SUCCESS)
        {
            printf("INFO: Verifying cached buffer ...\n");

            uint32_t * sweep = buffer;

            for (size_t i = 0; i < sizeof(buffer); i += ALT_CACHE_LINE_SIZE)
            {
                uint32_t pattern2 = ((i & 0x1) == 0) ? pattern : ~pattern;

                for (size_t j = 0; j < ALT_CACHE_LINE_SIZE; j += sizeof(uint32_t))
                {
                    if (*sweep != pattern2)
                    {
                        printf("ERROR: Unexpected buffer value at offset 0x%x.\n", i);
                        status = ALT_E_ERROR;
                        break;
                    }
                    ++sweep;
                }

                if (bytewr | serr | derr)
                {
                    printf("ERROR: ECC ISR for L2 triggered [i = 0x%x].\n", i);
                    status = ALT_E_ERROR;
                    break;
                }
            }
        }
    }

    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Cached buffer IO test pass.\n");
    }

    // Due to technical details of how the L2 RAM is implemented, we need to ensure that
    // a read first occurs on the L2. This way the Write Enable (WE) is set low. Then set
    // the ECC injection, and write to the same spot. This way, the WE will edge high
    // causing the injection logic to procede correctly. If the last IO was a write, the
    // WE does not come down, and the ECC injection is not done. This is an artifact of
    // the ARM L2 controller logic, probably done to save power.

    if (status == ALT_E_SUCCESS)
    {
        // Zero out our cache line.
        register uint32_t * iter = buffer;
        for (register int i = 0; i < ALT_CACHE_LINE_SIZE; i += sizeof(*iter))
        {
            *iter = 0xffffffff;
            ++iter;
        }

        // Now clean and invalidate the data from L1 to L2.
        alt_cache_l1_data_purge(buffer, ALT_CACHE_LINE_SIZE);

        // Read that buffer to ensure that the last IO on that line was a read.
        if (*buffer != 0xffffffff)
        {
            printf("ERROR: See %s:%d.\n", __FILE__, __LINE__);
            status = ALT_E_ERROR;
        }
    }

    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Inject SERR into L2 Data RAM.\n");
        status = alt_ecc_serr_inject(ALT_ECC_RAM_L2_DATA);
    }

    if (status == ALT_E_SUCCESS)
    {
        // Write to buffer and flush to L2.
        register uint32_t * iter = buffer;
        for (register int i = 0; i < ALT_CACHE_LINE_SIZE; i += sizeof(*iter))
        {
            *iter = i;
            ++iter;
        }

        // Purge that line. This way the next time that buffer is read, it will be fetched from L2.
        alt_cache_l1_data_purge(buffer, ALT_CACHE_LINE_SIZE);

        // Read out the data. The actual reading will cause the error to be reported.
        iter = buffer;
        for (int i = 0; i < ALT_CACHE_LINE_SIZE; i += sizeof(*iter))
        {
            value = *iter;
            ++iter;
        }
    }
    
    printf("INFO: SERR detected : %d.\n", (int)serr);

    // Same procedure as SERR but using DERR instead.

    if (status == ALT_E_SUCCESS)
    {
        // Zero out our cache line.
        register uint32_t * iter = buffer;
        for (register int i = 0; i < ALT_CACHE_LINE_SIZE; i += sizeof(*iter))
        {
            *iter = 0xffffffff;
            ++iter;
        }

        // Now clean and invalidate the data from L1 to L2.
        alt_cache_l1_data_purge(buffer, ALT_CACHE_LINE_SIZE);

        // Read that buffer to ensure that the last IO on that line was a read.
        if (*buffer != 0xffffffff)
        {
            printf("ERROR: See %s:%d.\n", __FILE__, __LINE__);
            status = ALT_E_ERROR;
        }
    }

    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Inject DERR into L2 Data RAM.\n");
        status = alt_ecc_derr_inject(ALT_ECC_RAM_L2_DATA);
    }

    if (status == ALT_E_SUCCESS)
    {
        // Write to buffer and flush to L2.
        register uint32_t * iter = buffer;
        for (register int i = 0; i < ALT_CACHE_LINE_SIZE; i += sizeof(*iter))
        {
            *iter = i;
            ++iter;
        }

        // Purge that line. This way the next time that buffer is read, it will be fetched from L2.
        alt_cache_l1_data_purge(buffer, ALT_CACHE_LINE_SIZE);

        // Read out the data. The actual reading will cause the error to be reported.
        iter = buffer;
        for (int i = 0; i < ALT_CACHE_LINE_SIZE; i += sizeof(*iter))
        {
            value = *iter;
            ++iter;
        }
    }
    
    printf("INFO: DERR detected : %d.\n", (int)derr);

    return status;
}

/******************************************************************************/
/*!
 * Demonstrates injecting single bit error (SERR) and double bit error (DERR)
 * into L2 cache
 *
 * \return      result of the function
 */
ALT_STATUS_CODE ecc_demo_l2(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

#if 0 // The HWLIBs APIs for MPU frequencies need to be updated
    if (status == ALT_E_SUCCESS)
    {
        alt_freq_t freq;
        status = alt_clk_freq_get(ALT_CLK_MPU, &freq);
        printf("INFO: MPU frequency at %d MHz.\n", (int)(freq / 1000000));
    }
#endif

    // Setup MMU page table
    if (status == ALT_E_SUCCESS)
    {
       status = pt_setup();
    }

    // Enable L1 caches
    if (status == ALT_E_SUCCESS)
    {
        status = alt_cache_l1_enable_all();
    }

    // Initialize and setup L2
    if (status == ALT_E_SUCCESS)
    {
       status = l2_setup();
    }

    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Initialize and start ECC for L2.\n");
        status = alt_ecc_start(ALT_ECC_RAM_L2_DATA);
    }
    
    // Setup L2 ECC interrupts
    if (status == ALT_E_SUCCESS)
    {
       status = l2_int_setup();
    }

    if (status == ALT_E_SUCCESS)
    {
        status = l2_ecc_test();
    }
    
    // Cleanup L2 ECC interrupts
    if (status == ALT_E_SUCCESS)
    {
        status = l2_int_cleanup();
    }

    alt_ecc_stop(ALT_ECC_RAM_L2_DATA);

    // Cleanup L2 cache
    if (status == ALT_E_SUCCESS)
    {
        alt_cache_l2_disable();
        alt_cache_l2_parity_disable();
        alt_cache_l2_uninit();
    }
  
    // Disable L1 caches
    if (status == ALT_E_SUCCESS)
    {
        alt_cache_l1_data_disable();
    }
    
    // Disable MMU page table
    if (status == ALT_E_SUCCESS)
    {
        alt_pt_uninit();
    }

    // Display status
    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: ECC L2 Demo succeeded.\n\n");
    }
    else
    {
        printf("INFO: ECC L2 Demo failed.\n\n");
    }
    
    return status;
}

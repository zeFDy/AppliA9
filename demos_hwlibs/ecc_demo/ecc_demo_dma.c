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
#include "alt_ecc.h"
#include "alt_interrupt.h"
#include "alt_dma.h"
#include "alt_dma_program.h"
#include "alt_dma_common.h"

// Initialize interrupt subsystem 
ALT_STATUS_CODE socfpga_int_dma_setup(ALT_INT_INTERRUPT_t int_id, ALT_INT_TRIGGER_t trigger);

// Setup ECC DMA interrupt
ALT_STATUS_CODE dma_int_setup();

// Cleanup specific ECC DMA interrupt 
ALT_STATUS_CODE socfpga_int_dma_cleanup(ALT_INT_INTERRUPT_t int_id);

// Cleanup interrupt subsystem 
ALT_STATUS_CODE dma_int_cleanup();

// DMA ECC ISR callback
void dma_ecc_isr(uint32_t icciar, void * context);

// DMA ECC test
ALT_STATUS_CODE dma_ecc_test(void);

// DMA Data transfer
ALT_STATUS_CODE alt_dma_mfifo_ecc_test_helper(ALT_DMA_CHANNEL_t channel,
                                              ALT_DMA_PROGRAM_t * program,
                                              const void * src,
                                              void * dst);

// DMA Channel wait
ALT_STATUS_CODE alt_dma_channel_wait_for_state(ALT_DMA_CHANNEL_t channel,
                                               ALT_DMA_CHANNEL_STATE_t state,
                                               uint32_t count);

// ECC DMA demo
ALT_STATUS_CODE ecc_demo_dma(void);

#define DMA_WAIT_COUNT  10000

// Interrupt counters
static uint32_t serr = 0;
static uint32_t derr = 0;

static ALT_DMA_CHANNEL_t channel;
static ALT_DMA_PROGRAM_t program;

char src_buf[512 * 8] __attribute__ ((aligned (64)));
char dst_buf[512 * 8] __attribute__ ((aligned (64)));

/******************************************************************************/
/*!
 * Initialize interrupt subsystem 
 *
 * \param       int_id interrupt id
 * \param       trigger type of trigger to be used for the interrupt
 * \return      result of the function
 */
ALT_STATUS_CODE socfpga_int_dma_setup(ALT_INT_INTERRUPT_t int_id, ALT_INT_TRIGGER_t trigger)
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
 * Setup ECC DMA interrupt
 *
 * \return      result of the function
 */
ALT_STATUS_CODE dma_int_setup()
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    
    // Setup interrupts for SERR and DERR
    if (status == ALT_E_SUCCESS)
    {
        status = socfpga_int_dma_setup(ALT_INT_INTERRUPT_DMA_ECC_CORRECTED_IRQ, ALT_INT_TRIGGER_AUTODETECT);
        status = socfpga_int_dma_setup(ALT_INT_INTERRUPT_DMA_ECC_UNCORRECTED_IRQ, ALT_INT_TRIGGER_AUTODETECT);
    }

    // Register interrupt handlers for SERR and DERR
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_isr_register(ALT_INT_INTERRUPT_DMA_ECC_CORRECTED_IRQ, dma_ecc_isr, &serr);
        status = alt_int_isr_register(ALT_INT_INTERRUPT_DMA_ECC_UNCORRECTED_IRQ, dma_ecc_isr, &derr);
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
 * Cleanup specific ECC DMA interrupt 
 *
 * \param       int_id interrupt id
 * \return      result of the function
 */
ALT_STATUS_CODE socfpga_int_dma_cleanup(ALT_INT_INTERRUPT_t int_id)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    // Disable ECC DMA interrupt at the distributor level
    if(status == ALT_E_SUCCESS)
    {
        status = alt_int_dist_disable(int_id);
    }
    
    // Unregister ECC DMA ISR
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
ALT_STATUS_CODE dma_int_cleanup()
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
        printf("INFO: Cleaning up DMA ECC interrupts.\n");
        status = socfpga_int_dma_cleanup(ALT_INT_INTERRUPT_DMA_ECC_CORRECTED_IRQ);
        status = socfpga_int_dma_cleanup(ALT_INT_INTERRUPT_DMA_ECC_UNCORRECTED_IRQ);
    }
    
    return status;
}

/******************************************************************************/
/*!
 * DMA ECC ISR Callback 
 *
 * \param       icciar
 * \param       context ISR context.
 * \return      none
 */
void dma_ecc_isr(uint32_t icciar, void * context)
{
    ALT_INT_INTERRUPT_t int_id = (ALT_INT_INTERRUPT_t)ALT_INT_ICCIAR_ACKINTID_GET(icciar);

    volatile uint32_t * count = (volatile uint32_t *)context;

    switch (int_id)
    {
    case ALT_INT_INTERRUPT_DMA_ECC_CORRECTED_IRQ:
        alt_ecc_status_clear(ALT_ECC_RAM_DMA, ALT_ECC_ERROR_DMA_SERR);
        break;
    case ALT_INT_INTERRUPT_DMA_ECC_UNCORRECTED_IRQ:
        alt_ecc_status_clear(ALT_ECC_RAM_DMA, ALT_ECC_ERROR_DMA_DERR);
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
 * Initialize and setup DMA
 *
 * \return      result of the function
 */
ALT_STATUS_CODE dma_setup(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    ALT_DMA_CFG_t dma_config;

    // Initializing DMA
    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: Initializing DMA.\n");
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
    
    // Allocate DMA channel
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Allocate DMA channel ...\n");
        status = alt_dma_channel_alloc_any(&channel);
    }

    return status;
}

/******************************************************************************/
/*!
 * DMA ECC Test
 *
 * \return      result of the function
 */
ALT_STATUS_CODE dma_ecc_test(void)
{

    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Inject SERR into DMA MFIFO.\n");
        status = alt_ecc_serr_inject(ALT_ECC_RAM_DMA);
    }
 
    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_mfifo_ecc_test_helper(channel, &program, src_buf, dst_buf);
    }
    
    printf("INFO: SERR detected : %d.\n", (int)serr);
    
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Inject DERR into DMA MFIFO.\n");
        status = alt_ecc_derr_inject(ALT_ECC_RAM_DMA);
    }
 
    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_mfifo_ecc_test_helper(channel, &program, src_buf, dst_buf);
    }

    printf("INFO: DERR detected : %d.\n", (int)derr);

    return status;
}

/******************************************************************************/
/*!
 * DMA data transfer
 *
 * \param       DMA channel
 * \param       DMA program
 * \param       source 
 * \param       destination
 * \return      result of the function
 */
//
// ECC is only generated when the chunk of memory with an ECC error is read.
// As such, this function first reads a large buffer filling up the MFIFO
// completely.
//
ALT_STATUS_CODE alt_dma_mfifo_ecc_test_helper(ALT_DMA_CHANNEL_t channel,
                                              ALT_DMA_PROGRAM_t * program,
                                              const void * src,
                                              void * dst)
{
    if ((uintptr_t)src & (sizeof(uint64_t) - 1))
    {
        return ALT_E_BAD_ARG;
    }

    if ((uintptr_t)dst & (sizeof(uint64_t) - 1))
    {
        return ALT_E_BAD_ARG;
    }

    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_init(program);
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_SAR, (uintptr_t)src);
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_DAR, (uintptr_t)dst);
    }

    // Program in the following parameters:
    //  - SAI  (src address incrementing)
    //  - SS64 (src burst size of 8-byte)
    //  - SB16 (src burst length of 16 transfers)
    //  - DAI  (dst address incrementing)
    //  - DS64 (dst burst size of 8-byte)
    //  - DB16 (dst burst length of 16 transfers)
    //  - All other options default.

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                        (   ALT_DMA_CCR_OPT_SAI
                                          | ALT_DMA_CCR_OPT_SS64
                                          | ALT_DMA_CCR_OPT_SB16
                                          | ALT_DMA_CCR_OPT_SP_DEFAULT
                                          | ALT_DMA_CCR_OPT_SC_DEFAULT
                                          | ALT_DMA_CCR_OPT_DAI
                                          | ALT_DMA_CCR_OPT_DS64
                                          | ALT_DMA_CCR_OPT_DB16
                                          | ALT_DMA_CCR_OPT_DP_DEFAULT
                                          | ALT_DMA_CCR_OPT_DC_DEFAULT
                                          | ALT_DMA_CCR_OPT_ES_DEFAULT
                                        )
            );
    }

    // Load data to fill MFIFO. The MFIF0 is 512 entries. Each entry is 64-bits. Each load
    // bursts 16 items of 64-bits. Thus we need to do 512 / 16 = 32 loads

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMALP(program, 32);
    }

    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Write to DMA MFIFO.\n");
        status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
    }

    // Store all the data out of the MFIFO.

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMALP(program, 32);
    }

    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Read from DMA MFIFO.\n");
        status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
    }

    // End Program

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAEND(program);
    }

    // Execute program on the given channel.

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_channel_exec(channel, program);
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_channel_wait_for_state(channel, ALT_DMA_CHANNEL_STATE_STOPPED, DMA_WAIT_COUNT);
    }

    return status;
}

/******************************************************************************/
/*!
 * DMA Channel wait
 *
 * \param       DMA channel
 * \param       DMA state
 * \param       count
 * \return      result of the function
 */
ALT_STATUS_CODE alt_dma_channel_wait_for_state(ALT_DMA_CHANNEL_t channel,
                                               ALT_DMA_CHANNEL_STATE_t state,
                                               uint32_t count)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    ALT_DMA_CHANNEL_STATE_t current;

    uint32_t i = count;
    while (--i)
    {
        status = alt_dma_channel_state_get(channel, &current);
        if (status != ALT_E_SUCCESS)
        {
            break;
        }
        if (current == state)
        {
            break;
        }
    }

    if (i == 0)
    {
        printf("ERROR: DMA wait timeout");
        status = ALT_E_TMO;
    }

    return status;
}

/******************************************************************************/
/*!
 * Demonstrates injecting single bit error (SERR) and double bit error (DERR)
 * into DMA MFIFO
 *
 * \return      result of the function
 */
ALT_STATUS_CODE ecc_demo_dma()
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    printf("INFO: ECC DMA Demo started.\n");
    
    // Initialize and setup DMA
    if (status == ALT_E_SUCCESS)
    {
       status = dma_setup();
    }
    
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Initialize and start ECC for DMA.\n");
        status = alt_ecc_start(ALT_ECC_RAM_DMA);
    }

    // Setup DMA ECC interrupts
    if (status == ALT_E_SUCCESS)
    {
       status = dma_int_setup();
    }

     // Clear any spurious ECC interrupts
    if (status == ALT_E_SUCCESS)
    {
        status = alt_ecc_status_clear(ALT_ECC_RAM_DMA, ALT_ECC_ERROR_DMA_SERR);
        status = alt_ecc_status_clear(ALT_ECC_RAM_DMA, ALT_ECC_ERROR_DMA_DERR);
    }
 
    if (status == ALT_E_SUCCESS)
    {
        status = dma_ecc_test();
    }

    // Cleanup DMA ECC interrupts
    if (status == ALT_E_SUCCESS)
    {
        status = dma_int_cleanup();
    }
    
    if (status == ALT_E_SUCCESS)
    {
        status = alt_ecc_stop(ALT_ECC_RAM_DMA);
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_uninit();
    }

    // Display status
    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: ECC DMA Demo succeeded.\n\n");
    }
    else
    {
        printf("INFO: ECC DMA Demo failed.\n\n");
    }

    return status;
}

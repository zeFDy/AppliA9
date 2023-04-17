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
#include "alt_sdmmc.h"

// Initialize interrupt subsystem 
ALT_STATUS_CODE socfpga_int_sdmmc_setup(ALT_INT_INTERRUPT_t int_id, ALT_INT_TRIGGER_t trigger);

// Setup ECC SDMMC interrupt
ALT_STATUS_CODE sdmmc_int_setup();

// Cleanup specific ECC SDMMC interrupt 
ALT_STATUS_CODE socfpga_int_sdmmc_cleanup(ALT_INT_INTERRUPT_t int_id);

// Cleanup interrupt subsystem 
ALT_STATUS_CODE sdmmc_int_cleanup();

// SDMMC ECC ISR Callback 
void sdmmc_ecc_isr(uint32_t icciar, void * context);

// SDMMC ECC Test
ALT_STATUS_CODE sdmmc_ecc_test(void);

// ECC SDMMC Demo
ALT_STATUS_CODE ecc_demo_sdmmc(void);

// Interrupt counters
static uint32_t serr = 0;
static uint32_t derr = 0;

static uint32_t buffer[ALT_SDMMC_FIFO_NUM_ENTRIES];

/******************************************************************************/
/*!
 * Initialize interrupt subsystem
 *
 * \param       int_id interrupt id
 * \param       trigger type of trigger to be used for the interrupt
 * \return      result of the function
 */
ALT_STATUS_CODE socfpga_int_sdmmc_setup(ALT_INT_INTERRUPT_t int_id, ALT_INT_TRIGGER_t trigger)
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
 * Setup ECC SDMMC interrupt
 *
 * \return      result of the function
 */
ALT_STATUS_CODE sdmmc_int_setup()
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    
    // Setup interrupts for SERR and DERR
    if (status == ALT_E_SUCCESS)
    {
        status = socfpga_int_sdmmc_setup(ALT_INT_INTERRUPT_SDMMC_PORTA_ECC_CORRECTED, ALT_INT_TRIGGER_AUTODETECT);
        status = socfpga_int_sdmmc_setup(ALT_INT_INTERRUPT_SDMMC_PORTA_ECC_UNCORRECTED, ALT_INT_TRIGGER_AUTODETECT);
        status = socfpga_int_sdmmc_setup(ALT_INT_INTERRUPT_SDMMC_PORTB_ECC_CORRECTED, ALT_INT_TRIGGER_AUTODETECT);
        status = socfpga_int_sdmmc_setup(ALT_INT_INTERRUPT_SDMMC_PORTB_ECC_UNCORRECTED, ALT_INT_TRIGGER_AUTODETECT);
    }

    // Register interrupt handlers for SERR and DERR
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_isr_register(ALT_INT_INTERRUPT_SDMMC_PORTA_ECC_CORRECTED, sdmmc_ecc_isr, &serr);
        status = alt_int_isr_register(ALT_INT_INTERRUPT_SDMMC_PORTA_ECC_UNCORRECTED, sdmmc_ecc_isr, &derr);
        status = alt_int_isr_register(ALT_INT_INTERRUPT_SDMMC_PORTB_ECC_CORRECTED, sdmmc_ecc_isr, &serr);
        status = alt_int_isr_register(ALT_INT_INTERRUPT_SDMMC_PORTB_ECC_UNCORRECTED, sdmmc_ecc_isr, &derr);
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
 * Cleanup specific ECC SDMMC interrupt 
 *
 * \param       int_id interrupt id
 * \return      result of the function
 */
ALT_STATUS_CODE socfpga_int_sdmmc_cleanup(ALT_INT_INTERRUPT_t int_id)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    // Disable ECC SDMMC interrupt at the distributor level
    if(status == ALT_E_SUCCESS)
    {
        status = alt_int_dist_disable(int_id);
    }
    
    // Unregister ECC SDMMC ISR
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
ALT_STATUS_CODE sdmmc_int_cleanup()
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
        printf("INFO: Cleaning up ECC SDMMC interrupts.\n");
        status = socfpga_int_sdmmc_cleanup(ALT_INT_INTERRUPT_SDMMC_PORTA_ECC_CORRECTED);
        status = socfpga_int_sdmmc_cleanup(ALT_INT_INTERRUPT_SDMMC_PORTA_ECC_UNCORRECTED);
        status = socfpga_int_sdmmc_cleanup(ALT_INT_INTERRUPT_SDMMC_PORTB_ECC_CORRECTED);
        status = socfpga_int_sdmmc_cleanup(ALT_INT_INTERRUPT_SDMMC_PORTB_ECC_UNCORRECTED);
    }
    
    return status;
}

/******************************************************************************/
/*!
 * SDMMC ECC ISR Callback 
 *
 * \param       icciar
 * \param       context ISR context.
 * \return      none
 */
void sdmmc_ecc_isr(uint32_t icciar, void * context)
{
    ALT_INT_INTERRUPT_t int_id = (ALT_INT_INTERRUPT_t)ALT_INT_ICCIAR_ACKINTID_GET(icciar);

    volatile uint32_t * count = (volatile uint32_t *)context;

    switch (int_id)
    {
    case ALT_INT_INTERRUPT_SDMMC_PORTA_ECC_CORRECTED:
        alt_ecc_status_clear(ALT_ECC_RAM_SDMMC, ALT_ECC_ERROR_SDMMC_PORT_A_SERR);
        break;
    case ALT_INT_INTERRUPT_SDMMC_PORTA_ECC_UNCORRECTED:
        alt_ecc_status_clear(ALT_ECC_RAM_SDMMC, ALT_ECC_ERROR_SDMMC_PORT_A_DERR);
        break;
    case ALT_INT_INTERRUPT_SDMMC_PORTB_ECC_CORRECTED:
        alt_ecc_status_clear(ALT_ECC_RAM_SDMMC, ALT_ECC_ERROR_SDMMC_PORT_B_SERR);
        break;
    case ALT_INT_INTERRUPT_SDMMC_PORTB_ECC_UNCORRECTED:
        alt_ecc_status_clear(ALT_ECC_RAM_SDMMC, ALT_ECC_ERROR_SDMMC_PORT_B_DERR);
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
 * Initialize and setup SDMMC
 *
 * \return      result of the function
 */
ALT_STATUS_CODE sdmmc_setup(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    // Initializing SDMMC
    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: Initializing SDMMC...\n");
        status = alt_sdmmc_init();
    }
    
    // Reset SDMMC FIFO
    if(status == ALT_E_SUCCESS)
    {
        alt_sdmmc_fifo_reset();
    }

    // Clearing SDMMC FIFO
    for (uint32_t i; i<ALT_SDMMC_FIFO_NUM_ENTRIES; i++){
        buffer[i]=0x00;
    }

    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Clearing SDMMC FIFO...\n");
        status = alt_sdmmc_fifo_write(buffer, ALT_SDMMC_FIFO_NUM_ENTRIES);
    }
    
    return status;
}

/******************************************************************************/
/*!
 * SDMMC ECC Test
 *
 * \return      result of the function
 */
ALT_STATUS_CODE sdmmc_ecc_test(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Inject SERR into SDMMC FIFO.\n");
        status = alt_ecc_serr_inject(ALT_ECC_RAM_SDMMC);
    }
    
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Write to SDMMC FIFO.\n");
        status = alt_sdmmc_fifo_write(buffer, ALT_SDMMC_FIFO_NUM_ENTRIES);
    }

    if (status == ALT_E_SUCCESS)
        {
            printf("INFO: Read from SDMMC FIFO.\n");
            status = alt_sdmmc_fifo_read(buffer, ALT_SDMMC_FIFO_NUM_ENTRIES);
        }

    printf("INFO: SERR detected : %d.\n", (int)serr);

    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Inject DERR into SDMMC FIFO.\n");
        status = alt_ecc_derr_inject(ALT_ECC_RAM_SDMMC);
    }
    
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Write to SDMMC FIFO.\n");
        status = alt_sdmmc_fifo_write(buffer, ALT_SDMMC_FIFO_NUM_ENTRIES);
    }

    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Read SDMMC FIFO.\n");
        status = alt_sdmmc_fifo_read(buffer, ALT_SDMMC_FIFO_NUM_ENTRIES);
    }

    printf("INFO: DERR detected : %d.\n", (int)derr);

    return status;
}

/******************************************************************************/
/*!
 * Demonstrates injecting single bit error (SERR) and double bit error (DERR)
 * into SDMMC FIFO
 *
 * \return      result of the function
 */
ALT_STATUS_CODE ecc_demo_sdmmc()
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    printf("INFO: ECC SDMMC Demo started.\n");
    
    // Initialize and setup SDMMC
    if (status == ALT_E_SUCCESS)
    {
       status = sdmmc_setup();
    }
    
    // Setup SDMMC ECC interrupts
    if (status == ALT_E_SUCCESS)
    {
       status = sdmmc_int_setup();
    }
    
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Initialize and start ECC for SDMMC.\n");
        status = alt_ecc_start(ALT_ECC_RAM_SDMMC);
    }
    
    // Clear any spurious ECC interrupts
    if (status == ALT_E_SUCCESS)
    {
        status = alt_ecc_status_clear(ALT_ECC_RAM_SDMMC, ALT_ECC_ERROR_SDMMC_PORT_A_SERR);
        status = alt_ecc_status_clear(ALT_ECC_RAM_SDMMC, ALT_ECC_ERROR_SDMMC_PORT_A_DERR);
        status = alt_ecc_status_clear(ALT_ECC_RAM_SDMMC, ALT_ECC_ERROR_SDMMC_PORT_B_SERR);
        status = alt_ecc_status_clear(ALT_ECC_RAM_SDMMC, ALT_ECC_ERROR_SDMMC_PORT_B_DERR);
    }
    
    // Test SDMMC ECC
    if (status == ALT_E_SUCCESS)
    {
        status = sdmmc_ecc_test();
    }

    // Cleanup SDMMC ECC interrupts
    if (status == ALT_E_SUCCESS)
    {
        status = sdmmc_int_cleanup();
    }
 
    // Stop SDMMC ECC
    if (status == ALT_E_SUCCESS)
    {
        status = alt_ecc_stop(ALT_ECC_RAM_SDMMC);
    }

    // Uninitialize SDMMC
    if (status == ALT_E_SUCCESS)
    {
        status = alt_sdmmc_uninit();
    }

    // Display status
    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: ECC SDMMC Demo succeeded.\n\n");
    }
    else
    {
        printf("INFO: ECC SDMMC Demo failed.\n\n");
    }

    return status;
}

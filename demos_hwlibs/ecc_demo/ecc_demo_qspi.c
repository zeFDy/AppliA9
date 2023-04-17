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
#include "alt_qspi.h"

// Initialize interrupt subsystem 
ALT_STATUS_CODE socfpga_int_qspi_setup(ALT_INT_INTERRUPT_t int_id, ALT_INT_TRIGGER_t trigger);

// Setup ECC QSPI interrupt
ALT_STATUS_CODE qspi_int_setup();

// Cleanup specific ECC QSPI interrupt 
ALT_STATUS_CODE socfpga_int_qspi_cleanup(ALT_INT_INTERRUPT_t int_id);

// Cleanup  interrupt subsystem
ALT_STATUS_CODE qspi_int_cleanup();

// ECC QSPI ISR Callback
void qspi_ecc_isr(uint32_t icciar, void * context);

// Initialize and setup QSPI
ALT_STATUS_CODE qspi_setup(void);

// QSPI ECC Test
ALT_STATUS_CODE qspi_ecc_test(void);

// ECC QSPI Demo
ALT_STATUS_CODE ecc_demo_qspi(void);

// Interrupt counters
static uint32_t serr = 0;
static uint32_t derr = 0;

#define TEST_SIZE 256
static uint32_t buffer[TEST_SIZE]  __attribute__ ((aligned (64)));

/******************************************************************************/
/*!
 * Initialize interrupt subsystem 
 *
 * \param       int_id interrupt id
 * \param       trigger type of trigger to be used for the interrupt
 * \return      result of the function
 */
ALT_STATUS_CODE socfpga_int_qspi_setup(ALT_INT_INTERRUPT_t int_id, ALT_INT_TRIGGER_t trigger)
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
 * Setup ECC QSPI interrupt
 *
 * \return      result of the function
 */
ALT_STATUS_CODE qspi_int_setup()
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    
    // Setup interrupts for SERR and DERR
    if (status == ALT_E_SUCCESS)
    {
        status = socfpga_int_qspi_setup(ALT_INT_INTERRUPT_QSPI_ECC_CORRECTED_IRQ, ALT_INT_TRIGGER_AUTODETECT);
        status = socfpga_int_qspi_setup(ALT_INT_INTERRUPT_QSPI_ECC_UNCORRECTED_IRQ, ALT_INT_TRIGGER_AUTODETECT);
    }

    // Register interrupt handlers for SERR and DERR
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_isr_register(ALT_INT_INTERRUPT_QSPI_ECC_CORRECTED_IRQ, qspi_ecc_isr, &serr);
        status = alt_int_isr_register(ALT_INT_INTERRUPT_QSPI_ECC_UNCORRECTED_IRQ, qspi_ecc_isr, &derr);
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
 * Cleanup specific ECC QSPI interrupt 
 *
 * \param       int_id interrupt id
 * \return      result of the function
 */
ALT_STATUS_CODE socfpga_int_qspi_cleanup(ALT_INT_INTERRUPT_t int_id)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    // Disable ECC QSPI interrupt at the distributor level
    if(status == ALT_E_SUCCESS)
    {
        status = alt_int_dist_disable(int_id);
    }
    
    // Unregister ECC QSPI ISR
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
ALT_STATUS_CODE qspi_int_cleanup()
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
        printf("INFO: Cleaning up QSPI ECC interrupts.\n");
        status = socfpga_int_qspi_cleanup(ALT_INT_INTERRUPT_QSPI_ECC_CORRECTED_IRQ);
        status = socfpga_int_qspi_cleanup(ALT_INT_INTERRUPT_QSPI_ECC_UNCORRECTED_IRQ);
    }
    
    return status;
}

/******************************************************************************/
/*!
 * QSPI ECC ISR Callback 
 *
 * \param       icciar
 * \param       context ISR context.
 * \return      none
 */
void qspi_ecc_isr(uint32_t icciar, void * context)
{
    ALT_INT_INTERRUPT_t int_id = (ALT_INT_INTERRUPT_t)ALT_INT_ICCIAR_ACKINTID_GET(icciar);

    volatile uint32_t * count = (volatile uint32_t *)context;

    switch (int_id)
    {
    case ALT_INT_INTERRUPT_QSPI_ECC_CORRECTED_IRQ:
        alt_ecc_status_clear(ALT_ECC_RAM_QSPI, ALT_ECC_ERROR_QSPI_SERR);
        break;
    case ALT_INT_INTERRUPT_QSPI_ECC_UNCORRECTED_IRQ:
        alt_ecc_status_clear(ALT_ECC_RAM_QSPI, ALT_ECC_ERROR_QSPI_DERR);
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
 * Initialize and setup QSPI
 *
 * \return      result of the function
 */
ALT_STATUS_CODE qspi_setup(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    // Initializing QSPI
    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: Initializing QSPI.\n");
        status = alt_qspi_init();
    }

    // Enabling QSPI
    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: Enabling QSPI.\n");
        status = alt_qspi_enable();
    }
    
    // Checking QSPI is idle
    if(status == ALT_E_SUCCESS)
    {
        if (!alt_qspi_is_idle())
        {
            status = ALT_E_ERROR;
        }
    }
    
    
    return status;
}

/******************************************************************************/
/*!
 * QSPI ECC Test
 *
 * \return      result of the function
 */
ALT_STATUS_CODE qspi_ecc_test(void)
{

    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    // pick up a random flash address
    uint32_t address = rand() & (alt_qspi_get_device_size() - 1);

    // make it aligned to test size
    address &= ~(TEST_SIZE - 1);

    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Inject SERR into QSPI SRAM.\n");
        status = alt_ecc_serr_inject(ALT_ECC_RAM_QSPI);
    }
 
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Write to subsector 0x%08x.\n", (int)address);
        status = alt_qspi_write(address, buffer, TEST_SIZE);
    }

    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Read subsector 0x%08x.\n", (int)address);
        status = alt_qspi_read(buffer, address, TEST_SIZE);
    }
    
    printf("INFO: SERR detected : %d.\n", (int)serr);
    
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Inject DERR into QSPI SRAM.\n");
        status = alt_ecc_derr_inject(ALT_ECC_RAM_QSPI);
    }
 
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Write to subsector 0x%08x.\n", (int)address);
        status = alt_qspi_write(address, buffer, TEST_SIZE);
    }

    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Read subsector 0x%08x.\n", (int)address);
        status = alt_qspi_read(buffer, address, TEST_SIZE);
    }

    printf("INFO: DERR detected : %d.\n", (int)derr);

    return status;
}

/******************************************************************************/
/*!
 * Demonstrates injecting single bit error (SERR) and double bit error (DERR)
 * into QSPI
 *
 * \return      result of the function
 */
ALT_STATUS_CODE ecc_demo_qspi()
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    printf("INFO: ECC QSPI Demo started.\n");
    
    // Initialize and setup QSPI
    if (status == ALT_E_SUCCESS)
    {
       status = qspi_setup();
    }
    
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Initialize and start ECC for QSPI.\n");
        status = alt_ecc_start(ALT_ECC_RAM_QSPI);
    }

    // Setup QSPI ECC interrupts
    if (status == ALT_E_SUCCESS)
    {
       status = qspi_int_setup();
    }

     // Clear any spurious ECC interrupts
    if (status == ALT_E_SUCCESS)
    {
        status = alt_ecc_status_clear(ALT_ECC_RAM_QSPI, ALT_ECC_ERROR_QSPI_SERR);
        status = alt_ecc_status_clear(ALT_ECC_RAM_QSPI, ALT_ECC_ERROR_QSPI_DERR);
    }
 
    // Test QSPI ECC 
    if (status == ALT_E_SUCCESS)
    {
        status = qspi_ecc_test();
    }

    // Cleanup QSPI ECC interrupts
    if (status == ALT_E_SUCCESS)
    {
        status = qspi_int_cleanup();
    }

    if (status == ALT_E_SUCCESS)
    {
        alt_ecc_stop(ALT_ECC_RAM_QSPI);
    }

    if (status == ALT_E_SUCCESS)
    {
        alt_qspi_disable();
    }
    
    if (status == ALT_E_SUCCESS)
    {
        alt_qspi_uninit();
    }

    // Display status
    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: ECC QSPI Demo succeeded.\n\n");
    }
    else
    {
        printf("INFO: ECC QSPI Demo failed.\n\n");
    }

    return status;
}

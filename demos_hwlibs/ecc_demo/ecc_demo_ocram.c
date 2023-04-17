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
#include "alt_clock_manager.h"
#include "alt_globaltmr.h"
#include "socal/hps.h"
#include "socal/socal.h"

// Initialize interrupt subsystem and setup ECC OCRAM interrupt
ALT_STATUS_CODE socfpga_int_ocram_setup(ALT_INT_INTERRUPT_t int_id, ALT_INT_TRIGGER_t trigger);

// Cleanup interrupt subsystem and the specific ECC OCRAM interrupt
ALT_STATUS_CODE socfpga_int_ocram_cleanup(ALT_INT_INTERRUPT_t int_id);

// ECC OCRAM ISR Callback
void ocram_ecc_isr(uint32_t icciar, void * context);

// Reads OCRAM content
ALT_STATUS_CODE read_ocram_content(void * address, uint32_t size);

// Injects SERR and DERR into OCRAM
ALT_STATUS_CODE data_inject_error(uint32_t error_type);

// ECC OCRAM Demo
ALT_STATUS_CODE ecc_demo_ocram(void);

typedef enum ecc_error_e
{
    INJECT_SINGLE_ERROR,
    INJECT_DOUBLE_ERROR,

} ecc_error_t;

// Interrupt counters
static uint32_t serr = 0;
static uint32_t derr = 0;

// read value from L2 cache
static uint32_t value = 0;

/******************************************************************************/
/*!
 * Initialize interrupt subsystem and setup ECC interrupt.
 *
 * \param       int_id interrupt id
 * \param       trigger type of trigger to be used for the interrupt
 * \return      result of the function
 */
ALT_STATUS_CODE socfpga_int_ocram_setup(ALT_INT_INTERRUPT_t int_id, ALT_INT_TRIGGER_t trigger)
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
 * Cleanup interrupt subsystem and the specific ECC interrupt.
 *
 * \param       int_id interrupt id
 * \return      result of the function
 */
ALT_STATUS_CODE socfpga_int_ocram_cleanup(ALT_INT_INTERRUPT_t int_id)
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

    // Disable ECC OCRAM interrupt at the distributor level
    if(status == ALT_E_SUCCESS)
    {
        status = alt_int_dist_disable(int_id);
    }
    
    // Unregister ECC OCRAM ISR
    if(status == ALT_E_SUCCESS)
    {
        status = alt_int_isr_unregister(int_id);
    }
    
    return status;
}

/******************************************************************************/
/*!
 * OCRAM ECC ISR Callback 
 *
 * \param       icciar
 * \param       context ISR context.
 * \return      none
 */
void ocram_ecc_isr(uint32_t icciar, void * context)
{
    ALT_INT_INTERRUPT_t int_id = (ALT_INT_INTERRUPT_t)ALT_INT_ICCIAR_ACKINTID_GET(icciar);

    volatile uint32_t * count = (volatile uint32_t *)context;

    switch (int_id)
    {
    case ALT_INT_INTERRUPT_RAM_ECC_CORRECTED_IRQ:
        alt_ecc_status_clear(ALT_ECC_RAM_OCRAM, ALT_ECC_ERROR_OCRAM_SERR);
        break;
    case ALT_INT_INTERRUPT_RAM_ECC_UNCORRECTED_IRQ:
        alt_ecc_status_clear(ALT_ECC_RAM_OCRAM, ALT_ECC_ERROR_OCRAM_DERR);
        break;
    default:
        break;
    }
    ++(*count);
}

/******************************************************************************/
/*!
 * Reads OCRAM content
 *
 * \param  address  OCRAM start address
 * \param  size   size of data
 * \return        result of the function
 */
ALT_STATUS_CODE read_ocram_content(void * address, uint32_t size)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    uint32_t * count = address;

    for (uint32_t i = 0; i < (size / sizeof(uint32_t)) ; ++i)
    {
        value = alt_read_word(count);
        count++;
    }

    return status;
}

/******************************************************************************/
/*!
 * Injects SERR and DERR into OCRAM
 *
 * \param       error type
 * \return      result of the function
 */
ALT_STATUS_CODE data_inject_error(uint32_t error_type)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    // Generate random address for error injection
    uint32_t data_error = rand() % (((uint32_t)ALT_OCRAM_UB_ADDR - (uint32_t)ALT_OCRAM_LB_ADDR) + 1);
    data_error &= ~0x3;

    switch (error_type)
    {
    case INJECT_SINGLE_ERROR:
        // Inject SERR
        status = alt_ecc_serr_inject(ALT_ECC_RAM_OCRAM);
        status = alt_write_word((ALT_OCRAM_ADDR + data_error), 0x0000);
        break;
    case INJECT_DOUBLE_ERROR:
        // Inject DERR
        status = alt_ecc_derr_inject(ALT_ECC_RAM_OCRAM);
        status = alt_write_word((ALT_OCRAM_ADDR + data_error), 0x0000);
        break;
    }

    printf("INFO: Error injected at 0x%08x.\n",(int)(ALT_OCRAM_ADDR + data_error));

    return status;
}

/******************************************************************************/
/*!
 * Demonstrates injecting single bit error (SERR) and double bit error (DERR)
 * into OCRAM
 *
 * \return      result of the function
 */
ALT_STATUS_CODE ecc_demo_ocram(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    printf("INFO: ECC OCRAM Demo started.\n");
    
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Initialize and start ECC for OCRAM.\n");
        status = alt_ecc_start(ALT_ECC_RAM_OCRAM);
    }
    
    // Setup interrupts for SERR and DERR
    if (status == ALT_E_SUCCESS)
    {
        status = socfpga_int_ocram_setup(ALT_INT_INTERRUPT_RAM_ECC_CORRECTED_IRQ, ALT_INT_TRIGGER_AUTODETECT);
        status = socfpga_int_ocram_setup(ALT_INT_INTERRUPT_RAM_ECC_UNCORRECTED_IRQ, ALT_INT_TRIGGER_AUTODETECT);
    }

    // Register interrupt handlers for SERR and DERR
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_isr_register(ALT_INT_INTERRUPT_RAM_ECC_CORRECTED_IRQ, ocram_ecc_isr, &serr);
        status = alt_int_isr_register(ALT_INT_INTERRUPT_RAM_ECC_UNCORRECTED_IRQ, ocram_ecc_isr, &derr);
    }
 
    // Inject SERR
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Injecting SERR ...\n");
        status = data_inject_error(INJECT_SINGLE_ERROR);
    }
    
    // Read back OCRAM content
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Read OCRAM contents ...\n");
        status = read_ocram_content(ALT_OCRAM_ADDR, ((uint32_t)ALT_OCRAM_UB_ADDR - (uint32_t)ALT_OCRAM_LB_ADDR) + 1);
    }

    printf("INFO: SERR detected : %d.\n", (int)serr);

    // Inject DERR
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Injecting DERR ...\n");
        status = data_inject_error(INJECT_DOUBLE_ERROR);
    }

    // Read back OCRAM content
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Read OCRAM contents ...\n");
        status = read_ocram_content(ALT_OCRAM_ADDR, ((uint32_t)ALT_OCRAM_UB_ADDR - (uint32_t)ALT_OCRAM_LB_ADDR) + 1);
    }
    
    printf("INFO: DERR detected : %d.\n", (int)derr);

    // Cleanup interrupts for SERR and DERR
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Cleaning up ECC OCRAM interrupts.\n");
        status = socfpga_int_ocram_cleanup(ALT_INT_INTERRUPT_RAM_ECC_CORRECTED_IRQ);
        status = socfpga_int_ocram_cleanup(ALT_INT_INTERRUPT_RAM_ECC_UNCORRECTED_IRQ);
    }
        
    if (status == ALT_E_SUCCESS)
    {
        alt_ecc_stop(ALT_ECC_RAM_OCRAM);
    }

    // Display status
    if(status == ALT_E_SUCCESS)
    {
        printf("INFO: ECC OCRAM Demo succeeded.\n\n");
    }
    else
    {
        printf("INFO: ECC OCRAM Demo failed.\n\n");
    }

    return status;
}

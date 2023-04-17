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
#include <stdbool.h>
#include <inttypes.h>
#include "alt_interrupt.h"
#include "alt_globaltmr.h"
#include "socal/socal.h"

// System initialization
ALT_STATUS_CODE system_init(void);

// System shutdown
ALT_STATUS_CODE system_uninit(void);

// Demonstrates ECC on OCRAM
ALT_STATUS_CODE ecc_demo_ocram(void);

// Demonstrates ECC on QSPI
ALT_STATUS_CODE ecc_demo_qspi(void);

// Demonstrates ECC on SDMMC
ALT_STATUS_CODE ecc_demo_sdmmc(void);

// Demonstrates ECC on DMA
ALT_STATUS_CODE ecc_demo_dma(void);

// Demonstrates ECC on L2 cache
ALT_STATUS_CODE ecc_demo_l2(void);

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
        status = alt_globaltmr_init();
    }

    // Initialize global interrupts
    if(status == ALT_E_SUCCESS)
    {
        status = alt_int_global_init();
    }

    // Initialize CPU interrupts
    if(status == ALT_E_SUCCESS)
    {
        status = alt_int_cpu_init();
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
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    
    printf("INFO: System shutdown.\n");
    
    // Uninitialize CPU interrupts
    if(status == ALT_E_SUCCESS)
    {
        status = alt_int_cpu_uninit();
    }

    // Uninitialize Global interrupts
    if(status == ALT_E_SUCCESS)
    {
        status = alt_int_global_uninit();
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

    // OCRAM ECC Demo
    if(status == ALT_E_SUCCESS)
    {
        status = ecc_demo_ocram();
    }

    // QSPI ECC Demo
    if(status == ALT_E_SUCCESS)
    {

        status = ecc_demo_qspi();
    }

    // SDMMC ECC Demo
    if(status == ALT_E_SUCCESS)
    {

        status = ecc_demo_sdmmc();
    }

    // DMA ECC Demo
    if(status == ALT_E_SUCCESS)
    {

        status = ecc_demo_dma();
    }

    // L2 ECC Demo
    if(status == ALT_E_SUCCESS)
    {

        status = ecc_demo_l2();
    }

    // System uninit
    if(status == ALT_E_SUCCESS)
    {
        status = system_uninit();
    }

    // Report status
    if (status == ALT_E_SUCCESS)
    {
        printf("RESULT: All tests successful.\n\n");
        return 0;
    }
    else
    {
        printf("RESULT: Some failures detected.\n\n");
        return 1;
    }
}

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

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <stdbool.h>
//#include <inttypes.h>

#include "common.h"
#include "hwlibs/include/alt_interrupt.h"
#include "hwlibs/include/alt_globaltmr.h"
#include "hwlibs/include/alt_timers.h"
#include "hwlibs/include/alt_clock_manager.h"
#include "hwlibs/include/alt_watchdog.h"
#include "hwlibs/include/socal/hps.h"
#include "debugConsole.h"

//void alt_int_handler_irq(void);

// 1ms for private timer period
#define PRIVATETIMERTICKVALUE	1

// 200MHz / 200 pour avoir 1MHz -> 1us resolution
#define GLOBAL_TIMER_PRESCALER	199
	
#define TIMER_LOAD_VALUE 32000


volatile bool Private_Timer_Interrupt_Fired_core0     = false;
volatile bool Private_Timer_Interrupt_Fired_core1     = false;

uint64_t cntr_value[10];
uint32_t intr_cnt = 0;

uint32_t ledCounter_core0     =0;
uint32_t ledValue_core0       =0;
volatile uint32_t timerLoopCount_core0 =0;

uint32_t ledCounter_core1     =0;
uint32_t ledValue_core1       =0;
volatile uint32_t timerLoopCount_core1 =0;


static __inline uint32_t get_current_core_num(void)
{
    uint32_t affinity;

    /* Use the MPIDR. This is only available at PL1 or higher.
     / See ARMv7, section B4.1.106. */
    __asm("MRC p15, 0, %0,          c0, c0, 5" : "=r" (affinity));
    return affinity & 0xFF;
}


/******************************************************************************/
/*!
 * Initialize system
 *
 * \return      result of the function
 */
ALT_STATUS_CODE system_init(void)
{

	
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    puts("\n\r");
    puts("CORE0: System Initialization.\n\r");

    // Initialize global timer
    puts("CORE0: Setting up Global Timer.\n\r");
    if(!alt_globaltmr_int_is_enabled())
    {
		alt_globaltmr_prescaler_set(GLOBAL_TIMER_PRESCALER);
		status = alt_globaltmr_init();
    }

    return status;
}

/******************************************************************************/
/*!
 * Initialize interrupt subsystem and setup Timer interrupts.
 *
 * \return      result of the function
 */
ALT_STATUS_CODE soc_int_setup_core0(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    //int cpu_target = 0x1; //CPU0 will handle the interrupts

    //puts("\n\r");
    puts("CORE0: Interrupt Setup.\n\r");

    // Initialize global interrupts
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_global_init();
    }

    // Initialize CPU interrupts
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_cpu_init();
    }

    // Set interrupt trigger type
    if (status == ALT_E_SUCCESS)
    {
        //status = alt_int_dist_trigger_set(ALT_INT_INTERRUPT_PPI_TIMER_GLOBAL, ALT_INT_TRIGGER_AUTODETECT);
        status = alt_int_dist_trigger_set(ALT_INT_INTERRUPT_PPI_TIMER_PRIVATE, ALT_INT_TRIGGER_AUTODETECT);
    }

    // Enable interrupt at the distributor level
    if (status == ALT_E_SUCCESS)
    {
        //status = alt_int_dist_enable(ALT_INT_INTERRUPT_PPI_TIMER_GLOBAL);
        status = alt_int_dist_enable(ALT_INT_INTERRUPT_PPI_TIMER_PRIVATE);
    }

    // Set interrupt distributor target
    //if (status == ALT_E_SUCCESS)
    //{
    //    status = alt_int_dist_target_set(ALT_INT_INTERRUPT_TIMER_L4SP_1_IRQ, cpu_target);
    //}

    // Set interrupt trigger type
    //if (status == ALT_E_SUCCESS)
    //{
    //    status = alt_int_dist_trigger_set(ALT_INT_INTERRUPT_TIMER_L4SP_1_IRQ, ALT_INT_TRIGGER_AUTODETECT);
    //}


    // Enable interrupt at the distributor level
    //if (status == ALT_E_SUCCESS)
    //{
    //    status = alt_int_dist_enable(ALT_INT_INTERRUPT_TIMER_L4SP_1_IRQ);
    //}

    // Set interrupt distributor target
    //if (status == ALT_E_SUCCESS)
    //{
    //    status = alt_int_dist_target_set(ALT_INT_INTERRUPT_WDOG0_IRQ, cpu_target);
    //}

    // Set interrupt trigger type
    //if (status == ALT_E_SUCCESS)
    //{
    //    status = alt_int_dist_trigger_set(ALT_INT_INTERRUPT_WDOG0_IRQ, ALT_INT_TRIGGER_AUTODETECT);
    //}

    // Enable interrupt at the distributor level
    //if (status == ALT_E_SUCCESS)
    //{
    //    status = alt_int_dist_enable(ALT_INT_INTERRUPT_WDOG0_IRQ);
    //}

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

ALT_STATUS_CODE soc_int_setup_core1(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    //int cpu_target = 0x1; //CPU0 will handle the interrupts

    //puts("\n\r");
    puts("CORE1: Interrupt Setup.\n\r");

    // Initialize global interrupts
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_global_init();
		puts("CORE1: alt_int_global_init = ");
		putHexa32(status);
		puts(".\n\r");
	}

    // Initialize CPU interrupts
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_cpu_init();
		puts("CORE1: alt_int_cpu_init = ");
		putHexa32(status);
		puts(".\n\r");
	}

    // Set interrupt trigger type
    if (status == ALT_E_SUCCESS)
    {
        //status = alt_int_dist_trigger_set(ALT_INT_INTERRUPT_PPI_TIMER_GLOBAL, ALT_INT_TRIGGER_AUTODETECT);
        status = alt_int_dist_trigger_set(ALT_INT_INTERRUPT_PPI_TIMER_PRIVATE, ALT_INT_TRIGGER_EDGE);
		puts("CORE1: alt_int_dist_trigger_set = ");
		putHexa32(status);
		puts(".\n\r");
    }

    // Enable interrupt at the distributor level
    if (status == ALT_E_SUCCESS)
    {
		//status = alt_int_dist_enable(ALT_INT_INTERRUPT_PPI_TIMER_GLOBAL);
        status = alt_int_dist_enable(ALT_INT_INTERRUPT_PPI_TIMER_PRIVATE);
		puts("CORE1: alt_int_dist_enable = ");
		putHexa32(status);
		puts(".\n\r");
    }

    // Enable CPU interrupts
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_cpu_enable();
		puts("CORE1: alt_int_cpu_enable = ");
		putHexa32(status);
		puts(".\n\r");
    }

    // Enable global interrupts
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_global_enable();
		puts("CORE1: alt_int_global_enable = ");
		putHexa32(status);
		puts(".\n\r");
	}

    puts("CORE1: Interrupt Setup done whit status = ");
	putHexa32(status);
	puts(".\n\r");
	return status;
}


/******************************************************************************/
/*!
 * Private Timer ISR Callback
 *
 * \param       icciar
 * \param       context ISR context.
 * \return      none
 */

void private_timer_isr_callback_core0(long unsigned int uliDummy, void* pvDummy)
{
	volatile unsigned int*	ledData 	= (unsigned int *)0xFF709000;
    //volatile unsigned int*	Uart0Data 	= (unsigned int *)0xFFC02000;
	//*Uart0Data = (unsigned int) ('1');
	uint32_t	uiCoreNum =get_current_core_num();
	
    // Clear interrupt source don't care about the return value
    alt_gpt_int_clear_pending(ALT_GPT_CPU_PRIVATE_TMR);


    ledCounter_core0++;
    if(ledCounter_core0>500)
    //if(ledCounter>ledPeriod)
    {
        timerLoopCount_core0++;
        ledCounter_core0 =0;
        if(ledValue_core0==1) 
        {
            ledValue_core0 =0;
            if(uiCoreNum==0x00)	{puts("CORE0: IRQ '1' core0 func\n\r");*ledData = (unsigned int) (0);}
            if(uiCoreNum==0x01)	{puts("CORE1: IRQ '1' core0 func\n\r");}
        }
        else            
        {
            ledValue_core0 =1;
			if(uiCoreNum==0x00)	{puts("CORE0: IRQ '0' core0 func\n\r");*ledData = (unsigned int) (1 <<24);}
            if(uiCoreNum==0x01)	{puts("CORE1: IRQ '0' core0 func\n\r");}
        }
		
		//ledPeriod = ledPeriod + LED_PERIOD_STEP;
		//if(ledPeriod>LED_PERIOD_MAX)		ledPeriod = LED_PERIOD_MIN;
    }

    if(timerLoopCount_core0>10)
    {
        // Notify main thread
        if(uiCoreNum==0x00)	{puts("\n\rCORE0: private_timer_isr_callback_core0()\n\r");}
        if(uiCoreNum==0x01)	{puts("\n\rCORE1: private_timer_isr_callback_core0()\n\r");}
        //putc('N');
        Private_Timer_Interrupt_Fired_core0 = true;  // let main loop wait
        timerLoopCount_core0 =0;
    }

}

void private_timer_isr_callback_core1(long unsigned int uliDummy, void* pvDummy)
{
	volatile unsigned int*	ledData 	= (unsigned int *)0xFF709000;
    //volatile unsigned int*	Uart0Data 	= (unsigned int *)0xFFC02000;
	//*Uart0Data = (unsigned int) ('1');
	uint32_t	uiCoreNum =get_current_core_num();
	
    // Clear interrupt source don't care about the return value
    alt_gpt_int_clear_pending(ALT_GPT_CPU_PRIVATE_TMR);


    ledCounter_core1++;
    if(ledCounter_core1>500)
    //if(ledCounter>ledPeriod)
    {
        timerLoopCount_core1++;
        ledCounter_core1 =0;
        if(ledValue_core1==1) 
        {
            ledValue_core1 =0;
            if(uiCoreNum==0x00)	{puts("CORE0: IRQ '1' core1 func\n\r");*ledData = (unsigned int) (0);}
            if(uiCoreNum==0x01)	{puts("CORE1: IRQ '1' core1 func\n\r");}
        }
        else            
        {
            ledValue_core1 =1;
 			if(uiCoreNum==0x00)	{puts("CORE0: IRQ '0' core1 func\n\r");*ledData = (unsigned int) (1 <<24);}
            if(uiCoreNum==0x01)	{puts("CORE1: IRQ '0' core1 func\n\r");}
        }
		
		//ledPeriod = ledPeriod + LED_PERIOD_STEP;
		//if(ledPeriod>LED_PERIOD_MAX)		ledPeriod = LED_PERIOD_MIN;
    }

    if(timerLoopCount_core1>10)
    {
        // Notify main thread
        if(uiCoreNum==0x00)	{puts("\n\rCORE0: private_timer_isr_callback_core1()\n\r");}
        if(uiCoreNum==0x01)	{puts("\n\rCORE1: private_timer_isr_callback_core1()\n\r");}
        //putc('N');
        Private_Timer_Interrupt_Fired_core1 = true;  // let main loop wait
        timerLoopCount_core1 =0;
    }

}


ALT_STATUS_CODE private_timer_setup_core0(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    Private_Timer_Interrupt_Fired_core0 = false;

    puts("\n\r");
    puts("INFO: CORE0: Private Timer Setup.\n\r");

    // Set Private Timer to free-running was one-shot
    if(status == ALT_E_SUCCESS)
    {
        puts("CORE0: Setting Private Timer mode.\n\r");
        //status = alt_gpt_mode_set(ALT_GPT_SP_TMR1, ALT_GPT_RESTART_MODE_ONESHOT);
        status = alt_gpt_mode_set(ALT_GPT_CPU_PRIVATE_TMR, ALT_GPT_RESTART_MODE_PERIODIC);
   }

    // Load Private Timer value
    if(status == ALT_E_SUCCESS)
    {
        puts("CORE0: Setting Private Timer count value.\n\r");
        //status = alt_gpt_counter_set(ALT_GPT_SP_TMR1, 1024);
        //status = alt_gpt_counter_set(ALT_GPT_SP_TMR1, 65535);
        // #define CONFIG_HPS_CLK_L4_SP_HZ 100000000
		
		// periph clk semble etre 100MHz
		// period du timer = LoadValue / 100000000 si prescaler = 0
		
		
		/*
		alt_freq_t periphClk = 0;
		alt_clk_freq_get(ALT_CLK_MPU_PERIPH ,&periphClk); 
		// -> periphClk = 200MHz
		
		double timerPeriod = PRIVATETIMERTICKVALUE * 0.001;
		uint32_t uiLoadVal = (timerPeriod * periphClk) -1;
		puts("INFO: periphClk=");
		putHexa32(periphClk);
		puts("\r\nINFO: uiLoadVal=");
        putHexa32(uiLoadVal);
		puts("\r\n");
        */
		
		uint32_t uiLoadVal = 199999;	// if periphClk is still 200MHz
		status = alt_gpt_counter_set(ALT_GPT_CPU_PRIVATE_TMR, uiLoadVal /*100000*/ /*1000000000*/);        
    }

    //uint32_t timer_val = alt_gpt_curtime_millisecs_get(ALT_GPT_SP_TMR1);
    //puts("INFO: SP Timer1 Time to zero in milliseconds is ");
    //putHexa32(timer_val);
    //puts("Hz.\n\r");

    timerLoopCount_core0 =0;

    // Register timer ISR
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_isr_register(ALT_INT_INTERRUPT_PPI_TIMER_PRIVATE, private_timer_isr_callback_core0, NULL);
    }

    // Enable interrupt SP Timer 1
    if(status == ALT_E_SUCCESS)
    {
        puts("CORE0: Enabling Private Timer Interrupt.\n\r");
        status = alt_gpt_int_enable(ALT_GPT_CPU_PRIVATE_TMR);
    }

    // Set Private Timer to start running
    if(status == ALT_E_SUCCESS)
    {
        puts("CORE0: Starting Private Timer.\n\r");
        status = alt_gpt_tmr_start(ALT_GPT_CPU_PRIVATE_TMR);                
        //puts("done.\n\r");
    }

    return status;
}

ALT_STATUS_CODE private_timer_setup_core1(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    Private_Timer_Interrupt_Fired_core1 = false;

    puts("\n\r");
    puts("CORE1: Private Timer Setup Core1.\n\r");

    // Set Private Timer to free-running was one-shot
    if(status == ALT_E_SUCCESS)
    {
        puts("CORE1: Setting Private Timer mode.\n\r");
        //status = alt_gpt_mode_set(ALT_GPT_SP_TMR1, ALT_GPT_RESTART_MODE_ONESHOT);
        status = alt_gpt_mode_set(ALT_GPT_CPU_PRIVATE_TMR, ALT_GPT_RESTART_MODE_PERIODIC);
   }

    // Load private Timer value
    if(status == ALT_E_SUCCESS)
    {
        puts("CORE1: Setting Private Timer count value.\n\r");
        //status = alt_gpt_counter_set(ALT_GPT_SP_TMR1, 1024);
        //status = alt_gpt_counter_set(ALT_GPT_SP_TMR1, 65535);
        // #define CONFIG_HPS_CLK_L4_SP_HZ 100000000
		
		// periph clk semble etre 100MHz
		// period du timer = LoadValue / 100000000 si prescaler = 0
		
		
		/*
		alt_freq_t periphClk = 0;
		alt_clk_freq_get(ALT_CLK_MPU_PERIPH ,&periphClk); 
		// -> periphClk = 200MHz
		
		double timerPeriod = PRIVATETIMERTICKVALUE * 0.001;
		uint32_t uiLoadVal = (timerPeriod * periphClk) -1;
		puts("INFO: periphClk=");
		putHexa32(periphClk);
		puts("\r\nINFO: uiLoadVal=");
        putHexa32(uiLoadVal);
		puts("\r\n");
        */
		
		uint32_t uiLoadVal = 199999;	// if periphClk is still 200MHz to have 1kHz -> 1ms
		status = alt_gpt_counter_set(ALT_GPT_CPU_PRIVATE_TMR, uiLoadVal /*100000*/ /*1000000000*/);        
    }

    //uint32_t timer_val = alt_gpt_curtime_millisecs_get(ALT_GPT_SP_TMR1);
    //puts("INFO: SP Timer1 Time to zero in milliseconds is ");
    //putHexa32(timer_val);
    //puts("Hz.\n\r");

    timerLoopCount_core1 =0;

    // Register timer ISR
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_isr_register(ALT_INT_INTERRUPT_PPI_TIMER_PRIVATE, private_timer_isr_callback_core1, NULL);
    }

    // Enable interrupt SP Timer 1
    if(status == ALT_E_SUCCESS)
    {
        puts("CORE1: Enabling Private Timer Interrupt.\n\r");
        status = alt_gpt_int_enable(ALT_GPT_CPU_PRIVATE_TMR);
    }

    // Set Private Timer to start running
    if(status == ALT_E_SUCCESS)
    {
        puts("CORE1: Starting Private Timer.\n\r");
        status = alt_gpt_tmr_start(ALT_GPT_CPU_PRIVATE_TMR);                
        //puts("done.\n\r");
    }

    return status;
}


/******************************************************************************/
/*!
 * Global Timer Functions
 *
 * \return      result of the function
 */
ALT_STATUS_CODE global_timer_setup(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    
    
	puts("INFO : Global Timer Setup.\n\r");

    // Start Global Timer
    if(status == ALT_E_SUCCESS)
    {
        puts("INFO : Starting Global Timer.\n\r");
        status = alt_globaltmr_start();
    }

    return status;
}


/******************************************************************************/
/*!
 * Main entry point
 *
 */
int timers_init_Core0(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

	//0x18
	//0x1C

    /*
    unsigned char*  irqVector1 = 0xFF000034;     // essai FDy
    *irqVector1 = alt_int_handler_irq;           // essai FDy - surcharge directe du vecteur IRQ

    unsigned char*  fiqVector1 = 0xFF000038;     // essai FDy
    *fiqVector1 = alt_int_handler_irq;           // essai FDy - surcharge directe du vecteur FIQ

    unsigned char*  irqVector2 = 0x00000034;     // essai FDy
    *irqVector2 = alt_int_handler_irq;           // essai FDy - surcharge directe du vecteur IRQ

    unsigned char*  fiqVector2 = 0x00000038;     // essai FDy
    *fiqVector2 = alt_int_handler_irq;           // essai FDy - surcharge directe du vecteur FIQ
    */

    // System init
    if(status == ALT_E_SUCCESS)
    {
        status = system_init();
    }

	//#if false
    // Setup Interrupt (for private timer)
    if(status == ALT_E_SUCCESS)
    {
        status = soc_int_setup_core0();
    }

    // Private Timer set
    if(status == ALT_E_SUCCESS)
    {
        status = private_timer_setup_core0();
    }
	//#endif
	
	// Use Global timer to measure code snippet & ticks (delay...)
	// already done in system_init()
	//if(status == ALT_E_SUCCESS)
	//{
	//	  status = global_timer_setup();
	//}

	return 0;
}

int timers_init_Core1(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

	
    // System init
    //if(status == ALT_E_SUCCESS)
    //{
    //    status = system_init();
    //}

    // Setup Interrupt (for private timer)
    if(status == ALT_E_SUCCESS)
    {
        status = soc_int_setup_core1();
    }

    // Private Timer set
    if(status == ALT_E_SUCCESS)
    {
        status = private_timer_setup_core1();
    }

	
	return 0;
}






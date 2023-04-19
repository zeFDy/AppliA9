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

void alt_int_handler_irq(void);


#define TIMER_LOAD_VALUE 32000

ALT_STATUS_CODE system_init(void);
ALT_STATUS_CODE system_uninit(void);
ALT_STATUS_CODE timer0_freerun_demo(void);
ALT_STATUS_CODE timer1_oneshot_demo(void);
ALT_STATUS_CODE global_timer_demo(void);
ALT_STATUS_CODE watch_dog_demo(void);

// Initialize interrupt subsystem and setup timer interrupt
//ALT_STATUS_CODE timers_demo_soc_int_setup(void);

// Cleanup interrupt subsystem and the specific timer interrupt
//static ALT_STATUS_CODE timers_demo_soc_int_cleanup();

// ISR callback for the General Purpose Timer
//static void timers_demo_glbl_timer_isr_callback();

// ISR callback for the General Purpose Timer
//static void timers_demo_gp_timer_isr_callback();

// ISR callback for the Watchdog0
//static void timers_demo_wdog0_isr_callback();

volatile bool Global_Timer_Interrupt_Fired  = false;
volatile bool GP_Timer_Interrupt_Fired      = false;
volatile bool WDOG0_Interrupt_Fired         = false;
uint64_t cntr_value[10];
uint32_t intr_cnt = 0;

uint32_t ledCounter     =0;
#define		LED_PERIOD_STEP 10
#define 	LED_PERIOD_MIN	10
#define 	LED_PERIOD_MAX	250
uint32_t ledPeriod		=LED_PERIOD_MIN;
uint32_t ledValue       =0;
volatile uint32_t timerLoopCount =0;

/******************************************************************************/
/*!
 * Initialize system
 *
 * \return      result of the function
 */
ALT_STATUS_CODE timers_demo_system_init(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    puts("\n\r");
    puts("INFO: System Initialization.\n\r");

    // Initialize global timer
    if(status == ALT_E_SUCCESS)
    {
        puts("INFO: Setting up Global Timer.\n\r");
        if(!alt_globaltmr_int_is_enabled())
        {
            status = alt_globaltmr_init();
        }
    }

    // // Initialize general purpose timers
    // if(status == ALT_E_SUCCESS)
    // {
    //     puts("INFO: Initializing General Purpose Timers.\n\r");
    //     status = alt_gpt_all_tmr_init();
    // }

    // // Initialize watchdog0 timer
    // if(status == ALT_E_SUCCESS)
    // {
    //     puts("INFO: Initializing Watchdog 0 Timer.\n\r");
    //     status = alt_wdog_init();
    // }

    return status;
}

/******************************************************************************/
/*!
 * Initialize interrupt subsystem and setup Timer interrupts.
 *
 * \return      result of the function
 */
ALT_STATUS_CODE timers_demo_soc_int_setup(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    int cpu_target = 0x1; //CPU0 will handle the interrupts

    puts("\n\r");
    puts("INFO: Interrupt Setup.\n\r");

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
        status = alt_int_dist_trigger_set(ALT_INT_INTERRUPT_PPI_TIMER_GLOBAL, ALT_INT_TRIGGER_AUTODETECT);
    }

    // Enable interrupt at the distributor level
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_dist_enable(ALT_INT_INTERRUPT_PPI_TIMER_GLOBAL);
    }

    // Set interrupt distributor target
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_dist_target_set(ALT_INT_INTERRUPT_TIMER_L4SP_1_IRQ, cpu_target);
    }

    // Set interrupt trigger type
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_dist_trigger_set(ALT_INT_INTERRUPT_TIMER_L4SP_1_IRQ, ALT_INT_TRIGGER_AUTODETECT);
    }


    // Enable interrupt at the distributor level
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_dist_enable(ALT_INT_INTERRUPT_TIMER_L4SP_1_IRQ);
    }

    // Set interrupt distributor target
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_dist_target_set(ALT_INT_INTERRUPT_WDOG0_IRQ, cpu_target);
    }

    // Set interrupt trigger type
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_dist_trigger_set(ALT_INT_INTERRUPT_WDOG0_IRQ, ALT_INT_TRIGGER_AUTODETECT);
    }

    // Enable interrupt at the distributor level
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_dist_enable(ALT_INT_INTERRUPT_WDOG0_IRQ);
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

///******************************************************************************/
///*!
// * Global Timer Module ISR Callback
// *
// * \param       icciar
// * \param       context ISR context.
// * \return      none
// */
//
//void timers_demo_glbl_timer_isr_callback()
//{
//
//    // Clear int source don't care about the return value
//    alt_globaltmr_int_clear_pending();
//    cntr_value[intr_cnt] = alt_globaltmr_get64();
//    intr_cnt++;
//
//    // Notify main thread after 10 interrupts
//    if (intr_cnt == 10)
//    {
//        Global_Timer_Interrupt_Fired = true;
//    }
//}

/******************************************************************************/
/*!
 * Timer Module ISR Callback
 *
 * \param       icciar
 * \param       context ISR context.
 * \return      none
 */

void timers_demo_gp_timer_isr_callback(long unsigned int uliDummy, void* pvDummy)
{
	//volatile unsigned int*	ledData 	= (unsigned int *)0xFF709000;
    //volatile unsigned int*	Uart0Data 	= (unsigned int *)0xFFC02000;
	//*Uart0Data = (unsigned int) ('1');

    // Clear interrupt source don't care about the return value
    alt_gpt_int_clear_pending(ALT_GPT_SP_TMR1);


    ledCounter++;
    if(ledCounter>500)
    //if(ledCounter>ledPeriod)
    {
        timerLoopCount++;
        ledCounter =0;
        if(ledValue==1) 
        {
            ledValue =0;
            //*ledData = (unsigned int) (0);
            putc('H');
        }
        else            
        {
            ledValue =1;
 			//*ledData = (unsigned int) (1 <<24);
            putc('L');
        }
		
		//ledPeriod = ledPeriod + LED_PERIOD_STEP;
		//if(ledPeriod>LED_PERIOD_MAX)		ledPeriod = LED_PERIOD_MIN;
    }

    if(timerLoopCount>5)
    {
        // Notify main thread
        puts("\n\rNotify main thread\n\r");
        //putc('N');
        GP_Timer_Interrupt_Fired = true;  // let main loop wait
        timerLoopCount =0;
    }

}

///******************************************************************************/
///*!
// * Watchdog Module ISR Callback
// *
// * \param       icciar
// * \param       context ISR context.
// * \return      none
// */
//
//void timers_demo_wdog0_isr_callback()
//{
//
//    // Clear interrupt source don't care about the return value
//    alt_wdog_int_clear(ALT_WDOG0);
//
//    // Notify main thread
//    WDOG0_Interrupt_Fired = true;
//}

///******************************************************************************/
///*!
// * Set OSC0 Timer 0 to free-running.
// *
// * \return      result of the function
// */
//ALT_STATUS_CODE timers_demo_timer0_freerun_demo(void)
//{
//    ALT_STATUS_CODE status = ALT_E_SUCCESS;
//    uint32_t cnt_value = 65536;
//    uint32_t timer_val;
//
//
//    puts("\n\r");
//    puts("INFO: OSC 1 Timer 0 Free-run Demo.\n\r");
//
//    // Set OSC1 Timer 0 to free running mode
//    if(status == ALT_E_SUCCESS)
//    {
//        puts("INFO: Setting OSC1 Timer0 mode.\n\r");
//        status = alt_gpt_mode_set(ALT_GPT_OSC1_TMR0, ALT_GPT_RESTART_MODE_PERIODIC);
//    }
//
//    // Set OSC1 Timer 0 counter reset value
//    if(status == ALT_E_SUCCESS)
//    {
//        puts("INFO: Setting OSC1 Timer0 restart count value.\n\r");
//        status = alt_gpt_counter_set(ALT_GPT_OSC1_TMR0, cnt_value);
//    }
//
//    // Set OSC1 Timer 0 to start running
//    if(status == ALT_E_SUCCESS)
//    {
//        puts("INFO: Starting OSC1 Timer0.\n\r");
//        status = alt_gpt_tmr_start(ALT_GPT_OSC1_TMR0);
//    }
//
//    // Check if OSC1 Timer 0 is running
//    if(status == ALT_E_SUCCESS)
//    {
//        puts("INFO: Checking if OSC1 Timer0 is running.\n\r");
//        status = alt_gpt_tmr_is_running(ALT_GPT_OSC1_TMR0);
//        if(status == ALT_E_TRUE)
//        {
//            puts("INFO: OSC1 Timer0 is running.\n\r");
//            status = ALT_E_SUCCESS;
//        }
//        else
//        {
//            puts("FAIL: OSC1 Timer0 is not running.\n\r");
//            status = ALT_E_ERROR;
//        }
//    }
//
//    // Get Parameters of Osc1 Timer 0
//    if(status == ALT_E_SUCCESS)
//    {
//        timer_val = alt_gpt_freq_get(ALT_GPT_OSC1_TMR0);    // Notify main thread
//    //GP_Timer_Interrupt_Fired = true;  // let main loop wait
//
//        timer_val = alt_gpt_curtime_get(ALT_GPT_OSC1_TMR0);
//        puts("INFO: OSC1 Timer0 Time to zero in seconds is ");
//        putHexa32(timer_val);
//        puts("Hz.\n\r");
//
//        timer_val = alt_gpt_curtime_millisecs_get(ALT_GPT_OSC1_TMR0);
//        puts("INFO: OSC1 Timer0 Time to zero in milliseconds is ");
//        putHexa32(timer_val);
//        puts("Hz.\n\r");
//
//        timer_val = alt_gpt_curtime_microsecs_get(ALT_GPT_OSC1_TMR0);
//        puts("INFO: OSC1 Timer0 Time to zero in microseconds is ");
//        putHexa32(timer_val);
//        puts("Hz.\n\r");
//
//        timer_val = alt_gpt_maxtime_get(ALT_GPT_OSC1_TMR0);
//        puts("INFO: OSC1 Timer0 Max time in seconds is ");
//        putHexa32(timer_val);
//        puts("Hz.\n\r");
//
//        timer_val = alt_gpt_maxtime_millisecs_get(ALT_GPT_OSC1_TMR0);
//        puts("INFO: OSC1 Timer0 Max time in milliseconds is ");
//        putHexa32(timer_val);
//        puts("Hz.\n\r");
//    }
//
//    return status;
//}

/******************************************************************************/
/*!
 * Set SP Timer 1 to free-running was one-shot. (Clocked by l4_sp_clk)
 *
 * \return      result of the function
 */
ALT_STATUS_CODE timers_demo_timer1_oneshot_demo(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    GP_Timer_Interrupt_Fired = false;

    puts("\n\r");
    puts("INFO: SP Timer 1 One-shot Demo.\n\r");

    // Set SP Timer 1 to free-running was one-shot
    if(status == ALT_E_SUCCESS)
    {
        puts("INFO: Setting SP Timer 1 mode.\n\r");
        //status = alt_gpt_mode_set(ALT_GPT_SP_TMR1, ALT_GPT_RESTART_MODE_ONESHOT);
        status = alt_gpt_mode_set(ALT_GPT_SP_TMR1, ALT_GPT_RESTART_MODE_PERIODIC);
   }

    // Load SP Timer 1 value
    if(status == ALT_E_SUCCESS)
    {
        puts("INFO: Setting SP Timer 1 count value.\n\r");
        //status = alt_gpt_counter_set(ALT_GPT_SP_TMR1, 1024);
        //status = alt_gpt_counter_set(ALT_GPT_SP_TMR1, 65535);
        // #define CONFIG_HPS_CLK_L4_SP_HZ 100000000
        status = alt_gpt_counter_set(ALT_GPT_SP_TMR1, 100000);       // 100000 x 10ns -> 1ms 
    }

    //uint32_t timer_val = alt_gpt_curtime_millisecs_get(ALT_GPT_SP_TMR1);
    //puts("INFO: SP Timer1 Time to zero in milliseconds is ");
    //putHexa32(timer_val);
    //puts("Hz.\n\r");

    timerLoopCount =0;

    // Register timer ISR
    if (status == ALT_E_SUCCESS)
    {
        status = alt_int_isr_register(ALT_INT_INTERRUPT_TIMER_L4SP_1_IRQ, timers_demo_gp_timer_isr_callback, NULL);
    }

    // Enable interrupt SP Timer 1
    if(status == ALT_E_SUCCESS)
    {
        puts("INFO: Enabling SP Timer1 Interrupt.\n\r");
        status = alt_gpt_int_enable(ALT_GPT_SP_TMR1);
    }

    // Set SP Timer 1 to start running
    if(status == ALT_E_SUCCESS)
    {
        puts("INFO: Starting SP Timer1.\n\r");
        status = alt_gpt_tmr_start(ALT_GPT_SP_TMR1);                
        //puts("done.\n\r");
    }

    // Set SP Timer 1 to start running (this routine returns true, false or bad arg)
    if(status == ALT_E_SUCCESS)
    {
        puts("INFO: Checking if SP Timer 1 is running.\n\r");
        status = alt_gpt_tmr_is_running(ALT_GPT_SP_TMR1);
        if(alt_gpt_tmr_is_running(ALT_GPT_SP_TMR1) == ALT_E_TRUE)
        {
            status = ALT_E_SUCCESS;
            puts("INFO: SP Timer 1 is running.\n\r");
        }
        else
        {
            status = ALT_E_ERROR;
            puts("FAIL: SP Timer 1 is not running.\n\r");
        }
    }

    if(status == ALT_E_SUCCESS)
    {
        puts("INFO: Waiting for SP Timer1 Interrupt.\n\r");
        while (GP_Timer_Interrupt_Fired==false)
        {
        
        }

        //while(1)
        //{
        //
        //}

        //status = alt_gpt_int_disable(ALT_GPT_SP_TMR1);
        puts("\n\rINFO: SP Timer1 Interrupt Fired.\n\r");
    }

    return status;
}

///******************************************************************************/
///*!
// * Demo the Global Timer Functions
// *
// * \return      result of the function
// */
//ALT_STATUS_CODE timers_demo_global_timer_demo(void)
//{
//    ALT_STATUS_CODE status = ALT_E_SUCCESS;
//    uint64_t overhead, cntr_value_diff;
//
//    puts("\n\r");
//    puts("INFO: Global Timer Demo.\n\r");
//
//    // Start Global Timer
//    if(status == ALT_E_SUCCESS)
//    {
//        puts("INFO: Starting Global Timer.\n\r");
//        status = alt_globaltmr_start();
//    }
//
//    // Register Global Timer ISR
//    if (status == ALT_E_SUCCESS)
//    {
//        status = alt_int_isr_register(ALT_INT_INTERRUPT_PPI_TIMER_GLOBAL, timers_demo_glbl_timer_isr_callback, NULL);
//    }
//
//    // Get Global counter value
//    if(status == ALT_E_SUCCESS)
//    {
//        puts("INFO: Measuring for loop time.\n\r");
//        cntr_value[1] = alt_globaltmr_get64();
//        cntr_value[2] = alt_globaltmr_get64();
//        overhead = cntr_value[2] - cntr_value[1];
//
//        cntr_value[1] = alt_globaltmr_get64();
//        for (volatile int cntr = 1; cntr <= 511; cntr ++ )
//        {
//        }
//        cntr_value[2] = alt_globaltmr_get64();
//        cntr_value_diff = cntr_value[2] - cntr_value[1] - overhead;
//        puts("INFO: For loop time elapsed = ");
//        putHexa64(cntr_value_diff);
//        puts("Hz.\n\r");
//
//    }
//
//    // Set Global Timer autoinc value
//    if(status == ALT_E_SUCCESS)
//    {
//        puts("INFO: Setting Global Timer Comparator.\n\r");
//        cntr_value[1] = alt_globaltmr_get64();
//        status = alt_globaltmr_comp_set64(cntr_value[1] + TIMER_LOAD_VALUE);
//    }
//
//    // Set autoinc value
//    if(status == ALT_E_SUCCESS)
//    {
//        status = alt_globaltmr_autoinc_set(TIMER_LOAD_VALUE);
//    }
//
//    // Set Global Timer to autoinc mode
//    if(status == ALT_E_SUCCESS)
//    {
//        status = alt_globaltmr_autoinc_mode_start();
//    }
//
//    // if(status == ALT_E_SUCCESS)
//    // {
//    //     status = alt_globaltmr_comp_mode_start();
//    // }
//
//    // // Turn on Global Timer Interrupt
//    // if(status == ALT_E_SUCCESS)
//    // {
//    //     puts("INFO: Enabling Global Timer Interrupts.\n\r");
//    //     status = alt_globaltmr_int_enable();
//    // }
//
//    // if(status == ALT_E_SUCCESS)
//    // {
//    //     while (!Global_Timer_Interrupt_Fired)
//    //     {
//    //     }
//
//    //     status = alt_globaltmr_int_disable();
//    //     // Display the deltas between the interrupts
//    //     puts("INFO: Global Timer Interrupt Deltas.\n\r");
//    //     for (int i = 1; i <10; i++)
//    //     {
//    //         //printf("INFO: Global Timer Comparator Interrupt Delta %i = %" PRIu64".\n\r", i, cntr_value[i] - cntr_value[i-1]);
//    //         puts("INFO: Global Timer Comparator Interrupt Delta ");
//    //         putHexa64(i);
//    //         puts(" = ");
//    //         putHexa64(cntr_value[i] - cntr_value[i-1]);
//    //         puts("Hz.\n\r");
//    //     }
//    // }
//
//    return status;
//}

///******************************************************************************/
///*!
// * Watchdog Demo
// *
// * \return      result of the function
// */
//ALT_STATUS_CODE timers_demo_watch_dog_demo(void)
//{
//    ALT_STATUS_CODE status = ALT_E_SUCCESS;
//    uint32_t timer_val = 0, new_timer_val = 0;
//
//    puts("\n\r");
//    puts("INFO: CPU0 Watch Dog Demo.\n\r");
//
//    // Set Watchdog 0 to interrupt then reset mode
//    if(status == ALT_E_SUCCESS)
//    {
//        puts("INFO: Setting CPU0 Watchdog mode to Interrupt then Reset.\n\r");
//        status = alt_wdog_response_mode_set(ALT_WDOG0, ALT_WDOG_INT_THEN_RESET);
//    }
//
//    // Set Watchdog 0 Init value
//    if(status == ALT_E_SUCCESS)
//    {
//        puts("INFO: Setting CPU0 Watchdog Counter Initial Value.\n\r");
//        status = alt_wdog_counter_set(ALT_WDOG0_INIT, 0);
//    }
//
//    // Set Watchdog 0 counter value
//    if(status == ALT_E_SUCCESS)
//    {
//        puts("INFO: Setting CPU0 Watchdog Counter Reset Value.\n\r");
//        status = alt_wdog_counter_set(ALT_WDOG0, 1);
//    }
//
//    // Register wdog ISR
//    if (status == ALT_E_SUCCESS)
//    {
//        status = alt_int_isr_register(ALT_INT_INTERRUPT_WDOG0_IRQ, timers_demo_wdog0_isr_callback, NULL);
//    }
//
//    // Start Watchdog 0
//    if (status == ALT_E_SUCCESS)
//    {
//        puts("INFO: Starting Watchdog 0 Timer.\n\r");
//        status = alt_wdog_start(ALT_WDOG0);
//    }
//
//    // Check Watchdog 0 running
//    if (status == ALT_E_SUCCESS)
//    {
//        if (alt_wdog_tmr_is_enabled(ALT_WDOG0) == true)
//        {
//            puts("INFO: Watchdog 0 Timer is running.\n\r");
//        }
//        else
//        {
//            puts("INFO: Watchdog 0 Timer is not running.\n\r");
//        }
//    }
//
//    // Get Parameters of Wdog
//    if(status == ALT_E_SUCCESS)
//    {
//        timer_val = alt_wdog_counter_get_inittime_millisecs(ALT_WDOG0);
//        //printf("INFO: Watchdog 0 Initial time = %" PRIu32 " millisecs.\n\r",timer_val);
//        puts("INFO: Watchdog 0 Initial time = ");
//        putHexa32(timer_val);
//        puts("millisecs.\n\r");
//
//        timer_val = alt_wdog_counter_get_curtime_millisecs(ALT_WDOG0);
//        //printf("INFO: Watchdog 0 Current time = %" PRIu32 " millisecs.\n\r",timer_val);
//        puts("INFO: Watchdog 0 Current time = Delta ");
//        putHexa32(timer_val);
//        puts("millisecs.\n\r");
//
//        // Lets kill some time
//        for (volatile int cntr = 1; cntr <= 127; cntr ++ )
//        {
//        }
//
//        // Get the counter value
//        timer_val = alt_wdog_counter_get_current(ALT_WDOG0);
//        //printf("INFO: Watchdog 0 count = %" PRIu32 ".\n\r",timer_val);
//        puts("INFO: Watchdog 0 count = ");
//        putHexa32(timer_val);
//        puts("\n\r");
//
//        // Pet the dog
//        alt_wdog_reset(ALT_WDOG0);
//
//        // Get the new counter value should be greater than last time
//        new_timer_val = alt_wdog_counter_get_current(ALT_WDOG0);
//        //printf("INFO: Watchdog 0 count after reset= %" PRIu32 ".\n\r",new_timer_val);
//        puts("INFO: Watchdog 0 count after reset= ");
//        putHexa32(new_timer_val);
//        puts("\n\r");
//
//        if(new_timer_val < timer_val)
//        {
//            puts("INFO: Watchdog 0 didn't reset.\n\r");
//            status = ALT_E_ERROR;
//        }
//        else
//        {
//            puts("INFO: Watchdog 0 Reset.\n\r");
//        }
//
//    }
//
//    // Wait for watch dog to interrupt
//    if(status == ALT_E_SUCCESS)
//    {
//        puts("INFO: Waiting for Watchdog 0 Interrupt.\n\r");
//        while (!WDOG0_Interrupt_Fired)
//        {
//        }
//
//        puts("INFO: Watchdog 0 Interrupt Fired.\n\r");
//
//    }
//
//    // Stop Watchdog 0
//    if (status == ALT_E_SUCCESS)
//    {
//        puts("INFO: Stopping Watchdog 0 Timer.\n\r");
//        status = alt_wdog_stop(ALT_WDOG0);
//    }
//
//    return status;
//}

///******************************************************************************/
///*!
// * Cleanup interrupt subsystem and the specific Timer interrupts.
// *
// * \return      result of the function
// */
//static ALT_STATUS_CODE timers_demo_soc_int_cleanup()
//{
//    ALT_STATUS_CODE status = ALT_E_SUCCESS;
//
//    puts("\n\r");
//    puts("INFO: Cleaning up Timer interrupts.\n\r");
//
//    // Disable global interrupts
//    if(status == ALT_E_SUCCESS)
//    {
//        status = alt_int_global_disable();
//    }
//
//    // Disable CPU interrupts
//    if(status == ALT_E_SUCCESS)
//    {
//        status = alt_int_cpu_disable();
//    }
//
//    // Disable Global Timer interrupt at the distributor level
//    if(status == ALT_E_SUCCESS)
//    {
//        status = alt_int_dist_disable(ALT_INT_INTERRUPT_PPI_TIMER_GLOBAL);
//    }
//
//    // Unregister Global Timer ISR
//    if(status == ALT_E_SUCCESS)
//    {
//        status = alt_int_isr_unregister(ALT_INT_INTERRUPT_PPI_TIMER_GLOBAL);
//    }
//
//    // Disable Timer interrupt at the distributor level
//    if(status == ALT_E_SUCCESS)
//    {
//        status = alt_int_dist_disable(ALT_INT_INTERRUPT_TIMER_OSC1_1_IRQ);
//    }
//
//    // Unregister Timer ISR
//    if(status == ALT_E_SUCCESS)
//    {
//        status = alt_int_isr_unregister(ALT_INT_INTERRUPT_TIMER_OSC1_1_IRQ);
//    }
//
//    if(status == ALT_E_SUCCESS)
//    {
//        status = alt_int_dist_disable(ALT_INT_INTERRUPT_TIMER_L4SP_1_IRQ);
//    }
//
//    // Unregister Global Timer ISR
//    if(status == ALT_E_SUCCESS)
//    {
//        status = alt_int_isr_unregister(ALT_INT_INTERRUPT_TIMER_L4SP_1_IRQ);
//    }
//
//    return status;
//}

///******************************************************************************/
///*!
// * Un-init system
// *
// * \return      result of the function
// */
//ALT_STATUS_CODE timers_demo_system_uninit(void)
//{
//    ALT_STATUS_CODE status = ALT_E_SUCCESS;
//
//    puts("\n\r");
//    puts("INFO: Shutting Down System.\n\r");
//
//    if(status == ALT_E_SUCCESS)
//    {
//        status = alt_int_cpu_uninit();
//    }
//
//    if(status == ALT_E_SUCCESS)
//    {
//        status =  alt_int_global_uninit();
//    }
//
//    if(status == ALT_E_SUCCESS)
//    {
//        status =  alt_gpt_all_tmr_uninit();
//    }
//
//    if(status == ALT_E_SUCCESS)
//    {
//        status = alt_wdog_uninit();
//    }
//
//    return status;
//}
//
/******************************************************************************/
/*!
 * Main entry point
 *
 */
int timers_init(void)
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
        status = timers_demo_system_init();
    }

    // Setup Interrupt
    if(status == ALT_E_SUCCESS)
    {
        status = timers_demo_soc_int_setup();
    }

//    // Timer0 set to free running
//    if(status == ALT_E_SUCCESS)
//    {
//        status = timers_demo_timer0_freerun_demo();
//    }

    // Timer1 set to one shot
    if(status == ALT_E_SUCCESS)
    {
        status = timers_demo_timer1_oneshot_demo();
    }

//  // Use Global timer to measure code snippet
//  if(status == ALT_E_SUCCESS)
//  {
//      status = timers_demo_global_timer_demo();
//  }
//
//    // Demo CPU0 Watchdog
//    if(status == ALT_E_SUCCESS)
//    {
//        status = timers_demo_watch_dog_demo();
//    }
//
//    // Cleanup Interrupt
//    if(status == ALT_E_SUCCESS)
//    {
//        status = timers_demo_soc_int_cleanup();
//    }
//
//    // System uninit
//    if(status == ALT_E_SUCCESS)
//    {
//        status = timers_demo_system_uninit();
//    }
//
//    // Report status
//    if (status == ALT_E_SUCCESS)
//    {
//        puts("\n\rRESULT: All tests successful.\n\r");
//        return 0;
//    }
//    else
//    {
//        puts("\n\rRESULT: Some failures detected.\n\r");
//        return 1;
//    }

	return 0;
}




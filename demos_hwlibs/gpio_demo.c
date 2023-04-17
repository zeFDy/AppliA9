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
//#include "alt_generalpurpose_io.h"
//#include "alt_interrupt.h"
//#include "alt_globaltmr.h"
//#include "alt_clock_manager.h"

#include "../common.h"
#include "../hwlibs/include/alt_generalpurpose_io.h"
#include "../hwlibs/include/alt_interrupt.h"
#include "../hwlibs/include/alt_globaltmr.h"
#include "../hwlibs/include/alt_clock_manager.h"


#define LED_OUTPUT_DELAY_US     500000    // 500ms

//#define HPS_LED_ALL_BIT_MASK      0x0000F000
//#define HPS_LED_ALL_TURN_ON       0x00000000
//#define HPS_LED_ALL_TURN_OFF      0x0000F000

// #define HPS_LED_0_TURN_ON        0x00007000  // GPIO[44] (HPS_LED_0)
// #define HPS_LED_1_TURN_ON        0x0000B000  // GPIO[43] (HPS_LED_1)
// #define HPS_LED_2_TURN_ON        0x0000D000  // GPIO[42] (HPS_LED_2)
// #define HPS_LED_3_TURN_ON        0x0000E000  // GPIO[41] (HPS_LED_3)

// #define HPS_PB_0_ASSERT          0x01C00000  // GPIO[8] (HPS_PB_0)
// #define HPS_PB_1_ASSERT          0x01A00000  // GPIO[9] (HPS_PB_1)
// #define HPS_PB_2_ASSERT          0x01600000  // GPIO[10](HPS_PB_2)
// #define HPS_PB_3_ASSERT          0x00E00000  // GPIO[11](HPS_PB_3)

// DE10 NANO
#define HPS_LED_0_TURN_ON       (unsigned int) (1 <<24)     // GPIO[25] (HPS_LED_0)
#define HPS_LED_0_TURN_OFF      0                           // GPIO[25] (HPS_LED_0)
#define HPS_PB_0_ASSERT         0x01C00000                  // GPIO[24] (HPS_PB_0)

#define HPS_PB_INT_ALL_BIT_MASK     0x01E00000 // Interrupt bits for GPIO2

//ALT_GPIO_CONFIG_RECORD_t pb_gpio_init[] =
//{
//    { ALT_HLGPI_8, ALT_GPIO_PIN_INPUT, ALT_GPIO_PIN_LEVEL_TRIG_INT, ALT_GPIO_PIN_ACTIVE_LOW, 0, 0 }, //HPS_PB_0
//    { ALT_HLGPI_9, ALT_GPIO_PIN_INPUT, ALT_GPIO_PIN_LEVEL_TRIG_INT, ALT_GPIO_PIN_ACTIVE_LOW, 0, 0 }, //HPS_PB_1
//    { ALT_HLGPI_10, ALT_GPIO_PIN_INPUT, ALT_GPIO_PIN_LEVEL_TRIG_INT, ALT_GPIO_PIN_ACTIVE_LOW, 0, 0 },//HPS_PB_2
//    { ALT_HLGPI_11, ALT_GPIO_PIN_INPUT, ALT_GPIO_PIN_LEVEL_TRIG_INT, ALT_GPIO_PIN_ACTIVE_LOW, 0, 0 } //HPS_PB_3
//};

ALT_GPIO_CONFIG_RECORD_t led_gpio_init[] =
{
    { ALT_GPIO_1BIT_44, ALT_GPIO_PIN_OUTPUT, 0, 0, 0, ALT_GPIO_PIN_DATAZERO }, // HPS_LED_0
//  { ALT_GPIO_1BIT_43, ALT_GPIO_PIN_OUTPUT, 0, 0, 0, ALT_GPIO_PIN_DATAZERO }, // HPS_LED_1
//  { ALT_GPIO_1BIT_42, ALT_GPIO_PIN_OUTPUT, 0, 0, 0, ALT_GPIO_PIN_DATAZERO }, // HPS_LED_2
//  { ALT_GPIO_1BIT_41, ALT_GPIO_PIN_OUTPUT, 0, 0, 0, ALT_GPIO_PIN_DATAZERO }  // HPS_LED_3
};

uint32_t push_button_stat = 0;

// Determine size of an array
#define ARRAY_COUNT(array)  (sizeof(array) / sizeof(array[0]))

// System initialization
ALT_STATUS_CODE gpio_demo_system_init(void);

// System shutdown
ALT_STATUS_CODE gpio_demo_system_uninit(void);

// Delay function
ALT_STATUS_CODE gpio_demo_delay_us(uint32_t us);

// Initialize interrupt subsystem and setup GPIO interrupt
ALT_STATUS_CODE gpio_demo_socfpga_int_setup(ALT_INT_INTERRUPT_t int_id, ALT_INT_TRIGGER_t trigger);

// Cleanup interrupt subsystem and the specific GPIO interrupt
ALT_STATUS_CODE gpio_demo_socfpga_int_cleanup(ALT_INT_INTERRUPT_t int_id);

// ISR callback for the GPIO
void gpio_demo_gpio_isr_callback(uint32_t icciar, void * context);

// LED functions
ALT_STATUS_CODE gpio_demo_led_blink(void);
ALT_STATUS_CODE gpio_demo_led_scroll_right(void);
ALT_STATUS_CODE gpio_demo_led_scroll_left(void);
ALT_STATUS_CODE gpio_demo_led_off(void);

/******************************************************************************/
/*!
 * System Initialization
 * \return      result of the function
 */
ALT_STATUS_CODE gpio_demo_system_init(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    FDy_puts("INFO: System Initialization.\n\r");
    
    // Initialize global timer
    if(status == ALT_E_SUCCESS)
    {
        status = alt_globaltmr_init();
    }

    FDy_puts("INFO: Init GPIO module.\n\r");
    
    // Initialize GPIO module
    if(status == ALT_E_SUCCESS)
    {
        status = alt_gpio_init();
    }
    
    // Setup Interrupt
    if(status == ALT_E_SUCCESS)
    {
        status = gpio_demo_socfpga_int_setup(ALT_INT_INTERRUPT_GPIO2, ALT_INT_TRIGGER_LEVEL);
    }

    FDy_puts("INFO: Set up GPIO for LEDs.\n\r");
    
    // Setup GPIO LED
    if(status == ALT_E_SUCCESS)
    {
        status = alt_gpio_group_config(led_gpio_init, ARRAY_COUNT(led_gpio_init));
    }

    
    // removed the Input part
    // ----------------------
    //
    //FDy_puts("INFO: Set up GPIO for Push button.\n\r");

    // Setup GPIO PUSHBUTTON
    //if(status == ALT_E_SUCCESS)
    //{
    //    status = alt_gpio_group_config(pb_gpio_init, ARRAY_COUNT(pb_gpio_init));
    //}

    // removed the IRQ part
    // --------------------
    //

    // FDy_puts("INFO: Register interrupt handler.\n\r");
    
    // // Register slave ISR
    // if (status == ALT_E_SUCCESS)
    // {
    //     status = alt_int_isr_register(ALT_INT_INTERRUPT_GPIO2, gpio_demo_gpio_isr_callback, NULL);
    // }

    // // Enable GPIO interrupts
    // if (status == ALT_E_SUCCESS)
    // {
    //     status = alt_gpio_port_int_enable(ALT_GPIO_PORTC, HPS_PB_INT_ALL_BIT_MASK);
    // }

return status;
}

/******************************************************************************/
/*!
 * Initialize interrupt subsystem and setup GPIO interrupt.
 *
 * \param       int_id interrupt id
 * \param       trigger type of trigger to be used for the interrupt
 * \return      result of the function
 */
ALT_STATUS_CODE gpio_demo_socfpga_int_setup(ALT_INT_INTERRUPT_t int_id, ALT_INT_TRIGGER_t trigger)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    FDy_puts("INFO: Setting up GPIO interrupt.\n\r");

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
 * System Cleanup
 *
 * \return      result of the function
 */
ALT_STATUS_CODE gpio_demo_system_uninit(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
 
    FDy_puts("INFO: System uninit.\n\r");
 
    // Cleanup Interrupt
    if(status == ALT_E_SUCCESS)
    {
        status = gpio_demo_socfpga_int_cleanup(ALT_INT_INTERRUPT_GPIO2);
    }
    return status;
}

/******************************************************************************/
/*!
 * Cleanup interrupt subsystem and the specific GPIO interrupt.
 *
 * \param       int_id interrupt id
 * \return      result of the function
 */
ALT_STATUS_CODE gpio_demo_socfpga_int_cleanup(ALT_INT_INTERRUPT_t int_id)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    FDy_puts("INFO: Cleaning up GPIO interrupt.\n\r");

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

    // Disable GPIO interrupt at the distributor level
    if(status == ALT_E_SUCCESS)
    {
        status = alt_int_dist_disable(int_id);
    }

    // Unregister GPIO ISR
    if(status == ALT_E_SUCCESS)
    {
        status = alt_int_isr_unregister(int_id);
    }

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

    return status;
}

/******************************************************************************/
/*!
 * Delay function
 *
 * \param      us delay interval
 */
ALT_STATUS_CODE gpio_demo_delay_us(uint32_t us)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    
    uint64_t start_time = alt_globaltmr_get64();
    uint32_t timer_prescaler = alt_globaltmr_prescaler_get() + 1;
    uint64_t end_time;
    alt_freq_t timer_clock;

    status = alt_clk_freq_get(ALT_CLK_MPU_PERIPH, &timer_clock);
    end_time = start_time + us * ((timer_clock / timer_prescaler) / 1000000);

    while(alt_globaltmr_get64() < end_time)
    {
    }
    
    return status;
}

/******************************************************************************/
/*!
 * GPIO LED blink.
 *
 * \return      result of the function
 */
ALT_STATUS_CODE gpio_demo_led_blink(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    
    FDy_puts("INFO: HPS_PB_USER_0 was pushed.\n\r");
    
    if(status == ALT_E_SUCCESS)
    {
        status = alt_gpio_port_data_write(ALT_GPIO_PORTB, HPS_LED_0_TURN_ON, HPS_LED_0_TURN_ON);
    }    
    
    if(status == ALT_E_SUCCESS)
    {    
        status = gpio_demo_delay_us(LED_OUTPUT_DELAY_US);
    }
    
    if(status == ALT_E_SUCCESS)
    {
        status = alt_gpio_port_data_write(ALT_GPIO_PORTB, HPS_LED_0_TURN_ON, HPS_LED_0_TURN_OFF);
    }
    
    if(status == ALT_E_SUCCESS)
    {
        status = gpio_demo_delay_us(LED_OUTPUT_DELAY_US);
    }
    
    if(status == ALT_E_SUCCESS)
    {
        status = alt_gpio_port_data_write(ALT_GPIO_PORTB, HPS_LED_0_TURN_ON, HPS_LED_0_TURN_ON);
    }
    
    if(status == ALT_E_SUCCESS)
    {
        status = gpio_demo_delay_us(LED_OUTPUT_DELAY_US);
    }
    
    if(status == ALT_E_SUCCESS)
    {
        status = alt_gpio_port_data_write(ALT_GPIO_PORTB, HPS_LED_0_TURN_ON, HPS_LED_0_TURN_OFF);
    }
    
    if(status == ALT_E_SUCCESS)
    {
        status = gpio_demo_delay_us(LED_OUTPUT_DELAY_US);
    }
    
    return status;
}

/******************************************************************************/
/*!
 * GPIO LED scroll right.
 *
 * \return      result of the function
 */
// ALT_STATUS_CODE gpio_demo_led_scroll_right(void)
// {
//     ALT_STATUS_CODE status = ALT_E_SUCCESS;
    
//     FDy_puts("INFO: HPS_PB_USER_1 was pushed.\n\r");
    
//     if(status == ALT_E_SUCCESS)
//     {
//         status = alt_gpio_port_data_write(ALT_GPIO_PORTB, HPS_LED_ALL_BIT_MASK, HPS_LED_3_TURN_ON);
//     }
    
//     if(status == ALT_E_SUCCESS)
//     {
//         status = gpio_demo_delay_us(LED_OUTPUT_DELAY_US);
//     }
    
//     if(status == ALT_E_SUCCESS)
//     {
//         status = alt_gpio_port_data_write(ALT_GPIO_PORTB, HPS_LED_ALL_BIT_MASK, HPS_LED_2_TURN_ON);
//     }
    
//     if(status == ALT_E_SUCCESS)
//     {
//         status = gpio_demo_delay_us(LED_OUTPUT_DELAY_US);
//     }
    
//     if(status == ALT_E_SUCCESS)
//     {
//         status = alt_gpio_port_data_write(ALT_GPIO_PORTB, HPS_LED_ALL_BIT_MASK, HPS_LED_1_TURN_ON);
//     }
    
//     if(status == ALT_E_SUCCESS)
//     {
//         status = gpio_demo_delay_us(LED_OUTPUT_DELAY_US);
//     }
    
//     if(status == ALT_E_SUCCESS)
//     {
//         status = alt_gpio_port_data_write(ALT_GPIO_PORTB, HPS_LED_ALL_BIT_MASK, HPS_LED_0_TURN_ON);
//     }
    
//     if(status == ALT_E_SUCCESS)
//     {
//         status = gpio_demo_delay_us(LED_OUTPUT_DELAY_US);
//     }
    
//     return status;
//  }

/******************************************************************************/
/*!
 * GPIO LED scroll left.
 *
 * \return      result of the function
 */ 
// ALT_STATUS_CODE gpio_demo_led_scroll_left(void)
// {
//     ALT_STATUS_CODE status = ALT_E_SUCCESS;
    
//     FDy_puts("INFO: HPS_PB_USER_2 was pushed.\n\r");
    
//     if(status == ALT_E_SUCCESS)
//     {
//         status = alt_gpio_port_data_write(ALT_GPIO_PORTB, HPS_LED_ALL_BIT_MASK, HPS_LED_0_TURN_ON);
//     }
    
//     if(status == ALT_E_SUCCESS)
//     {
//         status = gpio_demo_delay_us(LED_OUTPUT_DELAY_US);
//     }
    
//     if(status == ALT_E_SUCCESS)
//     {
//         status = alt_gpio_port_data_write(ALT_GPIO_PORTB, HPS_LED_ALL_BIT_MASK, HPS_LED_1_TURN_ON);
//     }
    
//     if(status == ALT_E_SUCCESS)
//     {
//         status = gpio_demo_delay_us(LED_OUTPUT_DELAY_US);
//     }

//     if(status == ALT_E_SUCCESS)
//     {
//         status = alt_gpio_port_data_write(ALT_GPIO_PORTB, HPS_LED_ALL_BIT_MASK, HPS_LED_2_TURN_ON);
//     }
    
//     if(status == ALT_E_SUCCESS)
//     {
//         status = gpio_demo_delay_us(LED_OUTPUT_DELAY_US);
//     }
    
//     if(status == ALT_E_SUCCESS)
//     {
//         status = alt_gpio_port_data_write(ALT_GPIO_PORTB, HPS_LED_ALL_BIT_MASK, HPS_LED_3_TURN_ON);
//     }
    
//     if(status == ALT_E_SUCCESS)
//     {
//         status = gpio_demo_delay_us(LED_OUTPUT_DELAY_US);
//     }
    
//     return status;
// }

/******************************************************************************/
/*!
 * GPIO LED off.
 *
 * \return      result of the function
 */ 
// ALT_STATUS_CODE gpio_demo_led_off(void)
// {
//     ALT_STATUS_CODE status = ALT_E_SUCCESS;
    
//     FDy_puts("INFO: HPS_PB_USER_3 was pushed.\n\r");
    
//     if(status == ALT_E_SUCCESS)
//     {
//         status = alt_gpio_port_data_write(ALT_GPIO_PORTB, HPS_LED_ALL_BIT_MASK, HPS_LED_ALL_TURN_OFF);
//     }
        
//     if(status == ALT_E_SUCCESS)
//     {
//         //Disable GPIO interrupt to ready for system uninit
//         status = alt_gpio_port_int_disable(ALT_GPIO_PORTC, HPS_PB_INT_ALL_BIT_MASK);
//     }
    
//     return status;
// }

/******************************************************************************/
/*!
 * GPIO Module ISR Callback
 *
 * \param       icciar
 * \param       context ISR context.
 * \return      none
 */
// void gpio_demo_gpio_isr_callback(uint32_t icciar, void * context)
// {
//     ALT_STATUS_CODE status = ALT_E_SUCCESS;
//     uint32_t push_button_read = 0;

//     if (status == ALT_E_SUCCESS)
//     {
//         push_button_read = alt_gpio_port_int_status_get(ALT_GPIO_PORTC);
//         push_button_stat = push_button_read ^ HPS_PB_INT_ALL_BIT_MASK;
//     }

//     if ((push_button_stat == HPS_PB_0_ASSERT) && (status == ALT_E_SUCCESS))
//     {
//         status = gpio_demo_led_blink();
//     }

//     if ((push_button_stat == HPS_PB_1_ASSERT) && (status == ALT_E_SUCCESS))
//     {
//         status = gpio_demo_led_scroll_right();
//     }
    
//     if ((push_button_stat == HPS_PB_2_ASSERT) && (status == ALT_E_SUCCESS))
//     {
//         status = gpio_demo_led_scroll_left();
//     }
    
//     if ((push_button_stat == HPS_PB_3_ASSERT) && (status == ALT_E_SUCCESS))
//     {
//         status = gpio_demo_led_off();
//     }

//     return;
// }

/******************************************************************************/
/*!
 * Program entrypoint
 *
 * \return result of the program
 */
int gpio_demo_main(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    // System init
    if(status == ALT_E_SUCCESS)
    {
        status = gpio_demo_system_init();
    }
    
    /*
    FDy_puts("INFO: Press HPS_PB_USER_0: HPS LEDs blink.\n\r");
    FDy_puts("INFO: Press HPS_PB_USER_1: HPS LEDs scroll right.\n\r");
    FDy_puts("INFO: Press HPS_PB_USER_2: HPS LEDs scroll left.\n\r");
    FDy_puts("INFO: Press HPS_PB_USER_3: HPS LEDs turn off and exit demo.\n\r");
    */  
    
    if(status == ALT_E_SUCCESS)
    {
        // while (push_button_stat != HPS_PB_3_ASSERT)
        // {

        // // While loop to wait for GPIO interrupt triggers

        // }
        while(1)
        {
            gpio_demo_led_blink();
        }
    }
    
    // System uninit
    if(status == ALT_E_SUCCESS)
    {
        status = gpio_demo_system_uninit();
    }

    if (status == ALT_E_SUCCESS)
    {
        FDy_puts("INFO: GPIO demo exits.\n\r");
    }

    return 0;
}

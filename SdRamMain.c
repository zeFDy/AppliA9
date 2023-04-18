#include "common.h"
#include "hwlibs/include/alt_interrupt.h"
#include "hwlibs/include/alt_globaltmr.h"
#include "hwlibs/include/alt_timers.h"
#include "hwlibs/include/alt_clock_manager.h"
#include "hwlibs/include/alt_watchdog.h"
#include "hwlibs/include/socal/hps.h"
#include "ourDateTime.h"


// RESET MANAGER
#define SOCFPGA_RSTMGR_ADDRESS	0xffd05000
#define	RSTMGR_PERMODRESET		SOCFPGA_RSTMGR_ADDRESS + 0x14
#define	RSTMGR_PER2MODRESET		SOCFPGA_RSTMGR_ADDRESS + 0x18

#define 	MPU_BASE			0xfffec000
#define 	ALT_GIC_BASE_CPU	MPU_BASE + 0x0100
#define 	ALT_DIST_BASE_CPU	MPU_BASE + 0x1000

char		tempBuffer[20];

uint32_t	timerValueHigh;
uint32_t	timerValueLow;

extern bool GP_Timer_Interrupt_Fired;
extern void Cpu1CodeStart(void);

volatile unsigned int*	thisLedData 	= (unsigned int *)0xFF709000;
uint32_t 				thisLedValue    =0;
volatile uint32_t		j;
int 					iCounter;

static void alt_int_fixup_irq_stack(uint32_t stack_irq)
{
    __asm(
        "mrs r3, CPSR\n"
        //"msr CPSR_c, #(0x12 | 0x80 | 0x40)\n"		// asm is not happy with this line (no imediate mode in thumb mode)
        "mov r1, #0xD2\n"
		"msr CPSR_c, r1\n"
        "mov sp, %0\n"
        "msr CPSR_c, r3\n"
        : : "r" (stack_irq) : "r3"
        );
}

static __inline uint32_t get_current_sp(void)
{
    uint32_t uiSp;

    __asm("MOV %0, SP" : "=r" (uiSp));
    return uiSp;
}

static __inline uint32_t get_current_core_num(void)
{
    uint32_t affinity;

    /* Use the MPIDR. This is only available at PL1 or higher.
     / See ARMv7, section B4.1.106. */
    __asm("MRC p15, 0, %0,          c0, c0, 5" : "=r" (affinity));
    return affinity & 0xFF;
}


uint64_t  get_ticks(void)
{
	// semble etre OSC1TIMER0
	unsigned int uiCurrentValue =	readl(CONFIG_SYS_TIMERBASE + 0x04);
			
	/* increment tbu if tbl has rolled over */
	if (uiCurrentValue > timerValueLow)
		timerValueHigh--;

	timerValueLow = uiCurrentValue;	

	uint64_t	ui64returnValue = (uint64_t)(((uint64_t)timerValueHigh << 32) | timerValueLow);
	return 		ui64returnValue;
}

void putc(char c)
{
	volatile unsigned int*	Uart0_lsr 		= (unsigned int *)0xFFC02014;
	//#define UART_LSR_THRE 0x20

	//while ((serial_in(&com_port->lsr) & UART_LSR_THRE) == 0)
	while((*Uart0_lsr & 0x20)==0)	;	// check xmit empty
	
	volatile unsigned int*	Uart0Data = (unsigned int *)0xFFC02000;
	*Uart0Data = (unsigned int) (c);

}

void PutNible(uint8_t ucNibble)
{
	ucNibble = ucNibble + 0x30;
	if(ucNibble>0x39)	ucNibble+=0x07;	// 0x41 ('A') - 0x3A (>'9')

	putc((char)ucNibble);
}

void PutByte(uint8_t ucByte)
{
	unsigned char ucCarac = 0;

	ucCarac = (ucByte >> 4 ) & 0x0F;
	PutNible((char)ucCarac);

	ucCarac = ucByte & 0x0F;
	PutNible((char)ucCarac);
}

void putHexa32(uint32_t uiNumber)
{
	putc((char)'0');
	putc((char)'x');
	PutByte((uiNumber >> 24) & 0xFF);
	PutByte((uiNumber >> 16) & 0xFF);
	PutByte((uiNumber >>  8) & 0xFF);
	PutByte( uiNumber        & 0xFF);
}

void putHexa64(uint64_t ullNumber)
{
	putc((char)'0');
	putc((char)'x');

	PutByte((ullNumber >> 56) & 0xFF);
	PutByte((ullNumber >> 48) & 0xFF);
	PutByte((ullNumber >> 40) & 0xFF);
	PutByte((ullNumber >> 32) & 0xFF);
	
	PutByte((ullNumber >> 24) & 0xFF);
	PutByte((ullNumber >> 16) & 0xFF);
	PutByte((ullNumber >>  8) & 0xFF);
	PutByte( ullNumber        & 0xFF);
}

void puts(char *caMessage)
{
	while(*caMessage!=0)
	{
		putc(*caMessage++);
	}
}

void HexDump(uint8_t* pucStartAddr, unsigned int uiSize)
{
	char c = 0;
	
	for(unsigned int 	uiCounter =0;
						uiCounter<uiSize;
						uiCounter+=16)
	{
		putHexa32((uint32_t)pucStartAddr + uiCounter);
		puts(" - ");
		//memset(tempBuffer, 0, 20);
		for(int itempBufferCounter =0;
				itempBufferCounter <20;
				itempBufferCounter++)
		{
			tempBuffer[itempBufferCounter]=0;	
		}

		for(unsigned int 	uiLineCounter=0;
							uiLineCounter<16;
							uiLineCounter++)
		{
			if(uiCounter+uiLineCounter<= uiSize)
			{
				c = (char)pucStartAddr[uiCounter+uiLineCounter];
				PutByte(c);
				putc(' ');
				if(c<32||c>'z')	tempBuffer[uiLineCounter]='.';
				else 			tempBuffer[uiLineCounter]=c;
			}	
			else
			{
				puts("   ");
				tempBuffer[uiLineCounter] = ' ';
			}
		}

		puts("- ");
		puts(tempBuffer);
		puts("\n\r");
	}
}

void 		Cpu1Code(void)
{
	uint32_t	uiMainCounter =0;
	
	//*thisLedData = (unsigned int) (1 <<24);
	uint32_t affinity = get_current_core_num();
	puts("Cpu1Code affinity = 0x");
	PutByte((uint8_t)affinity);
	puts("\r\n");
	if(affinity==1)	puts("RUNNING ON CORE1\r\n");
	
	puts("SP=");
	putHexa32(get_current_sp());
	puts("\r\n");
	
	while(1)
	{
		// soft loop for around 500ms (5000000)
		//for(iCounter=0;iCounter<5000000;iCounter++)
		for(iCounter=0;iCounter<2000000;iCounter++)
		{
			j=iCounter*2;
		}

		uiMainCounter++;
		
		if(thisLedValue==1) 
		{
			thisLedValue =0;
			*thisLedData = (unsigned int) (0);
			//putc('h');
		}
		else            
		{
			thisLedValue =1;
			*thisLedData = (unsigned int) (1 <<24);
			//putc('l');
		}
		
		// next line to be modified (as % implies division and therfore lib.a)
		//if(uiMainCounter%100==0)	puts("CORE1 kicks !\r\n");
		if(uiMainCounter>=100)	
		{
			puts("CORE1 kicks !\r\n");
			uiMainCounter =0;
		}
	}
}

void SdRamMain(void)
{
	//volatile uint32_t*	ledData 			=(uint32_t *)0xFF709000;
	//int 					iLed 				=0;
	int 					iCounter 			=0;
	uint64_t 				ui64InitialValue 	=0;
		
	// Semble etre OSC1TIMER0	
	// timer init :
	#define TIMER_LOAD_VAL		0xFFFFFFFF
	//writel(TIMER_LOAD_VAL, &timer_base->load_val);
	writel(TIMER_LOAD_VAL, CONFIG_SYS_TIMERBASE + 0x00);
	//writel(TIMER_LOAD_VAL, &timer_base->curr_val);
	writel(TIMER_LOAD_VAL, CONFIG_SYS_TIMERBASE + 0x04);
	//writel(readl(&timer_base->ctrl) | 0x3, &timer_base->ctrl);
	// TimerControlReg = 0x03 - Interrupt Disabled + User-defined count mode + Timer Enable 
	// TimerControlReg = 0x07 - Interrupt Enabled  + User-defined count mode + Timer Enable 
	writel(readl(CONFIG_SYS_TIMERBASE + 0x08) | 0x3, CONFIG_SYS_TIMERBASE + 0x08);
	// Je supose qu'avec une valeur de chargement de 0xFFFFFFFF
	// le timer est downcounter...
	timerValueHigh	=TIMER_LOAD_VAL;
	timerValueLow   =TIMER_LOAD_VAL;


	puts("Hello my friend\n\r");
	puts("Welcome to the other side !\n\r");
	//puts("Version by FDy ("__DATE__" - "__TIME__")\n\r");	
	puts("Version by FDy ("OUR_DATE_TIME_FULL")\n\r");	

	puts("\n\r");
	puts("              ,   .-'\"'=;_  ,                \n\r");
	puts("              |\\.'-~`-.`-`;/|                 \n\r");
	puts("              \\.` '.'~-.` './                \n\r");
	puts("              (\\`,__=-'__,'/)                \n\r");
	puts("           _.-'-.( 0\\_/0 ).-'-._             \n\r");
	puts("         /'.-'   ' .---. '   '-.`\\           \n\r");
	puts("       /'  .' (=    (_)    =) '.  `\\         \n\r");
	puts("      /'  .',  `-.__.-.__.-'  ,'.  `\\        \n\r");
	puts("     (     .'.   V       V  ; '.     )       \n\r");
	puts("     (    |::  `-,__.-.__,-'  ::|    )       \n\r");
	puts("     |   /|`:.               .:'|\\   |       \n\r");
	puts("     |  / | `:.              :' |`\\  |       \n\r");
	puts("     | |  (  :.-------------.:  )  | |       \n\r");
	puts("     | |   ( `:.    FDy     :' )   | |       \n\r");
	puts("     | |    \\ :.-----------.: /    | |       \n\r");
	puts("     | |     \\`:.         .:'/     | |       \n\r");
	puts("     ) (      `\\`:.     .:'/'      ) (       \n\r");
	puts("     (  `)_     ) `:._.:' (     _(`  )       \n\r");
	puts("     \\  ' _)  .'           `.  (_ `  /       \n\r");
	puts("      \\  '_) /   .'\"```\"'.   \\ (_`  /        \n\r");
	puts("       `'\"`  \\  (         )  /  `\"'`         \n\r");
	puts("   ___        `.`.       .'.'        ___     \n\r");
	puts(" .`   ``\"\"\"'''--`_)     (_'--'''\"\"\"``   `.   \n\r");
	puts("(_(_(___...--'\"'`         `'\"'--...___)_)_)  \n\r");
	puts("\n\r");

	uint32_t affinity = get_current_core_num();
	puts((char*)"SdRamMain affinity = 0x");
	PutByte((uint8_t)affinity);
	puts((char*)"\r\n");
	if(affinity==0)	puts("RUNNING ON CORE0\r\n");
	puts((char*)"SP=");
	putHexa32(get_current_sp());
	puts((char*)"\r\n");
	
	
	alt_int_fixup_irq_stack(0x10000);									// Set IRQ Stack Pointer to 0x10000 -> Fix it !
	alt_write_word(ALT_GIC_BASE_CPU + 0x4, 0xFF /*priority_mask*/); 	// Set priority mask to 0xFF so IRq are forwarded to CPU /* iccpmr */
	
	//timers_demo_main();			
	//puts("End of timer demo...\n\r");
	
	// lever de reset du Core1
	volatile uint32_t*		cpu1startaddr 		= (uint32_t *)0xFFD080C4;
	//volatile uint32_t*	mpumodrst 			= (uint32_t *)0xFFD05010;
	
	// cpu1 start
	//*cpu1startaddr = 0x0000010C;
	*cpu1startaddr = (uint32_t)Cpu1CodeStart;
	
	//unsigned int mpumodrstValue = *mpumodrst;
	//*mpumodrst = mpumodrstValue & 0xFFFFFFFD;
	
	//unreset cpu1
	clrbits_le32(RSTMGR_MPUMODRESET, 1 << 1);		// cpu1=b1

	// Clock for private timer is PERIPHCLK
	//volatile uint32_t*	privateTimerLoadValue 		= (uint32_t *)0xFFFEC600;
	//volatile uint32_t*	privateTimerCounter 		= (uint32_t *)0xFFFEC604;
	volatile uint32_t*		privateTimerControl 		= (uint32_t *)0xFFFEC608;
	//volatile uint32_t*	privateTimerStatus 			= (uint32_t *)0xFFFEC60C;
	
	// set load value for private timer
	//*privateTimerLoadValue 			=  500000000;			// 1s if clock is 200 MHz
	//*privateTimerLoadValue 			= 2500000000;			
	// si ok, faire un essai avec writel()
	writel(1000000000, 0xFFFEC600);
	
	// start private timer
	//*privateTimerControl	= 3;					// start the timer without IRQ (=7 for IRQ)
	*privateTimerControl	= 7;					// b0 = Enable
													// b1 = 1 -> Auto (loop) / 0 -> Once
													// b2 = IRQ (=1 IRQ is generated when value goes to 0)
													// if IRQ is ON, interrupt ID29	is pending
													// (when occurs) in Interrupt Distributor		
	
	
	while(1)
	{
		// soft loop for around 500ms
		//for(int i=0;i<5000000;i++)
		//{
		//	volatile	int	j=i*2;
		//}

		//while(GP_Timer_Interrupt_Fired==false);
		//puts(".");
		//GP_Timer_Interrupt_Fired=false;
		
		//#if false

		// on fait par IRQ timer
		// if(1==iLed)
		// {
		// 	*ledData = (unsigned int) (0);
		// 	iLed =0;
		// 	//putc('L');
		// }
		// else
		// {
		// 	*ledData = (unsigned int) (1 <<24);
		// 	iLed =1;
		// 	//putc('H');
		// }
		
		// next line to be modified (as % implies division and therfore lib.a)
		//if(iCounter%10=0)	
		//if(iCounter>=10)	
		//{
		//	ui64InitialValue 							=get_ticks();
			unsigned int uiCurrentPrivateTimerValue 	=readl(0xFFFEC604);
			unsigned int uiCurrentPrivateTimerStatus 	=readl(0xFFFEC60C);
	
		//	puts((char*)"OSC1TIMER0 =");
		//	putHexa64(ui64InitialValue);
		//	puts((char*)" - PRIVATETIMER =");
			puts((char*)"PRIVATETIMER=");
			//putHexa32(*privateTimerCounter);
			//puts((char*)" (");
			putHexa32(uiCurrentPrivateTimerValue);
			puts((char*)" - ");
			putHexa32(uiCurrentPrivateTimerStatus);
			//puts((char*)") - Alive and kicking...\n\r");
			//puts((char*)" - Alive and kicking.\n\r");
			puts((char*)"\n\r");
			
		//	iCounter =0;
		//}
		//#endif
		
		ui64InitialValue = get_ticks();

		while(1)
		{
			uint64_t ui64ThisTickValue = get_ticks();
			if(ui64InitialValue-ui64ThisTickValue>=5000000)	break;
		}

		iCounter++;
		
	}
}


//#if false
void alt_int_handler_irq(void)
//#endif /* #if ALT_INT_PROVISION_VECTOR_SUPPORT */
{

    // Set Led HPS to 1 (On)
	volatile unsigned int*	ledData = (unsigned int *)0xFF709000;
	*ledData = (unsigned int) (1 <<24);

	//volatile unsigned int*	Uart0Data = (unsigned int *)0xFFC02000;
	//*Uart0Data = (unsigned int) ('0');

    putc('.');

	// uint32_t icciar = alt_read_word(alt_int_base_cpu + 0xC);

    // uint32_t ackintid = ALT_INT_ICCIAR_ACKINTID_GET(icciar);

    // //FDy_putc('-');

    // if (ackintid < ALT_INT_PROVISION_INT_COUNT)
    // {
    //     if (alt_int_dispatch[ackintid].callback)
    //     {
    //         alt_int_dispatch[ackintid].callback(icciar, alt_int_dispatch[ackintid].context);
    //     }
    // }
    // else
    // {
    //     /* Report error. */
    //     //dprintf("INT[ISR]: Unhandled interrupt ID = 0x%" PRIx32 ".\n", ackintid);
    //     puts("INT[ISR]: Unhandled interrupt ID = ");
	// 	putHexa32(ackintid);
	// 	puts(".\n\r");		
		
    // }

    // alt_write_word(alt_int_base_cpu + 0x10, icciar); /* icceoir */
	
}
//#endif

void hang(void)
{
	puts("### ERROR ### Please RESET the board ###\n");
	for (;;);
}

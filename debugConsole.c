#include "common.h"

char tempBuffer[20];

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


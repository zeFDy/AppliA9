#ifndef	__DEBUGCONSOLE_H__
#define	__DEBUGCONSOLE_H__

void putc(char c);
void PutNible(uint8_t ucNibble);
void PutByte(uint8_t ucByte);
void putHexa32(uint32_t uiNumber);
void putHexa64(uint64_t ullNumber);
void puts(char *caMessage);
void HexDump(uint8_t* pucStartAddr, unsigned int uiSize);

#endif
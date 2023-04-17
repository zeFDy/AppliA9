#ifndef __COMMON_H__
#define __COMMON_H__    1
#include <stdint.h>
// pour utiliser la hwlibs sur Cyclone V
#define soc_cv_av

#define	cpu_to_le32(x)	(x)
#define le32_to_cpu(x)	(x)

typedef unsigned char 		u8;
typedef unsigned char 		uint8_t;
typedef unsigned short 		u16;
typedef unsigned short 		uint16_t;
typedef unsigned int 		ulong;
typedef unsigned int 		u32;
//typedef unsigned int 		uint32_t;
typedef unsigned int 		size_t;
typedef int 				s32;
//typedef int 				int32_t;
typedef unsigned long long 	uint64_t;
typedef long long 			int64_t;

/*!
 * This is the system wide cache line size, given in bytes.
 */
#define ALT_CACHE_LINE_SIZE         32

#define	EIO		 	5	/* I/O error */
#define	EINVAL		22	/* Invalid argument */
#define NULL		0

#define ISB	asm volatile ("isb sy" : : : "memory")
#define DSB	asm volatile ("dsb sy" : : : "memory")
#define DMB	asm volatile ("dmb sy" : : : "memory")
#define isb()	ISB
#define dsb()	DSB
#define dmb()	DMB

#define SOCFPGA_SYSMGR_ADDRESS		0xffd08000
#define SYSMGR_FRZCTRL_ADDRESS 		0x40
#define FRZCTRL_ADDRESS				SOCFPGA_SYSMGR_ADDRESS + SYSMGR_FRZCTRL_ADDRESS
#define SOCFPGA_MPUL2_ADDRESS		0xfffef000
#define CONFIG_SYS_PL310_BASE		SOCFPGA_MPUL2_ADDRESS
#define SOCFPGA_MPUSCU_ADDRESS		0xfffec000
#define SOCFPGA_L3REGS_ADDRESS		0xff800000
#define SOCFPGA_OSC1TIMER0_ADDRESS	0xffd00000
#define CONFIG_SYS_TIMERBASE		SOCFPGA_OSC1TIMER0_ADDRESS
#define SOCFPGA_RSTMGR_ADDRESS		0xffd05000
#define SOCFPGA_SCANMGR_ADDRESS		0xfff02000
#define FRZCTRL_ADDRESS				SOCFPGA_SYSMGR_ADDRESS + SYSMGR_FRZCTRL_ADDRESS
#define SOCFPGA_SDR_ADDRESS			0xffc20000

// NIC301
#define NIC301_LWHPS2FPGAREGS	SOCFPGA_L3REGS_ADDRESS + 0x20
#define NIC301_HPS2FPGAREGS		SOCFPGA_L3REGS_ADDRESS + 0x90
#define NIC301_ACP				SOCFPGA_L3REGS_ADDRESS + 0x94
#define NIC301_ROM				SOCFPGA_L3REGS_ADDRESS + 0x98
#define NIC301_OCRAM			SOCFPGA_L3REGS_ADDRESS + 0x9C
#define NIC301_SDRDATA			SOCFPGA_L3REGS_ADDRESS + 0xA0

// RESET MANAGER
#define RSTMGR_MPUMODRESET		SOCFPGA_RSTMGR_ADDRESS + 0x10
#define	RSTMGR_PERMODRESET		SOCFPGA_RSTMGR_ADDRESS + 0x14
#define	RSTMGR_PER2MODRESET		SOCFPGA_RSTMGR_ADDRESS + 0x18

#define SYSMGR_ROMCODEGRP_CTRL_WARMRSTCFGIO	(1 << 1)


/*
 * Generic virtual read/write.  Note that we don't support half-word
 * read/writes.  We define __arch_*[bl] here, and leave __arch_*w
 * to the architecture specific code.
 */
#define __arch_getl(a)			(*(volatile unsigned int *)(a))
#define __arch_putl(v,a)		(*(volatile unsigned int *)(a) = (v))

#define __raw_writel(v,a)	__arch_putl(v,a)
#define __raw_readl(a)		__arch_getl(a)

#define mb()		dsb()
#define __iormb()	dmb()
#define __iowmb()	dmb()

#define writel(v,c)	({ u32 __v = v; __iowmb(); __arch_putl(__v,c); __v; })
#define readl(c)	({ u32 __v = __arch_getl(c); __iormb(); __v; })

#define out_arch(type,endian,a,v)	__raw_write##type(cpu_to_##endian(v),a)
#define in_arch(type,endian,a)		endian##_to_cpu(__raw_read##type(a))

#define out_le64(a,v)	out_arch(q,le64,a,v)
#define out_le32(a,v)	out_arch(l,le32,a,v)
#define out_le16(a,v)	out_arch(w,le16,a,v)

#define in_le64(a)	in_arch(q,le64,a)
#define in_le32(a)	in_arch(l,le32,a)
#define in_le16(a)	in_arch(w,le16,a)

#define clrbits(type, addr, clear) \
	out_##type((addr), in_##type(addr) & ~(clear))

#define setbits(type, addr, set) \
	out_##type((addr), in_##type(addr) | (set))
	
#define clrsetbits(type, addr, clear, set) \
	out_##type((addr), (in_##type(addr) & ~(clear)) | (set))

#define clrbits_le32(addr, clear) clrbits(le32, addr, clear)
#define setbits_le32(addr, set) setbits(le32, addr, set)
#define clrsetbits_le32(addr, clear, set) clrsetbits(le32, addr, clear, set)



#endif
#ifndef CORE_COMMON_H
#define CORE_COMMON_H
// clang-format off

/* Display */
#define SCREEN_WIDTH        160
#define SCREEN_HEIGHT       144
#define SCREEN_RESOLUTION   (SCREEN_WIDTH * SCREEN_HEIGHT)

/* CPU */
#define CPU_FREQUENCY   4194304

/* Memory */
#define BANK0           0x0000
#define BANK1           0x4000
#define VRAM            0x8000
#define EXRAM           0xA000
#define WRAM1           0xC000
#define WRAM2           0xD000
#define ECHO_RAM        0xE000
#define OAM             0xFE00
#define NOT_USABLE      0xFEA0
#define IO              0xFF00
#define HRAM            0xFF80

/* Macros helpers for converting an absolute address to the offest value for its memory area */
#define VRAM_OFFSET(ADDR)               ((ADDR) & 0x1FFF)
#define WRAM_OFFSET(ADDR)               ((ADDR) & 0x1FFF)
#define OAM_OFFSET(ADDR)                ((ADDR) & 0xFF)
#define UNUSABLE_MEM_OFFSET(ADDR)       ((ADDR) - NOT_USABLE)
#define IO_OFFSET(ADDR)                 ((ADDR) & 0x7F)
#define HRAM_OFFSET(ADDR)               ((ADDR) & 0x7F)

/* Memory sizes */
#define BANK0_SIZE      0x4000
#define BANK1_SIZE      0x4000
#define VRAM_SIZE       0x2000
#define EXRAM_SIZE      0x2000
#define WRAM_SIZE       0x2000
#define OAM_SIZE        0xA0
#define NOT_USABLE_SIZE 0x70
#define IO_SIZE         0x80
#define HRAM_SIZE       0x7F

/* I/O Registers */
#define JOYP            0xFF00
#define SB              0xFF01
#define SC              0xFF02
#define DIV             0xFF04
#define TIMA            0xFF05
#define TMA             0xFF06
#define TAC             0xFF07
#define IF              0xFF0F

// Sound Channel 1 - Pulse with period sweep
#define NR10            0xFF10
#define NR11            0xFF11
#define NR12            0xFF12
#define NR13            0xFF13
#define NR14            0xFF14

// Sound Channel 2 - Pulse */
#define NR21            0xFF16
#define NR22            0xFF17
#define NR23            0xFF18
#define NR24            0xFF19

// Sound Channel 3 - Wave output
#define NR30            0xFF1A
#define NR31            0xFF1B
#define NR32            0xFF1C
#define NR33            0xFF1D
#define NR34            0xFF1E

// Sound Channel 4 - Noise
#define NR41            0xFF20
#define NR42            0xFF21
#define NR43            0xFF22
#define NR44            0xFF23

// Sound global control registers
#define NR50            0xFF24 // Master volume ^ VIN panning
#define NR51            0xFF25 // Sound Panning
#define NR52            0xFF26 // Audio Master Control

#define WAVE_RAM        0xFF30

#define LCDC            0xFF40
#define STAT            0xFF41
#define SCY             0xFF42
#define SCX             0xFF43
#define LY              0xFF44
#define LYC             0xFF45
#define DMA             0xFF46
#define BGP             0xFF47
#define OBP0            0xFF48
#define OBP1            0xFF49
#define WY              0xFF4A
#define WX              0xFF4B

#define BOOT            0xFF50

#define IE              0xFFFF

// CGB-only registers
#define KEY1            0xFF4D
#define VBK             0xFF4F
#define HDMA1           0xFF51
#define HDMA2           0xFF52
#define HDMA3           0xFF53
#define HDMA4           0xFF54
#define HDMA5           0xFF55
#define RP              0xFF56
#define BCPS            0xFF68
#define BCPD            0xFF69
#define OCPS            0xFF6A
#define OCPD            0xFF6B
#define SVBK            0xFF70
#define PCM12           0xFF76
#define PCM34           0xFF77

// clang-format on
#endif

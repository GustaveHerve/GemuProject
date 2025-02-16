#ifndef CORE_COMMON_H
#define CORE_COMMON_H
// clang-format off

#include "stdint.h"

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
#define KEY0            0xFF4C
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
#define OPRI            0xFF6C
#define SVBK            0xFF70
#define PCM12           0xFF76
#define UNK1            0xFF72
#define UNK2            0xFF73
#define UNK3            0xFF74
#define UNK4            0xFF75
#define PCM34           0xFF77


#define _IO(ADDR)     IO_OFFSET(ADDR)
#define W(N)         WAVE_RAM + (N)

// clang-format on
static inline uint8_t io_read(uint8_t *io, uint8_t address)
{
    // These masks are only correct for DMG !
    static uint8_t masks[] = {
        [_IO(JOYP)] = 0x3F,  [_IO(SB)] = 0xFF,    [_IO(SC)] = 0x83,    [_IO(DIV)] = 0xFF,   [_IO(TIMA)] = 0xFF,
        [_IO(TMA)] = 0xFF,   [_IO(TAC)] = 0x07,   [_IO(IF)] = 0x1F,    [_IO(NR10)] = 0x7F,  [_IO(NR11)] = 0xFF,
        [_IO(NR12)] = 0xFF,  [_IO(NR13)] = 0xFF,  [_IO(NR14)] = 0xC7,  [_IO(NR21)] = 0xFF,  [_IO(NR22)] = 0xFF,
        [_IO(NR23)] = 0xFF,  [_IO(NR24)] = 0xC7,  [_IO(NR30)] = 0x80,  [_IO(NR31)] = 0xFF,  [_IO(NR32)] = 0x60,
        [_IO(NR33)] = 0xFF,  [_IO(NR34)] = 0xC7,  [_IO(NR41)] = 0x3F,  [_IO(NR42)] = 0xFF,  [_IO(NR43)] = 0xFF,
        [_IO(NR44)] = 0xC0,  [_IO(NR50)] = 0xFF,  [_IO(NR51)] = 0xFF,  [_IO(NR52)] = 0x8F,  [_IO(W(0))] = 0xFF,
        [_IO(W(1))] = 0xFF,  [_IO(W(2))] = 0xFF,  [_IO(W(3))] = 0xFF,  [_IO(W(4))] = 0xFF,  [_IO(W(5))] = 0xFF,
        [_IO(W(6))] = 0xFF,  [_IO(W(7))] = 0xFF,  [_IO(W(8))] = 0xFF,  [_IO(W(9))] = 0xFF,  [_IO(W(10))] = 0xFF,
        [_IO(W(11))] = 0xFF, [_IO(W(12))] = 0xFF, [_IO(W(13))] = 0xFF, [_IO(W(14))] = 0xFF, [_IO(W(15))] = 0xFF,
        [_IO(LCDC)] = 0xFF,  [_IO(STAT)] = 0x7F,  [_IO(SCY)] = 0xFF,   [_IO(SCX)] = 0xFF,   [_IO(LY)] = 0xFF,
        [_IO(LYC)] = 0xFF,   [_IO(DMA)] = 0xFF,   [_IO(BGP)] = 0xFF,   [_IO(OBP0)] = 0xFF,  [_IO(OBP1)] = 0xFF,
        [_IO(WY)] = 0xFF,    [_IO(WX)] = 0xFF,    [_IO(KEY0)] = 0,     [_IO(KEY1)] = 0,     [_IO(VBK)] = 0,
        [_IO(BOOT)] = 0xFE,  [_IO(HDMA1)] = 0,    [_IO(HDMA2)] = 0,    [_IO(HDMA3)] = 0,    [_IO(HDMA4)] = 0,
        [_IO(HDMA5)] = 0,    [_IO(RP)] = 0,       [_IO(BCPS)] = 0,     [_IO(BCPD)] = 0,     [_IO(OCPS)] = 0,
        [_IO(OCPD)] = 0,     [_IO(OPRI)] = 0,     [_IO(SVBK)] = 0,     [_IO(UNK1)] = 0,     [_IO(UNK2)] = 0,
        [_IO(UNK3)] = 0,     [_IO(UNK4)] = 0,     [_IO(PCM12)] = 0,    [_IO(PCM34)] = 0,
    };
    return io[IO_OFFSET(address)] & masks[IO_OFFSET(address)];
}

#undef W
#undef _IO

#endif

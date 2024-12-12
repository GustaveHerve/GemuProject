#ifndef MBC_BASE_H
#define MBC_BASE_H

#include <stdint.h>
#include <stdio.h>

struct cpu;

enum MBC_TYPE
{
    NO_MBC = 0,
    MBC1,
    MBC3 = 3,
};

struct mbc_base
{
    enum MBC_TYPE type;

    char *rom_path;
    FILE *save_file;

    uint8_t *rom;
    uint8_t *ram;

    uint8_t rom_size;
    uint8_t ram_size;

    uint16_t rom_bank_count;
    uint8_t ram_bank_count;

    /* Functions pointers */
    void (*_mbc_init)(struct mbc_base *mbc_base);
    void (*_mbc_free)(struct mbc_base *mbc_base);

    uint8_t (*_read_mbc_rom)(struct cpu *cpu, uint16_t address);
    void (*_write_mbc_rom)(struct cpu *cpu, uint16_t address, uint8_t val);

    uint8_t (*_read_mbc_ram)(struct cpu *cpu, uint16_t address);
    void (*_write_mbc_ram)(struct cpu *cpu, uint16_t address, uint8_t val);

    void (*_write_mbc)(struct cpu *cpu, uint16_t address, uint8_t val);
};

#endif

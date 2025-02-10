#ifndef MBC_BASE_H
#define MBC_BASE_H

#include <stdint.h>
#include <stdio.h>

struct cpu;

enum MBC_TYPE
{
    NO_MBC = 0,
    MBC1,
    MBC2,
    MBC3,
    MBC5,
};

struct mbc_base
{
    enum MBC_TYPE type;

    char *rom_path;
    FILE *save_file;

    uint8_t *rom;
    uint8_t *ram;

    uint8_t rom_size_header;
    uint8_t ram_size_header;

    uint16_t rom_bank_count;
    uint8_t ram_bank_count;

    unsigned int rom_total_size;
    unsigned int ram_total_size;

    /* Functions pointers */
    void (*_mbc_free)(struct mbc_base *mbc_base);

    uint8_t (*_read_mbc_rom)(struct mbc_base *mbc, uint16_t address);
    void (*_write_mbc_rom)(struct mbc_base *mbc, uint16_t address, uint8_t val);

    uint8_t (*_read_mbc_ram)(struct mbc_base *mbc, uint16_t address);
    void (*_write_mbc_ram)(struct mbc_base *mbc, uint16_t address, uint8_t val);
};

void mbc_free(struct mbc_base *mbc);
void set_mbc(struct mbc_base **output, uint8_t *rom, char *rom_path);

uint8_t read_mbc_rom(struct mbc_base *mbc, uint16_t address);
void write_mbc_rom(struct mbc_base *mbc, uint16_t address, uint8_t val);

uint8_t read_mbc_ram(struct mbc_base *mbc, uint16_t address);
void write_mbc_ram(struct mbc_base *mbc, uint16_t address, uint8_t val);

#endif

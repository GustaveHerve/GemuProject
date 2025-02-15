#ifndef CORE_MBC_BASE_H
#define CORE_MBC_BASE_H

#include <stdint.h>
#include <stdio.h>

struct cpu;

#define MBC_SET_VTABLE                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        mbc->_mbc_reset = _mbc_reset;                                                                                  \
        mbc->_mbc_free = _mbc_free;                                                                                    \
        mbc->_read_mbc_rom = _read_mbc_rom;                                                                            \
        mbc->_write_mbc_rom = _write_mbc_rom;                                                                          \
        mbc->_read_mbc_ram = _read_mbc_ram;                                                                            \
        mbc->_write_mbc_ram = _write_mbc_ram;                                                                          \
        mbc->_mbc_serialize = _mbc_serialize;                                                                          \
        mbc->_mbc_load_from_stream = _mbc_load_from_stream;                                                            \
    } while (0)

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
    void (*_mbc_reset)(struct mbc_base *mbc_base);
    void (*_mbc_free)(struct mbc_base *mbc_base);

    uint8_t (*_read_mbc_rom)(struct mbc_base *mbc, uint16_t address);
    void (*_write_mbc_rom)(struct mbc_base *mbc, uint16_t address, uint8_t val);

    uint8_t (*_read_mbc_ram)(struct mbc_base *mbc, uint16_t address);
    void (*_write_mbc_ram)(struct mbc_base *mbc, uint16_t address, uint8_t val);

    void (*_mbc_serialize)(struct mbc_base *mbc, FILE *stream);
    void (*_mbc_load_from_stream)(struct mbc_base *mbc, FILE *stream);
};

void set_mbc(struct mbc_base **output, uint8_t *rom, char *rom_path);

void mbc_reset(struct mbc_base *mbc);
void mbc_free(struct mbc_base *mbc);

uint8_t read_mbc_rom(struct mbc_base *mbc, uint16_t address);
void write_mbc_rom(struct mbc_base *mbc, uint16_t address, uint8_t val);

uint8_t read_mbc_ram(struct mbc_base *mbc, uint16_t address);
void write_mbc_ram(struct mbc_base *mbc, uint16_t address, uint8_t val);

void mbc_serialize(struct mbc_base *mbc, FILE *stream);
void mbc_load_from_stream(struct mbc_base *mbc, FILE *stream);

#endif

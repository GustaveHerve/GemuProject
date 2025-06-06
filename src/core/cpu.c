#include "cpu.h"

#include <stdio.h>

#include "emulation.h"
#include "serialization.h"
#include "utils.h"

void cpu_set_registers_post_boot(struct cpu *cpu, int checksum)
{
    cpu->a = 0x01;
    set_z(cpu, 1);
    set_n(cpu, 0);
    if (checksum == 0x00)
    {
        set_h(cpu, 0);
        set_c(cpu, 0);
    }
    else
    {
        set_h(cpu, 1);
        set_c(cpu, 1);
    }
    cpu->b = 0x00;
    cpu->c = 0x13;
    cpu->d = 0x00;
    cpu->e = 0xD8;
    cpu->h = 0x01;
    cpu->l = 0x4D;
    cpu->pc = 0x0100;
    cpu->sp = 0xFFFE;

    cpu->ime = 0;
}

void serialize_cpu_to_stream(FILE *stream, struct cpu *cpu)
{
    fwrite(&cpu->a, sizeof(uint8_t), 8, stream);

    fwrite_le_16(stream, cpu->sp);
    fwrite_le_16(stream, cpu->pc);

    fwrite(&cpu->ime, sizeof(uint8_t), 1, stream);
}

void load_cpu_from_stream(FILE *stream, struct cpu *cpu)
{
    fread(&cpu->a, sizeof(uint8_t), 8, stream);

    fread_le_16(stream, &cpu->sp);
    fread_le_16(stream, &cpu->pc);

    fread(&cpu->ime, sizeof(uint8_t), 1, stream);
}

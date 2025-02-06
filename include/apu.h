#ifndef APU_H
#define APU_H

#include <SDL3/SDL_audio.h>
#include <stdint.h>

#include "cpu.h"

#define SAMPLING_RATE 44100
#define SAMPLING_TCYCLES_INTERVAL (CPU_FREQUENCY / SAMPLING_RATE)
#define AUDIO_BUFFER_SIZE 512

/* Default GB audio samples are WAY too loud by default, scale them back */
#define EMULATOR_SOUND_VOLUME 0.4f;

/* Global control registers */
#define NR50 0xFF24 // Master volume ^ VIN panning
#define NR51 0xFF25 // Sound Panning
#define NR52 0xFF26 // Audio Master Control

/* Sound Channel 1 - Pulse with period sweep */
#define NR10 0xFF10
#define NR11 0xFF11
#define NR12 0xFF12
#define NR13 0xFF13
#define NR14 0xFF14

/* Sound Channel 2 - Pulse */
#define NR21 0xFF16
#define NR22 0xFF17
#define NR23 0xFF18
#define NR24 0xFF19

/* Sound Channel 3 - Wave output */
#define NR30 0xFF1A
#define NR31 0xFF1B
#define NR32 0xFF1C
#define NR33 0xFF1D
#define NR34 0xFF1E

/* Sound Channel 4 - Noise */
#define NR41 0xFF20
#define NR42 0xFF21
#define NR43 0xFF22
#define NR44 0xFF23

#define NRx4_TRIGGER_MASK (1 << 7)
#define NRx4_LENGTH_ENABLE (1 << 6)
#define NRx4_UNUSED_PART (0x7 << 3)
#define NRx4_PERIOD (0x7 << 0)

union audio_sample
{
    struct
    {
        float left_sample;
        float right_sample;
    } stereo_sample;
    uint64_t full_sample_packed;
};

struct apu
{
    struct cpu *cpu;

    struct ch1 *ch1;
    struct ch2 *ch2;
    struct ch3 *ch3;
    struct ch4 *ch4;

    uint8_t fs_pos;

    unsigned int sampling_counter;

    SDL_AudioStream *audio_stream;

    uint16_t previous_div;
};

struct ch1
{
    /* Length */
    unsigned int length_timer;

    /* Envelope */
    unsigned int period_timer;
    unsigned int current_volume;
    unsigned int env_dir;
    unsigned int env_period;

    unsigned int frequency_timer;
    unsigned int duty_pos;

    /* Sweep */
    unsigned int sweep_enabled;
    unsigned int shadow_frequency;
    unsigned int sweep_timer;
};

struct ch2
{
    /* Length */
    unsigned int length_timer;

    /* Envelope */
    unsigned int period_timer;
    unsigned int current_volume;
    unsigned int env_dir;
    unsigned int env_period;

    unsigned int frequency_timer;
    unsigned int duty_pos;
};

struct ch3
{
    /* Length */
    unsigned int length_timer;

    unsigned int frequency_timer;

    unsigned int wave_pos;
    unsigned int sample_buffer;
};

struct ch4
{
    /* Length */
    unsigned int length_timer;

    /* Envelope */
    unsigned int period_timer;
    unsigned int current_volume;
    unsigned int env_dir;
    unsigned int env_period;

    unsigned int frequency_timer;

    unsigned int lfsr;
    unsigned int polynomial_counter;
};

void apu_init(struct cpu *cpu, struct apu *apu);

void apu_free(struct apu *apu);

void handle_trigger_event_ch1(struct apu *apu);
void handle_trigger_event_ch2(struct apu *apu);
void handle_trigger_event_ch3(struct apu *apu);
void handle_trigger_event_ch4(struct apu *apu);

void enable_timer(struct apu *apu, uint8_t ch_number);

void apu_tick_m(struct apu *apu);

static inline uint8_t is_apu_on(struct apu *apu)
{
    return apu->cpu->membus[NR52] >> 7;
}

static inline uint8_t is_dac_on(struct apu *apu, uint8_t number)
{
    if (number == 3)
        return apu->cpu->membus[NR30] >> 7;
    unsigned int nrx2 = NR12 + ((NR22 - NR12) * (number - 1));
    return apu->cpu->membus[nrx2] & 0xF8;
}

static inline uint8_t is_channel_on(struct apu *apu, uint8_t number)
{
    return (apu->cpu->membus[NR52] >> (number - 1)) & 0x01;
}

static inline void turn_channel_off(struct apu *apu, uint8_t number)
{
    apu->cpu->membus[NR52] &= ~(1 << (number - 1));
}

static inline void turn_channel_on(struct apu *apu, uint8_t number)
{
    apu->cpu->membus[NR52] |= 1 << (number - 1);
}

static inline float dac_output(unsigned int amplitude)
{
    return (amplitude / 7.5f) - 1.0f;
}

#endif

#ifndef CORE_APU_H
#define CORE_APU_H

#include <stdint.h>

struct gb_core;

#define SAMPLING_RATE 44100
#define AUDIO_BUFFER_SIZE 512

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

struct apu
{
    struct ch1 ch1;
    struct ch2 ch2;
    struct ch3 ch3;
    struct ch4 ch4;

    uint8_t fs_pos;

    unsigned int sampling_counter;

    uint16_t previous_div_apu;
};

void apu_init(struct apu *apu);

void handle_trigger_event_ch1(struct gb_core *gb);
void handle_trigger_event_ch2(struct gb_core *gb);
void handle_trigger_event_ch3(struct gb_core *gb);
void handle_trigger_event_ch4(struct gb_core *gb);

void enable_timer(struct gb_core *gb, uint8_t ch_number);

void apu_tick_m(struct gb_core *gb);

static inline float dac_output(unsigned int amplitude)
{
    return (amplitude / 7.5f) - 1.0f;
}

#endif

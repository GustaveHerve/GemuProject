#ifndef CORE_APU_H
#define CORE_APU_H

#include <stdint.h>
#include <stdio.h>

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
    uint32_t trigger_request;

    /* Length */
    uint32_t length_timer; // 8 bits

    /* Envelope */
    uint32_t period_timer;   // 3 bits
    uint32_t current_volume; // 4 bits
    uint32_t env_dir;        // 1 bit
    uint32_t env_period;     // 3 bits

    uint32_t frequency_timer; // 16 bits
    uint32_t duty_pos;        // 8 bits

    /* Sweep */
    uint32_t sweep_enabled;    // 1 bit
    uint32_t shadow_frequency; // 16 bits
    uint32_t sweep_timer;      // 3 bits
    uint8_t neg_calc;
};

struct ch2
{
    uint32_t trigger_request;

    /* Length */
    uint32_t length_timer;

    /* Envelope */
    uint32_t period_timer;
    uint32_t current_volume;
    uint32_t env_dir;
    uint32_t env_period;

    uint32_t frequency_timer;
    uint32_t duty_pos;
};

struct ch3
{
    uint32_t trigger_request;

    /* Length */
    uint32_t length_timer;

    uint32_t frequency_timer;

    uint32_t wave_pos;
    uint32_t sample_buffer;

    uint32_t phantom_sample;
};

struct ch4
{
    uint32_t trigger_request;

    /* Length */
    uint32_t length_timer;

    /* Envelope */
    uint32_t period_timer;
    uint32_t current_volume;
    uint32_t env_dir;
    uint32_t env_period;

    uint32_t frequency_timer;

    uint32_t lfsr;
    uint32_t polynomial_counter;
};

struct apu
{
    struct ch1 ch1;
    struct ch2 ch2;
    struct ch3 ch3;
    struct ch4 ch4;

    uint8_t fs_pos;

    uint32_t sampling_counter;

    uint16_t previous_div_apu;
};

void apu_init(struct apu *apu);

void handle_trigger_event_ch1(struct gb_core *gb);
void handle_trigger_event_ch2(struct gb_core *gb);
void handle_trigger_event_ch3(struct gb_core *gb);
void handle_trigger_event_ch4(struct gb_core *gb);

void apu_reload_timer(struct gb_core *gb, uint8_t ch_number);

void apu_tick(struct gb_core *gb);

void apu_turn_off(struct gb_core *gb);

void apu_write_reg(struct gb_core *gb, uint16_t address, uint8_t val);

void serialize_apu_to_stream(FILE *stream, struct apu *apu);

void load_apu_from_stream(FILE *stream, struct apu *apu);

static inline float dac_output(uint32_t amplitude)
{
    return (amplitude / 7.5f) - 1.0f;
}

#endif

#include "apu.h"

#include <stdlib.h>

#include "gb_core.h"

// clang-format off
static unsigned int duty_table[][8] = {
    { 0, 0, 0, 0, 0, 0, 0, 1, },
    { 0, 0, 0, 0, 0, 0, 1, 1, },
    { 0, 0, 0, 0, 1, 1, 1, 1, },
    { 1, 1, 1, 1, 1, 1, 0, 0, },
};
// clang-format on

static unsigned int ch3_shifts[] = {
    4,
    0,
    1,
    2,
};

static unsigned int ch4_divisors[] = {
    8,
    16,
    32,
    48,
    64,
    80,
    96,
    112,
};

#define NRXY(X, Y) (NR1##Y + ((NR2##Y - NR1##Y) * ((X) - 1)))

#define FREQUENCY(CH_NUMBER) ((gb->membus[NR##CH_NUMBER##4] & 0x07) << 8 | gb->membus[NR##CH_NUMBER##3])

#define DIV_APU_MASK (1 << (4 + 8))

#define WAVE_DUTY(NRX1) ((NRX1) >> 6)

#define ENVELOPE_DIR_DECREMENT 0
#define ENVELOPE_DIR_INCREMENT 1

#define ENV_SWEEP_PERIOD(NRX2) ((NRX2) & 0x7)
#define ENV_DIR(NRX2) (((NRX2) >> 3) & 0x1)
#define ENV_INIT_VOLUME(NRX2) (((NRX2) >> 4) & 0xF)

#define SWEEP_SHIFT(NR10) ((NR10) & 0x7)
#define SWEEP_DIR(NR10) (((NR10) >> 3) & 0x1)
#define SWEEP_PERIOD(NR10) (((NR10) >> 4) & 0x7)

#define SWEEP_DIR_INCREMENT 0
#define SWEEP_DIR_DECREMENT 1

#define WAVE_RAM 0xFF30
#define WAVE_OUTPUT(NR32) (((NR32) >> 5) & 0x3)

#define NOISE_CLOCK_DIVIDER_CODE(NR43) ((NR43) & 0x7)
#define NOISE_LFSR_WIDTH(NR43) (((NR43) >> 3) & 0x1)
#define NOISE_CLOCK_SHIFT(NR43) (((NR43) >> 4) & 0xF)

#define PANNING_LEFT 0
#define PANNING_RIGHT 1

#define LEFT_MASTER_VOLUME(NR50) (((NR50) >> 4) & 0x7)
#define RIGHT_MASTER_VOLUME(NR50) ((NR50) & 0x7)

#define LENGTH_ENABLE(NRX4) (((NRX4) >> 6) & 0x1)

static union audio_sample audio_buffer[AUDIO_BUFFER_SIZE];
static size_t audio_buffer_len = 0;

struct ch_generic
{
    unsigned int length_timer;

    unsigned int period_timer;
    unsigned int current_volume;
    unsigned int env_dir;
    unsigned int env_period;
};

void apu_init(struct apu *apu)
{
    memset(apu, 0, sizeof(struct apu));
    memset(&apu->ch1, 0, sizeof(struct ch1));
    memset(&apu->ch2, 0, sizeof(struct ch2));
    memset(&apu->ch3, 0, sizeof(struct ch3));
    memset(&apu->ch4, 0, sizeof(struct ch4));
    apu->fs_pos = 0;
    apu->sampling_counter = 0;
    apu->previous_div_apu = 0;
}

static void length_trigger(struct apu *apu, struct ch_generic *ch)
{
    if ((void *)ch == (void *)&apu->ch3 && ch->length_timer == 0)
        ch->length_timer = 256;
    else if (ch->length_timer == 0)
        ch->length_timer = 64;
}

static unsigned int calculate_frequency(struct gb_core *gb, uint8_t sweep_shift, uint8_t dir);

static void frequency_sweep_trigger(struct gb_core *gb, struct ch1 *ch1)
{
    uint8_t nr10 = gb->membus[NR10];
    unsigned int sweep_shift = SWEEP_SHIFT(nr10);
    unsigned int sweep_period = SWEEP_PERIOD(nr10);

    ch1->shadow_frequency = FREQUENCY(1);
    ch1->sweep_timer = sweep_period;

    if (ch1->sweep_timer == 0)
        ch1->sweep_timer = 8;

    if (sweep_period != 0 || sweep_shift != 0)
        ch1->sweep_enabled = 1;
    else
        ch1->sweep_enabled = 0;

    if (sweep_shift)
        calculate_frequency(gb, sweep_shift, SWEEP_DIR(nr10));
}

static void envelope_trigger(struct gb_core *gb, struct ch_generic *ch, uint8_t ch_number)
{
    uint8_t nrx2 = gb->membus[NRXY(ch_number, 2)];
    ch->current_volume = ENV_INIT_VOLUME(nrx2);
    ch->env_dir = ENV_DIR(nrx2);
    ch->env_period = ENV_SWEEP_PERIOD(nrx2);
    ch->period_timer = ch->env_period;
}

void handle_trigger_event_ch1(struct gb_core *gb)
{
    length_trigger(&gb->apu, (void *)&gb->apu.ch1);
    gb->apu.ch1.frequency_timer = (2048 - FREQUENCY(1)) * 4;
    envelope_trigger(gb, (void *)&gb->apu.ch1, 1);
    frequency_sweep_trigger(gb, &gb->apu.ch1);

    if (is_dac_on(gb, 1))
        turn_channel_on(gb, 1);
    else
        turn_channel_off(gb, 1);
}

void handle_trigger_event_ch2(struct gb_core *gb)
{
    length_trigger(&gb->apu, (void *)&gb->apu.ch2);
    gb->apu.ch2.frequency_timer = (2048 - FREQUENCY(2)) * 4;
    envelope_trigger(gb, (void *)&gb->apu.ch2, 2);

    if (is_dac_on(gb, 2))
        turn_channel_on(gb, 2);
    else
        turn_channel_off(gb, 2);
}

void handle_trigger_event_ch3(struct gb_core *gb)
{
    length_trigger(&gb->apu, (void *)&gb->apu.ch3);
    gb->apu.ch3.frequency_timer = (2048 - FREQUENCY(3)) * 2;
    gb->apu.ch3.wave_pos = 0;

    if (is_dac_on(gb, 3))
        turn_channel_on(gb, 3);
    else
        turn_channel_off(gb, 3);
}

void handle_trigger_event_ch4(struct gb_core *gb)
{
    length_trigger(&gb->apu, (void *)&gb->apu.ch4);
    uint8_t nr43 = gb->membus[NR43];
    unsigned int shift = NOISE_CLOCK_SHIFT(nr43);
    unsigned int divisor_code = NOISE_CLOCK_DIVIDER_CODE(nr43);
    gb->apu.ch4.frequency_timer = ch4_divisors[divisor_code] << shift;
    envelope_trigger(gb, (void *)&gb->apu.ch4, 4);

    gb->apu.ch4.lfsr = 0x7FFF;
    if (is_dac_on(gb, 4))
        turn_channel_on(gb, 4);
    else
        turn_channel_off(gb, 4);
}

void enable_timer(struct gb_core *gb, uint8_t ch_number)
{
    struct ch_generic *ch = NULL;
    unsigned int val = 64;
    unsigned int init_len_mask = 0x3F;
    switch (ch_number)
    {
    case 1:
        ch = (void *)&gb->apu.ch1;
        break;
    case 2:
        ch = (void *)&gb->apu.ch2;
        break;
    case 3:
        ch = (void *)&gb->apu.ch3;
        val = 256;
        init_len_mask = 0xFF;
        break;
    case 4:
        ch = (void *)&gb->apu.ch4;
        break;
    }
    unsigned int initial_length = gb->membus[NRXY(ch_number, 1)] & init_len_mask;

    ch->length_timer = val - initial_length;
}

static void length_clock(struct gb_core *gb, struct ch_generic *ch, uint8_t number)
{
    uint8_t nrx4 = gb->membus[NRXY(number, 4)];
    if (!LENGTH_ENABLE(nrx4))
        return;

    --ch->length_timer;
    if (ch->length_timer == 0)
        turn_channel_off(gb, number);
}

static void volume_env_clock(struct gb_core *gb, struct ch_generic *ch, uint8_t number)
{
    if (!is_channel_on(gb, number) || !is_dac_on(gb, number))
        return;

    if (ch->env_period == 0)
        return;

    if (ch->period_timer > 0)
        --ch->period_timer;

    if (ch->period_timer == 0)
    {
        ch->period_timer = ch->env_period;

        if (ch->env_dir == ENVELOPE_DIR_INCREMENT && ch->current_volume < 0xF)
            ++ch->current_volume;
        else if (ch->env_dir == ENVELOPE_DIR_DECREMENT && ch->current_volume > 0)
            --ch->current_volume;
    }
}

static unsigned int calculate_frequency(struct gb_core *gb, uint8_t sweep_shift, uint8_t dir)
{
    struct ch1 *ch1 = &gb->apu.ch1;
    unsigned int new_frequency = ch1->shadow_frequency >> sweep_shift;

    if (dir == SWEEP_DIR_DECREMENT)
        new_frequency = ch1->shadow_frequency - new_frequency;
    else
        new_frequency = ch1->shadow_frequency + new_frequency;

    if (new_frequency > 2047)
        turn_channel_off(gb, 1);

    return new_frequency;
}

static void frequency_sweep_clock(struct gb_core *gb)
{
    struct ch1 *ch1 = &gb->apu.ch1;

    uint8_t nr10 = gb->membus[NR10];

    uint8_t period = SWEEP_PERIOD(nr10);
    uint8_t dir = SWEEP_DIR(nr10);
    uint8_t shift = SWEEP_SHIFT(nr10);

    if (ch1->sweep_timer > 0)
        --ch1->sweep_timer;

    if (ch1->sweep_timer != 0)
        return;

    if (period == 0)
        ch1->sweep_timer = 8;
    else
        ch1->sweep_timer = period;

    if (ch1->sweep_enabled && period != 0)
    {
        unsigned int new_frequency = calculate_frequency(gb, shift, dir);
        if (new_frequency < 2048 && shift != 0)
        {
            gb->membus[NR13] = new_frequency & 0xFF;
            gb->membus[NR14] &= ~(0x07);
            gb->membus[NR14] |= (new_frequency >> 8);
            ch1->shadow_frequency = new_frequency;

            calculate_frequency(gb, shift, dir);
        }
    }
}

static void frame_sequencer_step(struct gb_core *gb)
{
    /* Length Counter tick */
    if (gb->apu.fs_pos % 2 == 0)
    {
        length_clock(gb, (struct ch_generic *)&gb->apu.ch1, 1);
        length_clock(gb, (struct ch_generic *)&gb->apu.ch2, 2);
        length_clock(gb, (struct ch_generic *)&gb->apu.ch3, 3);
        length_clock(gb, (struct ch_generic *)&gb->apu.ch4, 4);
    }

    /* Volume Envelope tick */
    if (gb->apu.fs_pos == 7)
    {
        // CH3 doesn't have an envelope functionality
        volume_env_clock(gb, (struct ch_generic *)&gb->apu.ch1, 1);
        volume_env_clock(gb, (struct ch_generic *)&gb->apu.ch2, 2);
        volume_env_clock(gb, (struct ch_generic *)&gb->apu.ch4, 4);
    }

    /* Frequency Sweep tick */
    if (gb->apu.fs_pos == 2 || gb->apu.fs_pos == 6)
    {
        // Only CH1 has a sweep functionality
        frequency_sweep_clock(gb);
    }

    gb->apu.fs_pos = (gb->apu.fs_pos + 1) % 8;
}

static void ch1_tick(struct gb_core *gb)
{
    if (!is_channel_on(gb, 1) || !is_dac_on(gb, 1))
        return;

    gb->apu.ch1.frequency_timer -= 1;
    if (gb->apu.ch1.frequency_timer == 0)
    {
        gb->apu.ch1.frequency_timer = (2048 - FREQUENCY(1)) * 4;
        gb->apu.ch1.duty_pos = (gb->apu.ch1.duty_pos + 1) % 8;
    }
}

static void ch2_tick(struct gb_core *gb)
{
    if (!is_channel_on(gb, 2) || !is_dac_on(gb, 2))
        return;

    gb->apu.ch2.frequency_timer -= 1;
    if (gb->apu.ch2.frequency_timer == 0)
    {
        gb->apu.ch2.frequency_timer = (2048 - FREQUENCY(2)) * 4;
        gb->apu.ch2.duty_pos = (gb->apu.ch2.duty_pos + 1) % 8;
    }
}

static void ch3_tick(struct gb_core *gb)
{
    if (!is_channel_on(gb, 3) || !is_dac_on(gb, 3))
        return;

    gb->apu.ch3.frequency_timer -= 1;
    if (gb->apu.ch3.frequency_timer == 0)
    {
        gb->apu.ch3.frequency_timer = (2048 - FREQUENCY(3)) * 2;
        gb->apu.ch3.wave_pos = (gb->apu.ch3.wave_pos + 1) % 32;

        unsigned int sample = gb->membus[WAVE_RAM + (gb->apu.ch3.wave_pos / 2)];
        // Each byte has two 4-bit samples
        if (gb->apu.ch3.wave_pos % 2 == 0)
            sample >>= 4;
        else
            sample &= 0x0F;

        gb->apu.ch3.sample_buffer = sample;
    }
}

static void ch4_tick(struct gb_core *gb)
{
    if (!is_channel_on(gb, 4) || !is_dac_on(gb, 4))
        return;

    uint8_t nr43 = gb->membus[NR43];
    unsigned int shift = NOISE_CLOCK_SHIFT(nr43);
    unsigned int divisor_code = NOISE_CLOCK_DIVIDER_CODE(nr43);

    gb->apu.ch4.frequency_timer -= 1;
    if (gb->apu.ch4.frequency_timer == 0)
    {
        gb->apu.ch4.frequency_timer = ch4_divisors[divisor_code] << shift;

        uint8_t xor_res = ((gb->apu.ch4.lfsr >> 1) & 0x01) ^ (gb->apu.ch4.lfsr & 0x01);
        gb->apu.ch4.lfsr = (gb->apu.ch4.lfsr >> 1) | (xor_res << 14);

        if (NOISE_LFSR_WIDTH(nr43))
            gb->apu.ch4.lfsr = (gb->apu.ch4.lfsr & ~(1 << 6)) | (xor_res << 6);
    }
}

static unsigned int get_channel_amplitude(struct gb_core *gb, uint8_t number, uint8_t panning)
{
    if (!is_dac_on(gb, number) || !is_channel_on(gb, number))
        return 0;

    unsigned int panning_mask = 1 << (number - 1);
    if (panning == PANNING_LEFT)
        panning_mask <<= 4;

    if (!(gb->membus[NR51] & panning_mask))
        return 0;

    if (number == 1)
    {
        unsigned int wave_duty = WAVE_DUTY(gb->membus[NR11]);
        unsigned int duty_pos = gb->apu.ch1.duty_pos;
        return duty_table[wave_duty][duty_pos] * gb->apu.ch1.current_volume;
    }

    if (number == 2)
    {
        unsigned int wave_duty = WAVE_DUTY(gb->membus[NR21]);
        unsigned int duty_pos = gb->apu.ch2.duty_pos;
        return duty_table[wave_duty][duty_pos] * gb->apu.ch2.current_volume;
    }

    if (number == 3)
    {
        unsigned int wave_output = WAVE_OUTPUT(gb->membus[NR32]);
        return gb->apu.ch3.sample_buffer >> ch3_shifts[wave_output];
    }

    return ((~gb->apu.ch4.lfsr) & 0x1) * gb->apu.ch4.current_volume;
}

static float mix_channels(struct gb_core *gb, uint8_t panning)
{
    float sum = 0.0f;
    for (size_t i = 1; i < 5; ++i)
    {
        sum += dac_output(get_channel_amplitude(gb, i, panning));
    }
    return sum / 4.0f;
}

static void queue_audio_sample(struct gb_core *gb)
{
    // If we have more than 0.125s of lag, skip this sample
    if (gb->callbacks.get_queued_audio_sample_count() > SAMPLING_RATE / 8)
        return;

    uint8_t nr50 = gb->membus[NR50];

    float left_sample = mix_channels(gb, PANNING_LEFT) * (float)LEFT_MASTER_VOLUME(nr50) / 8.0f * EMULATOR_SOUND_VOLUME;
    float right_sample =
        mix_channels(gb, PANNING_RIGHT) * (float)RIGHT_MASTER_VOLUME(nr50) / 8.0f * EMULATOR_SOUND_VOLUME;

    union audio_sample sample = {
        .stereo_sample =
            {
                .left_sample = left_sample,
                .right_sample = right_sample,
            },
    };

    audio_buffer[audio_buffer_len++] = sample;
    if (audio_buffer_len == AUDIO_BUFFER_SIZE)
    {
        gb->callbacks.queue_audio(audio_buffer);
        audio_buffer_len = 0;
    }
}

void apu_tick_m(struct gb_core *gb)
{
    if (!is_apu_on(gb))
        return;

    for (size_t i = 0; i < 4; ++i)
    {
        uint16_t div = gb->apu.previous_div_apu + 1;

        // DIV bit 4 falling edge detection
        if ((gb->apu.previous_div_apu & DIV_APU_MASK) && !(div & DIV_APU_MASK))
            frame_sequencer_step(gb);

        ch1_tick(gb);
        ch2_tick(gb);
        ch3_tick(gb);
        ch4_tick(gb);

        gb->apu.sampling_counter += SAMPLING_RATE;
        if (gb->apu.sampling_counter >= CPU_FREQUENCY)
        {
            queue_audio_sample(gb);
            gb->apu.sampling_counter -= CPU_FREQUENCY;
        }

        gb->apu.previous_div_apu += 1;
    }
}

#define _POSIX_C_SOURCE 199309L

#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include "cpu.h"
#include "emulation.h"

#define LCDC_PERIOD 70224
#define SECONDS_TO_NANOSECONDS 1000000000LL
#define MARGIN_OF_ERROR 200000000LL

int64_t get_nanoseconds(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_usec * 1000 + now.tv_sec * SECONDS_TO_NANOSECONDS;
}

void synchronize(struct cpu *cpu)
{
    if (get_global_settings()->turbo)
    {
        cpu->tcycles_since_sync = 0;
        return;
    }

    int64_t elapsed_ns = cpu->tcycles_since_sync * SECONDS_TO_NANOSECONDS / CPU_FREQUENCY;
    int64_t nanoseconds = get_nanoseconds();
    int64_t time_to_sleep = elapsed_ns + cpu->last_sync_timestamp - nanoseconds;
    if (time_to_sleep > 0 && time_to_sleep < LCDC_PERIOD * (SECONDS_TO_NANOSECONDS + MARGIN_OF_ERROR) / CPU_FREQUENCY)
    {
        struct timespec sleep = {0, time_to_sleep};
        nanosleep(&sleep, NULL);
        cpu->last_sync_timestamp += elapsed_ns;
    }
    else
    {
        // Emulation is late if time_to_sleep is negative
        if (time_to_sleep < 0 &&
            -time_to_sleep < LCDC_PERIOD * (SECONDS_TO_NANOSECONDS + MARGIN_OF_ERROR) / CPU_FREQUENCY)
        {
            // The difference is small enough to be negligible
            return;
        }
        cpu->last_sync_timestamp = nanoseconds;
    }

    cpu->tcycles_since_sync = 0;
}

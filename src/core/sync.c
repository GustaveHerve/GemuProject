#define _POSIX_C_SOURCE 199309L

#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include "emulation.h"
#include "gb_core.h"

#define LCDC_PERIOD 70224
#define SECONDS_TO_NANOSECONDS 1000000000LL
#define MARGIN_OF_ERROR 200000000LL

int64_t get_nanoseconds(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_usec * 1000 + now.tv_sec * SECONDS_TO_NANOSECONDS;
}

void synchronize(struct gb_core *gb)
{
    if (get_global_settings()->turbo)
    {
        /* Full-speed mode done by disabling synchronization */
        gb->tcycles_since_sync = 0;
        return;
    }

    int64_t elapsed_ns =
        gb->tcycles_since_sync * SECONDS_TO_NANOSECONDS / CPU_FREQUENCY; /* Represent elapsed emulated worth of time */
    int64_t nanoseconds = get_nanoseconds();                             /* Used to see the real life elapsed time */
    int64_t time_to_sleep =
        elapsed_ns + gb->last_sync_timestamp - nanoseconds; /* Compare the elapsed emulated to the real life elapsed */
    if (time_to_sleep > 0 && time_to_sleep < LCDC_PERIOD * (SECONDS_TO_NANOSECONDS + MARGIN_OF_ERROR) / CPU_FREQUENCY)
    {
        struct timespec sleep = {0, time_to_sleep};
        nanosleep(&sleep, NULL);
        gb->last_sync_timestamp += elapsed_ns;
    }
    else
    {
        /* Emulation is late if time_to_sleep is negative */
        if (time_to_sleep < 0 &&
            -time_to_sleep < LCDC_PERIOD * (SECONDS_TO_NANOSECONDS + MARGIN_OF_ERROR) / CPU_FREQUENCY)
        {
            /* In this case the difference is small enough to be negligible */
            return;
        }
        gb->last_sync_timestamp = nanoseconds;
    }

    gb->tcycles_since_sync = 0;
}

#define _POSIX_C_SOURCE 199309L

#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include "emulation.h"
#include "gb_core.h"

#define LCDC_PERIOD 70224
#define SECONDS_TO_NANOSECONDS 1000000000LL
#define MARGIN_OF_ERROR 1.20

int64_t get_nanoseconds(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_usec * 1000 + now.tv_sec * SECONDS_TO_NANOSECONDS;
}

/* This needs to be called often enough to ensure synchronization */
int64_t synchronize(struct gb_core *gb)
{
    if (get_global_settings()->turbo)
    {
        /* Full-speed mode done by disabling synchronization */
        gb->tcycles_since_sync = 0;
        return 0;
    }

    int64_t elapsed_ns =
        gb->tcycles_since_sync * SECONDS_TO_NANOSECONDS / CPU_FREQUENCY; /* Represent elapsed emulated worth of time */
    int64_t nanoseconds = get_nanoseconds();                             /* Used to see the real life elapsed time */
    int64_t time_to_sleep =
        elapsed_ns + gb->last_sync_timestamp - nanoseconds; /* Compare the elapsed emulated to the real life elapsed */

    /* Sleep only if time_to_sleep is positive and inferior to the interval of time between two frames (~16.7 ms) with
     * some margin of error. If the time_to_sleep is superior it means that we have done too much between two syncs */
    if (time_to_sleep > 0 && time_to_sleep < LCDC_PERIOD * (SECONDS_TO_NANOSECONDS * MARGIN_OF_ERROR) / CPU_FREQUENCY)
    {
        // struct timespec time_to_sleep_ts = {0, time_to_sleep};
        // nanosleep(&time_to_sleep_ts, NULL);
        gb->last_sync_timestamp += elapsed_ns;
    }
    else
    {
        /* Emulation is late if time_to_sleep is negative */
        if (time_to_sleep < 0 &&
            -time_to_sleep < LCDC_PERIOD * (SECONDS_TO_NANOSECONDS + MARGIN_OF_ERROR) / CPU_FREQUENCY)
        {
            /* In this case the deviation is small enough to be negligible */
            return 0;
        }
        gb->last_sync_timestamp = nanoseconds; /* If deviation is too big reset the last sync timestamp to now */
    }

    gb->tcycles_since_sync = 0;
    return time_to_sleep;
}

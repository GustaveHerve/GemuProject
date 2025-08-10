#include "audio.h"

#include <SDL3/SDL_audio.h>
#include <stdlib.h>

#include "apu.h"
#include "sdl_utils.h"

SDL_AudioStream *audio_stream;

int init_audio(void)
{
    SDL_AudioSpec audio_spec = {
        .format = SDL_AUDIO_F32,
        .channels = 2,
        .freq = SAMPLING_RATE,
    };

    SDL_CHECK_ERROR(
        (audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audio_spec, NULL, NULL)));
    SDL_CHECK_ERROR(SDL_ResumeAudioStreamDevice(audio_stream));

    return EXIT_SUCCESS;
}

int free_audio(void)
{
    if (audio_stream)
    {
        SDL_CHECK_ERROR(SDL_ClearAudioStream(audio_stream));
        SDL_CHECK_ERROR(SDL_PauseAudioStreamDevice(audio_stream));
        SDL_DestroyAudioStream(audio_stream);
    }
    return EXIT_SUCCESS;
}

int get_queued_sample_count(void)
{
    // TODO: what to do in case of error ?
    return SDL_GetAudioStreamQueued(audio_stream) / sizeof(union audio_sample);
}

int queue_audio(void *audio_buffer)
{
    // TODO: what to do in case of error ?
    return SDL_PutAudioStreamData(audio_stream, audio_buffer, AUDIO_BUFFER_SIZE * sizeof(union audio_sample));
}

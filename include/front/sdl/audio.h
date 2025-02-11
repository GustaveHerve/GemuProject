#ifndef SDL_AUDIO_H
#define SDL_AUDIO_H

#define SAMPLING_RATE 44100
#define AUDIO_BUFFER_SIZE 512

int init_audio(void);

int free_audio(void);

int get_queued_sample_count(void);

int queue_audio(void *audio_buffer);

#endif

#ifndef SDL_AUDIO_H
#define SDL_AUDIO_H

int init_audio(void);

int free_audio(void);

int get_queued_sample_count(void);

int queue_audio(void *audio_buffer);

#endif

/*
 *
 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com/
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifdef USE_SDL
#include <stdio.h>
#include <math.h>
#include "common.h"

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

#define TABLE_SIZE   (250)

#ifdef NO_SOUND
void SOUND_SetFrequency(unsigned freq, unsigned volume) {(void)freq; (void)volume;}
void SOUND_Init() {}
void SOUND_Start(unsigned msec, u16(*next_note_cb)()) {
    (void)msec;
    (void)next_note_cb;
    printf("beep\n");
}
void SOUND_StartWithoutVibrating(unsigned msec, u16(*next_note_cb)()) {
    (void)msec;
    (void)next_note_cb;
    printf("beep\n");
}
void SOUND_Stop() {}
#else

#include "SDL2/SDL.h"

static uint32_t sampleRate = 48000;
static uint32_t floatStreamLength = 2048;// must be a power of two, decrease to allow for a lower syncCompensationFactor to allow for lower latency, increase to reduce risk of underrun

static SDL_AudioDeviceID AudioDevice;
static SDL_AudioSpec audioSpec;

static u16(*next_note_cb)() = NULL;

static struct {
    //PaStreamParameters outputParameters;
    //PaStream *stream;

    float sine[TABLE_SIZE];
    int table_size;
    int phase;
    const int *ptr;
    int duration;
    int enable;
    char message[20];
} paData;

void SOUND_SetFrequency(unsigned freq, unsigned volume)
{
    int i;
    if (freq == 0) {
        paData.table_size = TABLE_SIZE;
        volume = 0;
    } else {
        paData.table_size = sampleRate / freq;
    }
    for(i = 0; i < paData.table_size; i++)
        paData.sine[i] = (float)volume / 100.0 * sin( ((double)i/(double)paData.table_size) * M_PI * 2. );
}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
void audioCallback(void *userData, uint8_t *outputBuffer, int byteStreamLength) {
    float *out = (float*)outputBuffer;
    int i;
    (void)userData;
    int sampleLength = byteStreamLength / sizeof(float) / 2;
    for (i=0; i<sampleLength; i++ )
    {
        *out++ = paData.sine[paData.phase];
        *out++ = paData.sine[paData.phase];
        paData.phase+=1;
        paData.duration--;
        if (! paData.duration) {
            u16 msec;
            if (next_note_cb == NULL || (msec = next_note_cb()) == 0) {
                while (++i < sampleLength) {
                    *out++ = 0;
                    *out++ = 0;
                }
                SOUND_Stop();
                return;
            }
            paData.duration = sampleRate * (long)msec / 1000;
            paData.phase = 0;
        }
        if (paData.phase >= paData.table_size) {
            paData.phase -= paData.table_size;
        }
    }
}

void SOUND_Init()
{
    static SDL_AudioSpec spec;

    spec.callback = audioCallback;
    spec.userdata = NULL;
    spec.freq     = sampleRate;
    spec.channels = 2;
    spec.samples  = floatStreamLength;
    spec.format   = AUDIO_F32;
    memset(&paData, 0, sizeof(paData));
    AudioDevice = SDL_OpenAudioDevice(NULL, 0, &spec, &audioSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

    if (AudioDevice == 0) {
        printf("\nFailed to open audio: %s\n", SDL_GetError());
        return;
    }
    if (audioSpec.format != spec.format) {
        printf("\nCouldn't get Float32 audio format.\n");
        return;
    }
    sampleRate = audioSpec.freq;
    paData.enable = 1;
}

void SOUND_StartWithoutVibrating(unsigned msec, u16(*_next_note_cb)())
{
    SOUND_Start(msec, _next_note_cb);
}

void SOUND_Start(unsigned msec, u16(*_next_note_cb)()) {
    if (! paData.enable)
        return;
    SOUND_Stop();
    next_note_cb = _next_note_cb;
    paData.duration = sampleRate * (long)msec / 1000;
    SDL_PauseAudioDevice(AudioDevice, 0);// unpause audio.
}

void SOUND_Stop()
{
    SDL_PauseAudioDevice(AudioDevice, 1);
}
#endif //NO_SOUND
#endif //USE_SDL

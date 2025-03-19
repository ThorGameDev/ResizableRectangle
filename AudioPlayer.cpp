#include "AudioPlayer.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <iostream>

struct SoundClip {
    Uint8* data;
    Uint32 data_len;
    SDL_AudioSpec spec;
    SDL_AudioStream* stream;
};

struct SoundData {
    SDL_AudioDeviceID audio_device;
};

static SoundClip* LoadSound(const char *path)
{
    SoundClip* sound = new SoundClip();
    char *full_path = NULL;

    /* Load the .wav files from wherever the app is being run from. */
    SDL_asprintf(&full_path, "%s%s", SDL_GetBasePath(), path);  /* allocate a string of the full file path */
    std::cout << "Loading " << full_path << std::endl;
    if (!SDL_LoadWAV(full_path, &sound->spec, &sound->data, &sound->data_len)) {
        SDL_Log("Couldn't load .wav file: %s", SDL_GetError());
        return nullptr;
    }

    sound->stream = SDL_CreateAudioStream(&sound->spec, NULL);
    if (!sound->stream) {
        SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
        return nullptr;
    } 

    return sound;
}

AudioPlayer::AudioPlayer()
{
    SDL_Init(SDL_INIT_AUDIO);
    std::cout << "Create AudioPlayer" << std::endl;
    song = LoadSound("resources/song.wav");
    SFX = LoadSound("resources/sound.wav");
    data = new SoundData();
    data->audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    playSong();
}

AudioPlayer::~AudioPlayer()
{
    SDL_CloseAudioDevice(data->audio_device);
    SDL_free(song->data);
    SDL_free(SFX->data);
    delete song;
    delete SFX;
    delete data;
    std::cout << "Destroyed AudioPlayer" << std::endl;
}

void AudioPlayer::playSound()
{
    SFX->stream = SDL_CreateAudioStream(&SFX->spec, NULL);
    if (!SFX->stream) {
        SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
    } 
    SDL_BindAudioStream(data->audio_device, SFX->stream);
    SDL_PutAudioStreamData(SFX->stream, SFX->data, (int) SFX->data_len);
}

void AudioPlayer::playSong()
{
    SDL_BindAudioStream(data->audio_device, song->stream);
    SDL_ResumeAudioDevice(data->audio_device);
}

void AudioPlayer::pauseSong()
{
    SDL_PauseAudioDevice(data->audio_device);
}

void AudioPlayer::checkRestart()
{
    if (SDL_GetAudioStreamQueued(song->stream) < ((int) song->data_len)) {
        SDL_PutAudioStreamData(song->stream, song->data, (int) song->data_len);
    }
}

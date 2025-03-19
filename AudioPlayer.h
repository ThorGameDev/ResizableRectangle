#ifndef AUDIO_PLAYER
#define AUDIO_PLAYER

class SoundClip;
class SoundData;

class AudioPlayer
{
    public:
        AudioPlayer();
        ~AudioPlayer();
        void playSound();
        void playSong();
        void pauseSong();
        void checkRestart();
    private:
        SoundClip* song;
        SoundClip* SFX;
        SoundData* data;
};

#endif

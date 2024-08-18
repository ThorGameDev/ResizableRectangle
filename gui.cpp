#include <SDL_stdinc.h>
#include <SDL_video.h>
#include <SDL_audio.h>
#include <fstream>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <SDL.h>
#include <SDL_render.h>
#include <SDL_events.h>
#include <SDL_keycode.h>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <deque>
#include <ostream>
#include <random>

//Screen
const int SCREEN_X = 980;
const int SCREEN_Y = 640;

//Obstacles
const int LONGLENGTH = 100;
const int SHORTLENGTH = 10;
const double OBSTACLESPEED = 250;
const int MAXOBSTACLES = 500;
const int DAMAGESPEED = 400;
const int SPACING = 100;
const int HARDSPACING = 70;

//Player
const int PLAYERSCALE = 40;
const int PLAYERSCALESML = 10;
const int HIDDENPLAYERSCALE = 34;
const int HIDDENPLAYERSCALESML = 4;
const int PLAYERSPEED = 200;
const int ALTPLAYERSPEED = 40;

//Wave Management
const int TIMEPERWAVE = 2;

//Sound
const int SFXPLAYERS = 10;
const bool MUSIC = true;

//Effects
const double FORCE = 200;
const double PHASETIME = 0.04;
const int PHASES = 2;

struct Effects
{
    double offset_x = 0;
    double offset_y = 0;
    double momentum_x = 0;
    double momentum_y = 0;
    double phaseTime = 0;
    int phases = 0;
};

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
        SDL_AudioSpec wavSpec;
        Uint32 wavLength;
        Uint8 *wavBuffer;
        SDL_AudioDeviceID deviceId;

#ifndef __EMSCRIPTEN__
        SDL_AudioSpec wavSpecSfx;
        Uint32 wavLengthSfx;
        Uint8 *wavBufferSfx;
        int sfxdevice = 0;
        std::vector<SDL_AudioDeviceID> deviceIdSfx;
#endif
};

AudioPlayer::AudioPlayer()
{
    SDL_Init(SDL_INIT_AUDIO );
    std::cout << "Create AudioPlayer" << std::endl;
    SDL_LoadWAV("resources/song.wav", &wavSpec, &wavBuffer, &wavLength);
    deviceId = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, 0);
    SDL_QueueAudio(deviceId, wavBuffer, wavLength);
    playSong();


#ifndef __EMSCRIPTEN__
    SDL_LoadWAV("resources/sound.wav", &wavSpecSfx, &wavBufferSfx, &wavLengthSfx);
    for(int i=0; i <= SFXPLAYERS; i++)
    {
        deviceIdSfx.push_back(SDL_OpenAudioDevice(NULL, 0, &wavSpecSfx, NULL, 0));
        SDL_PauseAudioDevice(deviceIdSfx[i], 0);
    }
#endif
}

AudioPlayer::~AudioPlayer()
{
    SDL_CloseAudioDevice(deviceId);
    SDL_FreeWAV(wavBuffer);
#ifndef __EMSCRIPTEN__
    for(int i=0; i <= SFXPLAYERS; i++)
    {
        SDL_CloseAudioDevice(deviceIdSfx[i]);
    }
    SDL_FreeWAV(wavBufferSfx);
#endif
    std::cout << "Destroyed AudioPlayer" << std::endl;
}

void AudioPlayer::playSound()
{
#ifndef __EMSCRIPTEN__
    SDL_QueueAudio(deviceIdSfx[sfxdevice], wavBufferSfx, wavLengthSfx);
    sfxdevice += 1;
    if(sfxdevice >= SFXPLAYERS)
    {
        sfxdevice = 0;
    }
#endif
}

void AudioPlayer::playSong()
{
    if(MUSIC)
    {
        SDL_PauseAudioDevice(deviceId, 0);
    }
}

void AudioPlayer::pauseSong()
{
    SDL_PauseAudioDevice(deviceId, 1);
}

void AudioPlayer::checkRestart()
{
    if(SDL_GetQueuedAudioSize(deviceId) <= 0)
    {
        int success = SDL_QueueAudio(deviceId, wavBuffer, wavLength);
    }
    //std::cout << "QueuedSize: "<< SDL_GetQueuedAudioSize(deviceId) << std::endl;
}

int randomRange(int min, int max)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(min, max);

    return distr(gen);
}

double randomRange_D(double min, double max)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> distr(min, max);

    return distr(gen);
}

struct Obstacle
{
    Obstacle();
    double x;
    double y;
    int scaleX;
    int scaleY;
    double speedX;
    double speedY;
    int r;
    int g;
    int b;
};

Obstacle::Obstacle()
{
    r = randomRange(50, 200);
    g = randomRange(50, 200);
    b = randomRange(50, 200);
}

struct Player
{
    double x = SCREEN_X / 2;
    double y = SCREEN_Y / 2;
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool change = false;
    bool _change = false;
    double hp = 255;
};

void renderSquare(int x, int y, int scaleX, int scaleY, int r, int g, int b, SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, r,  g, b, 255);
    SDL_Rect rect = {x - scaleX/2, y - scaleY/2, scaleX, scaleY};
    SDL_RenderFillRect(renderer, &rect);
}

class Game
{
    public:
        Game();
        ~Game();
        void input();
        void loopItteration();
        bool quitting = false;
        bool lost = false;
    private:
        //Timing
        double deltaTime;
        std::chrono::time_point<std::chrono::steady_clock> lastMeasure;

        //Game Logic
        void newGame();
        void manageWaves();
        void moveObjects();
        bool checkPlayerDeath();
        void renderFrame();
        void showScore();

        //Attacks
        void arroundAttack();
        void shower();
        void lattice();
        void splice();

        //Necessary Variables
        SDL_Renderer* renderer;
        SDL_Window* window;
        std::deque<Obstacle> things;
        SDL_Event Event;
        bool firstFrame;
        double lostTime;

        //Wave Manager
        double timeTillWave = TIMEPERWAVE;
        int currentWaveID = 0;
        int intensity;
        
        //Player
        Player player;

        //Sound
        AudioPlayer audio;

        //Effects
        Effects effects;
};

void Game::newGame()
{
    lastMeasure = std::chrono::steady_clock::now();
    firstFrame = true;
    player.x = SCREEN_X / 2;
    player.y = SCREEN_Y / 2;
    player.hp = 255;
    //things.clear();
    intensity = 1;
    timeTillWave = TIMEPERWAVE;
    currentWaveID = 0;
    lost = false;
    lostTime = 0;
    audio.playSong();
}

Game::Game()
{
    std::cout << "Game has been created" << std::endl;
    SDL_Init(SDL_INIT_VIDEO );
    window = SDL_CreateWindow("Change", 100, 100, SCREEN_X, SCREEN_Y, SDL_WINDOW_SHOWN);
    SDL_SetWindowResizable(window, SDL_TRUE);
    renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_RenderSetLogicalSize(renderer, SCREEN_X, SCREEN_Y);
    newGame();
}

Game::~Game()
{
    std::cout << "Game has been destroyed" << std::endl;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Game::renderFrame()
{
    for (int i = 0; i < things.size(); i++)
    {
        double x = things[i].x + effects.offset_x;
        double y = things[i].y + effects.offset_y;
        renderSquare(x, y, things[i].scaleX, things[i].scaleY, things[i].r, things[i].g, things[i].b ,renderer);
    }
    double x = player.x + effects.offset_x;
    double y = player.y + effects.offset_y;

    if (player.change == false)
        renderSquare(x, y, PLAYERSCALE, PLAYERSCALE,player.hp,player.hp,player.hp, renderer);
    else
        renderSquare(x, y, PLAYERSCALESML, PLAYERSCALESML,player.hp,player.hp,player.hp, renderer);
    
    SDL_UpdateWindowSurface(window);
    SDL_RenderPresent(renderer);
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    effects.offset_x += effects.momentum_x * deltaTime;
    effects.offset_y += effects.momentum_y * deltaTime;
    effects.phaseTime -= deltaTime;
    if(effects.phaseTime <= 0)
    {
        if (effects.phases <= 0)
        {
            double x = effects.offset_x;
            double y = effects.offset_y;
            double magnitude = sqrt(x*x + y*y);
            if(deltaTime > 0.01)
            {
                effects.offset_x = 0;
                effects.offset_y = 0;
                effects.momentum_x = 0;
                effects.momentum_y = 0;
            }
            else if (magnitude > 0.01)
            {
                x /= magnitude;
                y /= magnitude;
                effects.momentum_x = -x * FORCE;
                effects.momentum_y = -y * FORCE;
            }
            else
            {
                effects.offset_x = 0;
                effects.offset_y = 0;
                effects.momentum_x = 0;
                effects.momentum_y = 0;
            }
        }
        else
        {
            effects.phaseTime = PHASETIME;
            effects.phases -= 1;
            double phase = randomRange_D(0, M_PI * 2);
            effects.momentum_x = std::sin(phase)*FORCE;
            effects.momentum_y = std::cos(phase)*FORCE;
        }
    }
}

void Game::arroundAttack()
{
    for(int i = 0; i < intensity; i++)
    {
        Obstacle newObject;
        newObject.x = SCREEN_X + (LONGLENGTH/2);
        newObject.y = randomRange(0, SCREEN_Y);
        newObject.scaleX = LONGLENGTH;
        newObject.scaleY = SHORTLENGTH;
        newObject.speedX = -OBSTACLESPEED;
        newObject.speedY = 0;
        things.push_back(newObject);
    }
    for(int i = 0; i < intensity; i++)
    {
        Obstacle newObject;
        newObject.x = randomRange(0, SCREEN_X);
        newObject.y = SCREEN_Y + (LONGLENGTH/2);
        newObject.scaleX = SHORTLENGTH;
        newObject.scaleY = LONGLENGTH;
        newObject.speedX = 0;
        newObject.speedY = -OBSTACLESPEED;
        things.push_back(newObject);
    }
    for(int i = 0; i < intensity; i++)
    {
        Obstacle newObject;
        newObject.x = 0 - (LONGLENGTH/2);
        newObject.y = randomRange(0, SCREEN_Y);
        newObject.scaleX = LONGLENGTH;
        newObject.scaleY = SHORTLENGTH;
        newObject.speedX = OBSTACLESPEED;
        newObject.speedY = 0;
        things.push_back(newObject);
    }
    for(int i = 0; i < intensity; i++)
    {
        Obstacle newObject;
        newObject.x = randomRange(0, SCREEN_X);
        newObject.y = 0 - (LONGLENGTH/2);
        newObject.scaleX = SHORTLENGTH;
        newObject.scaleY = LONGLENGTH;
        newObject.speedX = 0;
        newObject.speedY = OBSTACLESPEED;
        things.push_back(newObject);
    }
}

void Game::shower()
{
    int choice = randomRange(0, 1);
    if(choice == 0)
    {
        for(int i = 0; i < intensity * 4; i++)
        {
            Obstacle newObject;
            newObject.x = randomRange(0, SCREEN_X);
            newObject.y = SCREEN_Y + randomRange((LONGLENGTH/2), SCREEN_Y);
            newObject.scaleX = SHORTLENGTH;
            newObject.scaleY = LONGLENGTH;
            newObject.speedX = 0;
            newObject.speedY = -OBSTACLESPEED;
            things.push_back(newObject);
        }
    }
    else if (choice == 1) 
    {
        for(int i = 0; i < intensity * 4; i++)
        {
            Obstacle newObject;
            newObject.x = randomRange(0, SCREEN_X);
            newObject.y = randomRange(-SCREEN_Y, -LONGLENGTH/2);
            newObject.scaleX = SHORTLENGTH;
            newObject.scaleY = LONGLENGTH;
            newObject.speedX = 0;
            newObject.speedY = OBSTACLESPEED;
            things.push_back(newObject);
        }
    }
}

void Game::lattice()
{
    int space = intensity >= 8 ? HARDSPACING : SPACING;
    int choice = randomRange(0, 1);
    int ofset_lattice = randomRange(0, space);
    if(choice == 0 || intensity > 30)
    {
        for(int i = ofset_lattice; i < SCREEN_X; i += space)
        {
            Obstacle newObject;
            newObject.x = i;
            newObject.y = SCREEN_Y + (LONGLENGTH/2);
            newObject.scaleX = SHORTLENGTH;
            newObject.scaleY = LONGLENGTH;
            newObject.speedX = 0;
            newObject.speedY = -OBSTACLESPEED;
            things.push_back(newObject);
        }
    }
    if (choice == 1 || intensity > 30) 
    {
        for(int i = ofset_lattice; i < SCREEN_X; i += space)
        {
            Obstacle newObject;
            newObject.x = i;
            newObject.y = 0 - (LONGLENGTH/2);
            newObject.scaleX = SHORTLENGTH;
            newObject.scaleY = LONGLENGTH;
            newObject.speedX = 0;
            newObject.speedY = OBSTACLESPEED;
            things.push_back(newObject);
        }
    }

    if(intensity <= 4)
        return;

    space = intensity > 20 ? HARDSPACING : SPACING;
    choice = randomRange(0, 1);
    ofset_lattice = randomRange(0, space);
    if(choice == 0 || intensity > 40)
    {
        for(int i = 0; i < SCREEN_Y; i += SPACING)
        {
            Obstacle newObject;
            newObject.x = SCREEN_X + (LONGLENGTH/2);
            newObject.y = i;
            newObject.scaleX = LONGLENGTH;
            newObject.scaleY = SHORTLENGTH;
            newObject.speedX = -OBSTACLESPEED;
            newObject.speedY = 0;
            things.push_back(newObject);
        }
    }
    if (choice == 1 || intensity > 40) 
    {
        for(int i = 0; i < SCREEN_Y; i += SPACING)
        {
            Obstacle newObject;
            newObject.x = -(LONGLENGTH/2);
            newObject.y = i;
            newObject.scaleX = LONGLENGTH;
            newObject.scaleY = SHORTLENGTH;
            newObject.speedX = OBSTACLESPEED;
            newObject.speedY = 0;
            things.push_back(newObject);
        }
    }
}

void Game::splice()
{
    shower();
    return;
    int choice = randomRange(0, 1);
    if(choice == 0)
    {
        for(int i = 0; i < intensity * 4; i++)
        {
            Obstacle newObject;
            newObject.x = randomRange(0, SCREEN_X);
            newObject.y = SCREEN_Y + (LONGLENGTH/2);
            newObject.scaleX = SHORTLENGTH;
            newObject.scaleY = LONGLENGTH;
            newObject.speedX = 0;
            newObject.speedY = -OBSTACLESPEED;
            things.push_back(newObject);
        }
    }
    else if (choice == 1) 
    {
        for(int i = 0; i < intensity * 4; i++)
        {
            Obstacle newObject;
            newObject.x = randomRange(0, SCREEN_X);
            newObject.y = 0 - (LONGLENGTH/2);
            newObject.scaleX = SHORTLENGTH;
            newObject.scaleY = LONGLENGTH;
            newObject.speedX = 0;
            newObject.speedY = OBSTACLESPEED;
            things.push_back(newObject);
        }
    }
}

void Game::input()
{
    while(SDL_PollEvent(&Event))
    {
#ifndef __EMSCRIPTEN__
        if (Event.type == SDL_QUIT)
        {
            quitting = true;
        }
#endif
        if (Event.type == SDL_KEYDOWN)
        {
            switch(Event.key.keysym.sym)
            {
#ifndef __EMSCRIPTEN__
                case SDLK_ESCAPE:
                    quitting = true;
                    break;
#endif
                case SDLK_UP:
                case SDLK_w:
                    player.up = true;
                    break;
                case SDLK_DOWN:
                case SDLK_s:
                    player.down = true;
                    break;
                case SDLK_LEFT:
                case SDLK_a:
                    player.left = true;
                    break;
                case SDLK_RIGHT:
                case SDLK_d:
                    player.right = true;
                    break;
                case SDLK_SPACE:
                    player.change = true;
                    break;
            }
        }
        if (Event.type == SDL_KEYUP)
        {
            switch(Event.key.keysym.sym)
            {
                case SDLK_UP:
                case SDLK_w:
                    player.up = false;
                    break;
                case SDLK_DOWN:
                case SDLK_s:
                    player.down = false;
                    break;
                case SDLK_LEFT:
                case SDLK_a:
                    player.left = false;
                    break;
                case SDLK_RIGHT:
                case SDLK_d:
                    player.right = false;
                    break;
                case SDLK_SPACE:
                    player.change = false;
                    break;
            }
        }
    }
}

void Game::manageWaves()
{
    timeTillWave -= deltaTime;
    if (timeTillWave <= 0)
    {
        timeTillWave += TIMEPERWAVE;
        int choice = randomRange(0, 2);
        if(choice == 0)
            arroundAttack();
        else if (choice == 1)
            shower();
        else if (choice == 2)
            lattice();
        else if (choice == 3)
            splice();

        while(things.size() >= MAXOBSTACLES)
            things.pop_front();

        currentWaveID += 1;
        if(currentWaveID % 5 == 0)
            intensity += 1;

    }
}

bool Game::checkPlayerDeath()
{
    int topX, topY, bottomX, bottomY;
    topX = topY = bottomX = bottomY = 0;
    if(!player.change)
    {
        topX = player.x + ((double)HIDDENPLAYERSCALE/2);
        topY = player.y + ((double)HIDDENPLAYERSCALE/2);
        bottomX = player.x - ((double)HIDDENPLAYERSCALE/2);
        bottomY = player.y - ((double)HIDDENPLAYERSCALE/2);
    }
    else
    {
        topX = player.x + ((double)HIDDENPLAYERSCALESML/2);
        topY = player.y + ((double)HIDDENPLAYERSCALESML/2);
        bottomX = player.x - ((double)HIDDENPLAYERSCALESML/2);
        bottomY = player.y - ((double)HIDDENPLAYERSCALESML/2);
    }
    for(int i=0; i < things.size(); i++)
    {
        int _topX = things[i].x + ((double)things[i].scaleX/2);
        int _topY = things[i].y + ((double)things[i].scaleY/2);
        int _bottomX = things[i].x - ((double)things[i].scaleX/2);
        int _bottomY = things[i].y - ((double)things[i].scaleY/2);
        bool x = false;
        bool y = false;
        if(!(_topX<bottomX||topX<_bottomX))
            x = true;
        if(!(_topY<bottomY||topY<_bottomY))
            y = true;
        if(x && y)
            return true;
    }
    if(topY < 0 || topX < 0 || bottomX > SCREEN_X || bottomY > SCREEN_Y)
        return true;
    return false;
}

void Game::moveObjects()
{
    for(int i = 0; i < things.size(); i++)
    {
        things[i].x += things[i].speedX * deltaTime;
        things[i].y += things[i].speedY * deltaTime;
    }

    int x = player.right - player.left;
    int y = player.down - player.up;
    double magnitude = std::sqrt(x*x + y*y);
    int speed = player.change ? ALTPLAYERSPEED : PLAYERSPEED;
    if(magnitude >= 0.2)
    {
        player.x += (x/magnitude) * deltaTime * speed;
        player.y += (y/magnitude) * deltaTime * speed;
    }
}

void Game::loopItteration()
{
    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
    double timeElapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - lastMeasure).count();
    lastMeasure = now;
    deltaTime = timeElapsed / 1000000;
    if( firstFrame == true)
    {
        firstFrame = false;
        return;
    }
    input();
    if (!lost)
        manageWaves();
    moveObjects();
    bool death = checkPlayerDeath();
    if (death)
    {
        player.hp -= deltaTime * DAMAGESPEED;
        if(player.hp <= 0)
        {
            lost = true;
        }
    }

    if(lost)
    {
        audio.pauseSong();
        lostTime += deltaTime;
        player.x = 100000;
        if(lostTime >= 2)
            newGame();
    }
    if(player._change != player.change)
    {
        player._change = player.change;
        audio.playSound();
        double phase = randomRange_D(0, M_PI * 2);
        effects.momentum_x = std::sin(phase)*FORCE;
        effects.momentum_y = std::cos(phase)*FORCE;
        effects.phases = PHASES;
        effects.phaseTime = PHASETIME;
    }

    audio.checkRestart();
    renderFrame();
}

Game game;

static void mainloop()
{
#ifdef __EMSCRIPTEN__
    if (game.quitting)
        emscripten_cancel_main_loop();
    game.loopItteration();
#endif
}

int main(int argc, char* argv[]) 
{
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(mainloop, 0, 1);
#else
    while(!game.quitting)
    {
        game.loopItteration();
    }
#endif

    return 0;
}

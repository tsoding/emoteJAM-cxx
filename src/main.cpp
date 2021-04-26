#include "aids.hpp"

#include <SDL2/SDL.h>

using namespace aids;

int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    defer(SDL_Quit());
    

    return 0;
}

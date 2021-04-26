#include "aids.hpp"

#include <SDL2/SDL.h>

using namespace aids;

void sdl_check(int code)
{
    if (code < 0) {
        panic("SDL ERROR: ", SDL_GetError());
    }
}

template <typename T>
T *sdl_check(T *ptr)
{
    if (ptr == nullptr) {
        panic("SDL ERROR: ", SDL_GetError());
    }
    return ptr;
}

int main()
{
    sdl_check(SDL_Init(SDL_INIT_VIDEO));
    defer(SDL_Quit());

    SDL_Window *window =
        sdl_check(SDL_CreateWindow(
                      "emoteJAM",
                      0, 0,
                      800, 600,
                      SDL_WINDOW_RESIZABLE));
    defer(SDL_DestroyWindow(window));

    SDL_Renderer *renderer =
        sdl_check(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED));
    defer(SDL_DestroyRenderer(renderer));

    bool quit = false;
    while (!quit) {
        SDL_Event event = {};
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: {
                quit = true;
            }
            break;
            }
        }

        sdl_check(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255));
        sdl_check(SDL_RenderClear(renderer));

        SDL_RenderPresent(renderer);
    }

    return 0;
}

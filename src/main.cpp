#include "./aids.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

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

void usage(FILE *stream, const char *program)
{
    println(stream, "Usage: ", program, " <emote.png>");
}

SDL_Texture *texture_from_file(SDL_Renderer *renderer, const char *file_path)
{
    int width, height;
    unsigned char *pixels = stbi_load(file_path, &width, &height, NULL, 4);
    if (pixels == NULL) {
        panic("ERROR: could not load file ", file_path, ": ", strerror(errno));
    }
    defer(stbi_image_free(pixels));

    SDL_Surface *surface =
        sdl_check(
            SDL_CreateRGBSurfaceFrom(
                pixels,
                width,
                height,
                32,
                width * 4,
                0x000000FF,
                0x0000FF00,
                0x00FF0000,
                0xFF000000));
    defer(SDL_FreeSurface(surface));

    return sdl_check(SDL_CreateTextureFromSurface(renderer, surface));
}

int main(int argc, char **argv)
{
    Args args = {argc, argv};
    const auto program = args.shift(); // skip program name;

    if (args.empty()) {
        usage(stderr, program);
        panic("ERROR: Path to emote's image is not provided");
    }

    const char *emote_image_path = args.shift();

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

    SDL_Texture *texture = texture_from_file(renderer, emote_image_path);
    defer(SDL_DestroyTexture(texture));

    SDL_Rect texture_rect = {};
    sdl_check(SDL_QueryTexture(
                  texture,
                  NULL, NULL,
                  &texture_rect.w, &texture_rect.h));

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

        sdl_check(SDL_RenderCopy(renderer,
                                 texture,
                                 &texture_rect,
                                 &texture_rect));

        SDL_RenderPresent(renderer);
    }

    return 0;
}

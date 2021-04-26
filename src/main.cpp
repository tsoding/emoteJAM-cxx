#include "./aids.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "./stb_image_write.h"

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

void render_state(SDL_Renderer *renderer,
                  SDL_Texture *texture,
                  const SDL_Rect *texture_rect)
{
    sdl_check(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255));
    sdl_check(SDL_RenderClear(renderer));

    sdl_check(SDL_RenderCopy(renderer,
                             texture,
                             texture_rect,
                             texture_rect));

    SDL_RenderPresent(renderer);
}

void save_renderer_to_png_file(SDL_Renderer *renderer,
                               const char *file_path)
{
    SDL_Rect target_rect = {};
    Uint32 target_format = SDL_PIXELFORMAT_ABGR8888;
    sdl_check(SDL_GetRendererOutputSize(
                  renderer,
                  &target_rect.w,
                  &target_rect.h));

    Uint32 *pixels = static_cast<Uint32*>(malloc(sizeof(Uint32) * target_rect.w * target_rect.h));
    if (pixels == nullptr) {
        panic("ERROR: save_renderer_to_png_file: could not allocate memory for pixels: ", strerror(errno));
    }
    defer(free(pixels));


    {
        Uint32 begin = SDL_GetTicks();
        sdl_check(SDL_RenderReadPixels(
                      renderer,
                      &target_rect,
                      target_format,
                      pixels,
                      sizeof(*pixels) * target_rect.w));
        println(stdout, "read pixels: ", SDL_GetTicks() - begin);
    }

    {
        Uint32 begin = SDL_GetTicks();
        if (!stbi_write_png(file_path, target_rect.w, target_rect.h, 4, pixels, sizeof(*pixels) * target_rect.w)) {
            panic("ERROR: could not save image to file ", file_path, ": ", strerror(errno));
        }
        println(stdout, "stbi_write_png: ", SDL_GetTicks() - begin);
    }
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

    const size_t DISPLAY_WIDTH = 800;
    const size_t DISPLAY_HEIGHT = 800;
    SDL_Window *window =
        sdl_check(SDL_CreateWindow(
                      "emoteJAM",
                      0, 0,
                      DISPLAY_WIDTH, DISPLAY_HEIGHT,
                      SDL_WINDOW_RESIZABLE));
    defer(SDL_DestroyWindow(window));

    SDL_Renderer *renderer =
        sdl_check(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED));
    defer(SDL_DestroyRenderer(renderer));

    {
        SDL_RendererInfo info = {};
        sdl_check(SDL_GetRendererInfo(renderer, &info));
        if (!(info.flags & SDL_RENDERER_TARGETTEXTURE)) {
            panic("ERROR: SDL Renderer Target Texture is NOT supported!");
        } else {
            println(stderr, "INFO: SDL Renderer Target Texture is supported!");
        }
    }

    SDL_Texture *emote_texture = texture_from_file(renderer, emote_image_path);
    defer(SDL_DestroyTexture(emote_texture));

    SDL_Rect emote_texture_rect = {};
    Uint32 emote_texture_format = 0;
    sdl_check(SDL_QueryTexture(
                  emote_texture,
                  &emote_texture_format,
                  NULL,
                  &emote_texture_rect.w,
                  &emote_texture_rect.h));

    SDL_Texture *target = sdl_check(
                              SDL_CreateTexture(
                                  renderer,
                                  SDL_PIXELFORMAT_ABGR8888,
                                  SDL_TEXTUREACCESS_TARGET,
                                  emote_texture_rect.w,
                                  emote_texture_rect.h));


    bool quit = false;
    while (!quit) {
        SDL_Event event = {};
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: {
                quit = true;
            }
            break;

            case SDL_KEYDOWN: {
                switch (event.key.keysym.sym) {
                case SDLK_s: {
                    sdl_check(SDL_SetRenderTarget(renderer, target));
                    defer(SDL_SetRenderTarget(renderer, NULL));

                    render_state(renderer, emote_texture, &emote_texture_rect);
                    Uint32 begin = SDL_GetTicks();
                    save_renderer_to_png_file(renderer, "output.png");
                    println(stdout, SDL_GetTicks() - begin);
                }
                break;
                }
            }
            break;
            }
        }

        render_state(renderer, emote_texture, &emote_texture_rect);
    }

    return 0;
}

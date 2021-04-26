#include "./aids.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "./stb_image_write.h"

#include <SDL2/SDL.h>

using namespace aids;

using Seconds = float;
using Milliseconds = Uint32;

const size_t DISPLAY_WIDTH = 800;
const size_t DISPLAY_HEIGHT = 800;
const size_t DISPLAY_FPS = 60;
const Milliseconds DELTA_TIME_MS = 1000 / DISPLAY_FPS;
const Seconds DELTA_TIME_SECS = 1.0f / static_cast<float>(DISPLAY_FPS);

Seconds current_time()
{
    return static_cast<float>(SDL_GetTicks()) / 1000.0f;
}

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
                  const SDL_Rect *texture_src,
                  float stretch)
{
    sdl_check(SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255));
    sdl_check(SDL_RenderClear(renderer));

    SDL_Rect texture_dest = {};

    texture_dest.h = static_cast<int>(floorf(static_cast<float>(texture_src->h) * stretch));
    texture_dest.w = static_cast<int>(floorf(static_cast<float>(texture_src->w) * (2.0f - stretch)));
    const int bottom_x = texture_src->x + texture_src->w / 2;
    const int bottom_y = texture_src->y + texture_src->h;
    texture_dest.x = bottom_x - texture_dest.w / 2;
    texture_dest.y = bottom_y - texture_dest.h;

    sdl_check(SDL_RenderCopy(renderer,
                             texture,
                             texture_src,
                             &texture_dest));

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
        for (int y = 0; y < target_rect.h; ++y) {
            for (int x = 0; x < target_rect.w; ++x) {
                if (pixels[y * target_rect.w + x] == 0xFF00FF00) {
                    pixels[y * target_rect.w + x] = 0;
                }
            }
        }
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

void save_animation(SDL_Renderer *renderer, SDL_Texture *emote_texture,
                    const SDL_Rect *emote_texture_rect,
                    size_t fps, Seconds duration)
{
    Seconds delta_time = 1.0f / static_cast<float>(fps);
    Seconds time = 0.0f;

    char file_path[1024];

    for (size_t frame_index = 0; time <= duration; ++frame_index) {
        float stretch = sinf(6.0f * time) * 0.5f + 1.0f;
        render_state(renderer, emote_texture, emote_texture_rect, stretch);
        snprintf(file_path, sizeof(file_path), "frame-%02zu.png", frame_index);
        save_renderer_to_png_file(renderer, file_path);
        println(stdout, "Saved ", file_path);
        time += delta_time;
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

                    save_animation(renderer,
                                   emote_texture,
                                   &emote_texture_rect,
                                   15,
                                   2.0f * M_PI / 6.0f);
                }
                break;
                }
            }
            break;
            }
        }

        float stretch = sinf(6.0f * current_time()) * 0.5f + 1.0f;
        render_state(renderer, emote_texture, &emote_texture_rect, stretch);
        SDL_Delay(DELTA_TIME_MS);
    }

    return 0;
}

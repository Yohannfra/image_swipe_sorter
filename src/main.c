#include "files.h"
#include "history.h"
#include "render.h"
#include "types.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    Config config;
    if (parse_args(argc, argv, &config) != 0) {
        return 1;
    }

    ImageList images;
    if (load_image_list(config.source_dir, &images) != 0) {
        return 1;
    }

    if (images.count == 0) {
        printf("No images found in '%s'\n", config.source_dir);
        return 0;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        free_image_list(&images);
        return 1;
    }

    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_WEBP | IMG_INIT_TIF;
    if ((IMG_Init(img_flags) & img_flags) != img_flags) {
        fprintf(stderr, "IMG_Init Error: %s\n", IMG_GetError());
        SDL_Quit();
        free_image_list(&images);
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Image Sorter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280,
        720, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (!window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        free_image_list(&images);
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        free_image_list(&images);
        return 1;
    }

    SDL_Texture *current_texture = NULL;
    int img_width = 0, img_height = 0;
    int need_load = 1;

    /* Zoom and pan state */
    float zoom = 1.0f;
    float pan_x = 0.0f;
    float pan_y = 0.0f;
    int dragging = 0;
    int drag_start_x = 0;
    int drag_start_y = 0;
    float drag_start_pan_x = 0.0f;
    float drag_start_pan_y = 0.0f;

    MoveHistory history;
    history_init(&history);

    int running = 1;
    SDL_Event event;

    while (running) {
        /* Allow undo even when all images processed */
        if (images.current >= images.count && history.count == 0) {
            break;
        }

        /* Load current image if needed */
        if (need_load && images.current < images.count) {
            if (current_texture) {
                SDL_DestroyTexture(current_texture);
                current_texture = NULL;
            }

            SDL_Surface *surface = IMG_Load(images.paths[images.current]);
            if (surface) {
                current_texture = SDL_CreateTextureFromSurface(renderer, surface);
                img_width = surface->w;
                img_height = surface->h;
                SDL_FreeSurface(surface);

                /* Reset zoom and pan for new image */
                zoom = 1.0f;
                pan_x = 0.0f;
                pan_y = 0.0f;

                char title[MAX_PATH + 64];
                snprintf(title, sizeof(title), "Image Sorter - %d/%d - %s", images.current + 1, images.count,
                    strrchr(images.paths[images.current], '/') + 1);
                SDL_SetWindowTitle(window, title);
            } else {
                fprintf(stderr, "Failed to load: %s\n", images.paths[images.current]);
                images.current++;
                continue;
            }
            need_load = 0;
        } else if (need_load && images.current >= images.count) {
            if (current_texture) {
                SDL_DestroyTexture(current_texture);
                current_texture = NULL;
            }
            SDL_SetWindowTitle(window, "Image Sorter - Done! (SPACE to undo)");
            need_load = 0;
        }

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_MOUSEWHEEL) {
                /* Zoom with mouse wheel */
                if (current_texture) {
                    float old_zoom = zoom;
                    if (event.wheel.y > 0) {
                        zoom *= 1.2f;
                    } else if (event.wheel.y < 0) {
                        zoom /= 1.2f;
                    }
                    /* Clamp zoom level */
                    if (zoom < 0.1f)
                        zoom = 0.1f;
                    if (zoom > 20.0f)
                        zoom = 20.0f;

                    /* Zoom towards mouse cursor */
                    int mouse_x, mouse_y;
                    SDL_GetMouseState(&mouse_x, &mouse_y);
                    int win_width, win_height;
                    SDL_GetWindowSize(window, &win_width, &win_height);

                    /* Adjust pan to keep the point under cursor stationary */
                    float center_x = win_width / 2.0f + pan_x;
                    float center_y = win_height / 2.0f + pan_y;
                    float dx = mouse_x - center_x;
                    float dy = mouse_y - center_y;
                    pan_x += dx * (1.0f - zoom / old_zoom);
                    pan_y += dy * (1.0f - zoom / old_zoom);
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    dragging = 1;
                    drag_start_x = event.button.x;
                    drag_start_y = event.button.y;
                    drag_start_pan_x = pan_x;
                    drag_start_pan_y = pan_y;
                } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                    /* Middle click to reset zoom/pan */
                    zoom = 1.0f;
                    pan_x = 0.0f;
                    pan_y = 0.0f;
                }
            } else if (event.type == SDL_MOUSEBUTTONUP) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    dragging = 0;
                }
            } else if (event.type == SDL_MOUSEMOTION) {
                if (dragging) {
                    pan_x = drag_start_pan_x + (event.motion.x - drag_start_x);
                    pan_y = drag_start_pan_y + (event.motion.y - drag_start_y);
                }
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                    case SDLK_q:
                        running = 0;
                        break;
                    case SDLK_LEFT: {
                        if (images.current < images.count) {
                            char dest_path[MAX_PATH];
                            if (move_file(images.paths[images.current], config.left_dir, dest_path) == 0) {
                                history_push(&history, images.paths[images.current], dest_path, images.current);
                                images.current++;
                                need_load = 1;
                            }
                        }
                        break;
                    }
                    case SDLK_RIGHT: {
                        if (images.current < images.count) {
                            char dest_path[MAX_PATH];
                            if (move_file(images.paths[images.current], config.right_dir, dest_path) == 0) {
                                history_push(&history, images.paths[images.current], dest_path, images.current);
                                images.current++;
                                need_load = 1;
                            }
                        }
                        break;
                    }
                    case SDLK_DOWN:
                        if (images.current < images.count) {
                            images.current++;
                            need_load = 1;
                        }
                        break;
                    case SDLK_SPACE: {
                        MoveEntry entry;
                        if (history_pop(&history, &entry) == 0) {
                            if (undo_move_file(entry.dest_path, entry.src_path) == 0) {
                                images.current = entry.image_index;
                                need_load = 1;
                            }
                        }
                        break;
                    }
                }
            }
        }

        /* Render */
        int win_width, win_height;
        SDL_GetWindowSize(window, &win_width, &win_height);

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        /* Draw image centered and scaled to fit, with zoom and pan */
        if (current_texture) {
            int margin = 80;
            int available_width = win_width - margin * 2;
            int available_height = win_height - margin;

            /* Base scale to fit image in window */
            float scale_x = (float)available_width / img_width;
            float scale_y = (float)available_height / img_height;
            float base_scale = (scale_x < scale_y) ? scale_x : scale_y;
            if (base_scale > 1.0f)
                base_scale = 1.0f;

            /* Apply zoom */
            float final_scale = base_scale * zoom;

            int render_width = (int)(img_width * final_scale);
            int render_height = (int)(img_height * final_scale);
            int render_x = (int)((win_width - render_width) / 2.0f + pan_x);
            int render_y = (int)((win_height - render_height) / 2.0f + pan_y);

            SDL_Rect dest = {render_x, render_y, render_width, render_height};
            SDL_RenderCopy(renderer, current_texture, NULL, &dest);
        }

        /* Draw UI indicators */
        int arrow_size = 60;
        int arrow_y = win_height / 2;

        SDL_SetRenderDrawColor(renderer, 200, 100, 100, 255);
        render_arrow(renderer, 20, arrow_y, arrow_size, -1);

        SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
        render_arrow(renderer, win_width - 20 - arrow_size, arrow_y, arrow_size, 1);

        /* Bottom instructions */
        int text_y = win_height - 25;
        int text_scale = 2;

        SDL_SetRenderDrawColor(renderer, 200, 100, 100, 255);
        render_text(renderer, "<- LEFT", 15, text_y, text_scale);

        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        render_text(renderer, "DOWN:SKIP  SPACE:UNDO", win_width / 2 - 120, text_y, text_scale);

        SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
        render_text(renderer, "RIGHT ->", win_width - 130, text_y, text_scale);

        /* Progress bar */
        int progress_width = win_width - 20;
        int progress_height = 4;
        SDL_Rect progress_bg = {10, 10, progress_width, progress_height};
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
        SDL_RenderFillRect(renderer, &progress_bg);

        int filled = (int)((float)images.current / images.count * progress_width);
        SDL_Rect progress_fill = {10, 10, filled, progress_height};
        SDL_SetRenderDrawColor(renderer, 100, 150, 200, 255);
        SDL_RenderFillRect(renderer, &progress_fill);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    if (images.current >= images.count) {
        printf("All images have been processed!\n");
    }

    if (current_texture) {
        SDL_DestroyTexture(current_texture);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    free_image_list(&images);

    return 0;
}

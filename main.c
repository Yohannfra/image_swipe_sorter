#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <dirent.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_IMAGES 10000
#define MAX_PATH   4096

typedef struct {
    char **paths;
    int count;
    int current;
} ImageList;

typedef struct {
    char source_dir[MAX_PATH];
    char left_dir[MAX_PATH];
    char right_dir[MAX_PATH];
} Config;

static int is_image_file(const char *filename)
{
    const char *ext = strrchr(filename, '.');
    if (!ext)
        return 0;
    ext++;

    const char *extensions[] = {"png", "PNG", "jpg", "JPG", "jpeg", "JPEG", "bmp", "BMP", "gif", "GIF", "webp",
        "WEBP", "tif", "TIF", "tiff", "TIFF", NULL};

    for (int i = 0; extensions[i]; i++) {
        if (strcmp(ext, extensions[i]) == 0)
            return 1;
    }
    return 0;
}

static int load_image_list(const char *dir_path, ImageList *list)
{
    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Error: Cannot open directory '%s'\n", dir_path);
        return -1;
    }

    list->paths = malloc(sizeof(char *) * MAX_IMAGES);
    list->count = 0;
    list->current = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && list->count < MAX_IMAGES) {
        if (entry->d_name[0] == '.')
            continue;
        if (!is_image_file(entry->d_name))
            continue;

        char full_path[MAX_PATH];
        snprintf(full_path, MAX_PATH, "%s/%s", dir_path, entry->d_name);

        list->paths[list->count] = strdup(full_path);
        list->count++;
    }

    closedir(dir);
    printf("Found %d images\n", list->count);
    return 0;
}

static void free_image_list(ImageList *list)
{
    for (int i = 0; i < list->count; i++) {
        free(list->paths[i]);
    }
    free(list->paths);
}

static int move_file(const char *src, const char *dest_dir)
{
    char *src_copy = strdup(src);
    char *filename = basename(src_copy);

    char dest_path[MAX_PATH];
    snprintf(dest_path, MAX_PATH, "%s/%s", dest_dir, filename);

    if (rename(src, dest_path) != 0) {
        fprintf(stderr, "Error: Failed to move '%s' to '%s'\n", src, dest_path);
        free(src_copy);
        return -1;
    }

    printf("Moved: %s -> %s\n", filename, dest_dir);
    free(src_copy);
    return 0;
}

static void render_text_simple(SDL_Renderer *renderer, const char *text, int x, int y, SDL_Color color, int scale)
{
    /* Simple bitmap-style text rendering without SDL_ttf */
    /* Each character is rendered as a small rectangle pattern */
    int char_width = 8 * scale;
    int char_height = 12 * scale;

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    for (int i = 0; text[i]; i++) {
        int cx = x + i * (char_width + 2 * scale);
        /* Draw a simple block for each character */
        SDL_Rect r = {cx, y, char_width, char_height};

        /* Simple character patterns */
        char c = text[i];
        if (c == ' ')
            continue;

        /* Draw filled rectangle for simplicity */
        SDL_RenderFillRect(renderer, &r);
    }
}

static void render_arrow(SDL_Renderer *renderer, int x, int y, int size, int direction)
{
    /* direction: -1 = left, 1 = right */
    SDL_Point points[7];
    int half = size / 2;
    int third = size / 3;

    if (direction < 0) {
        /* Left arrow */
        points[0] = (SDL_Point){x, y};
        points[1] = (SDL_Point){x + half, y - half};
        points[2] = (SDL_Point){x + half, y - third};
        points[3] = (SDL_Point){x + size, y - third};
        points[4] = (SDL_Point){x + size, y + third};
        points[5] = (SDL_Point){x + half, y + third};
        points[6] = (SDL_Point){x + half, y + half};
    } else {
        /* Right arrow */
        points[0] = (SDL_Point){x + size, y};
        points[1] = (SDL_Point){x + half, y - half};
        points[2] = (SDL_Point){x + half, y - third};
        points[3] = (SDL_Point){x, y - third};
        points[4] = (SDL_Point){x, y + third};
        points[5] = (SDL_Point){x + half, y + third};
        points[6] = (SDL_Point){x + half, y + half};
    }

    /* Draw filled arrow using lines */
    for (int i = 0; i < 6; i++) {
        SDL_RenderDrawLine(renderer, points[i].x, points[i].y, points[i + 1].x, points[i + 1].y);
    }
    SDL_RenderDrawLine(renderer, points[6].x, points[6].y, points[0].x, points[0].y);

    /* Fill the arrow */
    for (int row = y - half; row <= y + half; row++) {
        int left_bound = -1, right_bound = -1;
        for (int col = x; col <= x + size; col++) {
            int inside = 0;
            /* Simple point-in-polygon for arrow */
            if (direction < 0) {
                /* Left arrow fill logic */
                int rel_x = col - x;
                int rel_y = row - y;
                if (rel_x >= half) {
                    inside = (abs(rel_y) <= third);
                } else {
                    inside = (abs(rel_y) <= (half - rel_x));
                }
            } else {
                /* Right arrow fill logic */
                int rel_x = x + size - col;
                int rel_y = row - y;
                if (rel_x >= half) {
                    inside = (abs(rel_y) <= third);
                } else {
                    inside = (abs(rel_y) <= (half - rel_x));
                }
            }
            if (inside) {
                if (left_bound < 0)
                    left_bound = col;
                right_bound = col;
            }
        }
        if (left_bound >= 0) {
            SDL_RenderDrawLine(renderer, left_bound, row, right_bound, row);
        }
    }
}

static int parse_args(int argc, char *argv[], Config *config)
{
    memset(config, 0, sizeof(Config));

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source_dir> --left-dir=<path> --right-dir=<path>\n", argv[0]);
        return -1;
    }

    strncpy(config->source_dir, argv[1], MAX_PATH - 1);

    for (int i = 2; i < argc; i++) {
        if (strncmp(argv[i], "--left-dir=", 11) == 0) {
            strncpy(config->left_dir, argv[i] + 11, MAX_PATH - 1);
        } else if (strncmp(argv[i], "--right-dir=", 12) == 0) {
            strncpy(config->right_dir, argv[i] + 12, MAX_PATH - 1);
        }
    }

    if (config->left_dir[0] == '\0' || config->right_dir[0] == '\0') {
        fprintf(stderr, "Error: Both --left-dir and --right-dir are required\n");
        return -1;
    }

    /* Verify source directory exists */
    struct stat st;
    if (stat(config->source_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: Source directory '%s' does not exist\n", config->source_dir);
        return -1;
    }

    /* Create left/right directories if they don't exist */
    if (stat(config->left_dir, &st) != 0) {
        if (mkdir(config->left_dir, 0755) != 0) {
            fprintf(stderr, "Error: Cannot create left directory '%s'\n", config->left_dir);
            return -1;
        }
        printf("Created directory: %s\n", config->left_dir);
    } else if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: '%s' exists but is not a directory\n", config->left_dir);
        return -1;
    }

    if (stat(config->right_dir, &st) != 0) {
        if (mkdir(config->right_dir, 0755) != 0) {
            fprintf(stderr, "Error: Cannot create right directory '%s'\n", config->right_dir);
            return -1;
        }
        printf("Created directory: %s\n", config->right_dir);
    } else if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: '%s' exists but is not a directory\n", config->right_dir);
        return -1;
    }

    return 0;
}

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

    int running = 1;
    SDL_Event event;

    while (running && images.current < images.count) {
        /* Load current image if needed */
        if (need_load) {
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

                char title[MAX_PATH + 64];
                snprintf(title, sizeof(title), "Image Sorter - %d/%d - %s", images.current + 1, images.count,
                    strrchr(images.paths[images.current], '/') + 1);
                SDL_SetWindowTitle(window, title);
            } else {
                fprintf(stderr, "Failed to load: %s\n", images.paths[images.current]);
                /* Skip to next image */
                images.current++;
                continue;
            }
            need_load = 0;
        }

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                    case SDLK_q:
                        running = 0;
                        break;
                    case SDLK_LEFT:
                        if (move_file(images.paths[images.current], config.left_dir) == 0) {
                            images.current++;
                            need_load = 1;
                        }
                        break;
                    case SDLK_RIGHT:
                        if (move_file(images.paths[images.current], config.right_dir) == 0) {
                            images.current++;
                            need_load = 1;
                        }
                        break;
                    case SDLK_DOWN:
                    case SDLK_SPACE:
                        /* Skip without moving */
                        images.current++;
                        need_load = 1;
                        break;
                }
            }
        }

        /* Render */
        int win_width, win_height;
        SDL_GetWindowSize(window, &win_width, &win_height);

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        /* Draw image centered and scaled to fit */
        if (current_texture) {
            int margin = 80;
            int available_width = win_width - margin * 2;
            int available_height = win_height - margin;

            float scale_x = (float)available_width / img_width;
            float scale_y = (float)available_height / img_height;
            float scale = (scale_x < scale_y) ? scale_x : scale_y;
            if (scale > 1.0f)
                scale = 1.0f;

            int render_width = (int)(img_width * scale);
            int render_height = (int)(img_height * scale);
            int render_x = (win_width - render_width) / 2;
            int render_y = (win_height - render_height) / 2;

            SDL_Rect dest = {render_x, render_y, render_width, render_height};
            SDL_RenderCopy(renderer, current_texture, NULL, &dest);
        }

        /* Draw UI indicators */
        int arrow_size = 60;
        int arrow_y = win_height / 2;

        /* Left arrow and label */
        SDL_SetRenderDrawColor(renderer, 200, 100, 100, 255);
        render_arrow(renderer, 20, arrow_y, arrow_size, -1);

        /* Right arrow and label */
        SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
        render_arrow(renderer, win_width - 20 - arrow_size, arrow_y, arrow_size, 1);

        /* Bottom instructions */
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        SDL_Rect left_hint = {10, win_height - 30, 120, 20};
        SDL_Rect right_hint = {win_width - 130, win_height - 30, 120, 20};
        SDL_Rect skip_hint = {win_width / 2 - 60, win_height - 30, 120, 20};

        /* Draw hint backgrounds */
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
        SDL_RenderFillRect(renderer, &left_hint);
        SDL_RenderFillRect(renderer, &right_hint);
        SDL_RenderFillRect(renderer, &skip_hint);

        /* Draw borders */
        SDL_SetRenderDrawColor(renderer, 200, 100, 100, 255);
        SDL_RenderDrawRect(renderer, &left_hint);
        SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
        SDL_RenderDrawRect(renderer, &right_hint);
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        SDL_RenderDrawRect(renderer, &skip_hint);

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

    if (current_texture)
        SDL_DestroyTexture(current_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    free_image_list(&images);

    return 0;
}

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

#define HISTORY_SIZE 50

typedef struct {
    char src_path[MAX_PATH];  /* Original path (in source dir) */
    char dest_path[MAX_PATH]; /* Where it was moved to */
    int image_index;          /* Index in images list */
} MoveEntry;

typedef struct {
    MoveEntry entries[HISTORY_SIZE];
    int head;  /* Next write position */
    int count; /* Number of valid entries */
} MoveHistory;

static void history_init(MoveHistory *h)
{
    h->head = 0;
    h->count = 0;
}

static void history_push(MoveHistory *h, const char *src, const char *dest, int index)
{
    strncpy(h->entries[h->head].src_path, src, MAX_PATH - 1);
    strncpy(h->entries[h->head].dest_path, dest, MAX_PATH - 1);
    h->entries[h->head].image_index = index;
    h->head = (h->head + 1) % HISTORY_SIZE;
    if (h->count < HISTORY_SIZE)
        h->count++;
}

static int history_pop(MoveHistory *h, MoveEntry *out)
{
    if (h->count == 0)
        return -1;
    h->head = (h->head - 1 + HISTORY_SIZE) % HISTORY_SIZE;
    h->count--;
    *out = h->entries[h->head];
    return 0;
}

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

static int move_file(const char *src, const char *dest_dir, char *out_dest_path)
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
    if (out_dest_path) {
        strncpy(out_dest_path, dest_path, MAX_PATH - 1);
    }
    free(src_copy);
    return 0;
}

static int undo_move(MoveEntry *entry)
{
    if (rename(entry->dest_path, entry->src_path) != 0) {
        fprintf(stderr, "Error: Failed to undo move '%s'\n", entry->dest_path);
        return -1;
    }
    printf("Undo: restored %s\n", entry->src_path);
    return 0;
}

/* 5x7 bitmap font for basic characters */
static const unsigned char font_5x7[128][7] = {
    ['<'] = {0x04, 0x08, 0x10, 0x08, 0x04, 0x00, 0x00},
    ['>'] = {0x10, 0x08, 0x04, 0x08, 0x10, 0x00, 0x00},
    ['-'] = {0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00},
    ['A'] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x00},
    ['C'] = {0x0E, 0x11, 0x10, 0x10, 0x11, 0x0E, 0x00},
    ['D'] = {0x1E, 0x11, 0x11, 0x11, 0x11, 0x1E, 0x00},
    ['E'] = {0x1F, 0x10, 0x1E, 0x10, 0x10, 0x1F, 0x00},
    ['F'] = {0x1F, 0x10, 0x1E, 0x10, 0x10, 0x10, 0x00},
    ['G'] = {0x0E, 0x11, 0x10, 0x17, 0x11, 0x0E, 0x00},
    ['H'] = {0x11, 0x11, 0x1F, 0x11, 0x11, 0x11, 0x00},
    ['I'] = {0x0E, 0x04, 0x04, 0x04, 0x04, 0x0E, 0x00},
    ['K'] = {0x11, 0x12, 0x1C, 0x12, 0x11, 0x11, 0x00},
    ['L'] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x1F, 0x00},
    ['N'] = {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x00},
    ['O'] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x0E, 0x00},
    ['P'] = {0x1E, 0x11, 0x1E, 0x10, 0x10, 0x10, 0x00},
    ['R'] = {0x1E, 0x11, 0x1E, 0x14, 0x12, 0x11, 0x00},
    ['S'] = {0x0E, 0x10, 0x0E, 0x01, 0x01, 0x0E, 0x00},
    ['T'] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00},
    ['U'] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x0E, 0x00},
    ['W'] = {0x11, 0x11, 0x11, 0x15, 0x15, 0x0A, 0x00},
    ['B'] = {0x1E, 0x11, 0x1E, 0x11, 0x11, 0x1E, 0x00},
    ['/'] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00},
    [':'] = {0x00, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00},
    ['0'] = {0x0E, 0x13, 0x15, 0x19, 0x11, 0x0E, 0x00},
    ['1'] = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x0E, 0x00},
    ['2'] = {0x0E, 0x11, 0x02, 0x04, 0x08, 0x1F, 0x00},
    ['3'] = {0x0E, 0x01, 0x06, 0x01, 0x11, 0x0E, 0x00},
    ['4'] = {0x02, 0x06, 0x0A, 0x1F, 0x02, 0x02, 0x00},
    ['5'] = {0x1F, 0x10, 0x1E, 0x01, 0x11, 0x0E, 0x00},
    ['6'] = {0x0E, 0x10, 0x1E, 0x11, 0x11, 0x0E, 0x00},
    ['7'] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x00},
    ['8'] = {0x0E, 0x11, 0x0E, 0x11, 0x11, 0x0E, 0x00},
    ['9'] = {0x0E, 0x11, 0x0F, 0x01, 0x11, 0x0E, 0x00},
};

static void render_text(SDL_Renderer *renderer, const char *text, int x, int y, int scale)
{
    int cursor_x = x;
    for (int i = 0; text[i]; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c == ' ') {
            cursor_x += 6 * scale;
            continue;
        }
        if (c < 128) {
            for (int row = 0; row < 7; row++) {
                unsigned char bits = font_5x7[c][row];
                for (int col = 0; col < 5; col++) {
                    if (bits & (0x10 >> col)) {
                        SDL_Rect pixel = {cursor_x + col * scale, y + row * scale, scale, scale};
                        SDL_RenderFillRect(renderer, &pixel);
                    }
                }
            }
        }
        cursor_x += 6 * scale;
    }
}

static void render_arrow(SDL_Renderer *renderer, int x, int y, int size, int direction)
{
    /* direction: -1 = left, 1 = right */
    int half = size / 2;
    int shaft_height = size / 3;

    if (direction < 0) {
        /* Left arrow: point at left, shaft extends right */
        /* Triangle: point at x, base at x+half */
        for (int row = -half; row <= half; row++) {
            int width = half - abs(row);
            if (width > 0) {
                /* Draw from (x + half - width) to (x + half) */
                SDL_RenderDrawLine(renderer, x + half - width, y + row, x + half, y + row);
            }
        }
        /* Draw shaft rectangle to the right of triangle */
        SDL_Rect shaft = {x + half, y - shaft_height, half, shaft_height * 2};
        SDL_RenderFillRect(renderer, &shaft);
    } else {
        /* Right arrow: shaft on left, point at right */
        /* Draw shaft rectangle */
        SDL_Rect shaft = {x, y - shaft_height, half, shaft_height * 2};
        SDL_RenderFillRect(renderer, &shaft);
        /* Triangle: base at x+half, point at x+size */
        for (int row = -half; row <= half; row++) {
            int width = half - abs(row);
            if (width > 0) {
                /* Draw from (x + half) to (x + half + width) */
                SDL_RenderDrawLine(renderer, x + half, y + row, x + half + width, y + row);
            }
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
        } else if (need_load && images.current >= images.count) {
            /* All images processed - clear texture and update title */
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
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                    case SDLK_q:
                        running = 0;
                        break;
                    case SDLK_LEFT: {
                        char dest_path[MAX_PATH];
                        if (move_file(images.paths[images.current], config.left_dir, dest_path) == 0) {
                            history_push(&history, images.paths[images.current], dest_path, images.current);
                            images.current++;
                            need_load = 1;
                        }
                        break;
                    }
                    case SDLK_RIGHT: {
                        char dest_path[MAX_PATH];
                        if (move_file(images.paths[images.current], config.right_dir, dest_path) == 0) {
                            history_push(&history, images.paths[images.current], dest_path, images.current);
                            images.current++;
                            need_load = 1;
                        }
                        break;
                    }
                    case SDLK_DOWN:
                        /* Skip without moving */
                        images.current++;
                        need_load = 1;
                        break;
                    case SDLK_SPACE: {
                        /* Undo last move */
                        MoveEntry entry;
                        if (history_pop(&history, &entry) == 0) {
                            if (undo_move(&entry) == 0) {
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

        /* Bottom instructions with text */
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

    if (current_texture)
        SDL_DestroyTexture(current_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    free_image_list(&images);

    return 0;
}

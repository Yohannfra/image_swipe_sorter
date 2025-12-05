#include "files.h"

#include <dirent.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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

int parse_args(int argc, char *argv[], Config *config)
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

int load_image_list(const char *dir_path, ImageList *list)
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

void free_image_list(ImageList *list)
{
    for (int i = 0; i < list->count; i++) {
        free(list->paths[i]);
    }
    free(list->paths);
}

int move_file(const char *src, const char *dest_dir, char *out_dest_path)
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
        snprintf(out_dest_path, MAX_PATH, "%s", dest_path);
    }
    free(src_copy);
    return 0;
}

int undo_move_file(const char *dest_path, const char *src_path)
{
    if (rename(dest_path, src_path) != 0) {
        fprintf(stderr, "Error: Failed to undo move '%s'\n", dest_path);
        return -1;
    }
    printf("Undo: restored %s\n", src_path);
    return 0;
}

#ifndef TYPES_H
#define TYPES_H

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

#endif /* TYPES_H */

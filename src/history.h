#ifndef HISTORY_H
#define HISTORY_H

#include "types.h"

#define HISTORY_SIZE 50

typedef struct {
    char src_path[MAX_PATH];  /* Original path (in source dir) */
    char dest_path[MAX_PATH]; /* Where it was moved to */
    int image_index;          /* Index in images list */
    int direction;            /* -1 for left, 1 for right */
} MoveEntry;

typedef struct {
    MoveEntry entries[HISTORY_SIZE];
    int head;  /* Next write position */
    int count; /* Number of valid entries */
} MoveHistory;

/* Initialize history */
void history_init(MoveHistory *h);

/* Push a move entry to history */
void history_push(MoveHistory *h, const char *src, const char *dest, int index, int direction);

/* Pop last move entry, returns 0 on success, -1 if empty */
int history_pop(MoveHistory *h, MoveEntry *out);

#endif /* HISTORY_H */

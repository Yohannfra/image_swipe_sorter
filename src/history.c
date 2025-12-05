#include "history.h"

#include <string.h>

void history_init(MoveHistory *h)
{
    h->head = 0;
    h->count = 0;
}

void history_push(MoveHistory *h, const char *src, const char *dest, int index, int direction)
{
    strncpy(h->entries[h->head].src_path, src, MAX_PATH - 1);
    strncpy(h->entries[h->head].dest_path, dest, MAX_PATH - 1);
    h->entries[h->head].image_index = index;
    h->entries[h->head].direction = direction;
    h->head = (h->head + 1) % HISTORY_SIZE;
    if (h->count < HISTORY_SIZE)
        h->count++;
}

int history_pop(MoveHistory *h, MoveEntry *out)
{
    if (h->count == 0)
        return -1;
    h->head = (h->head - 1 + HISTORY_SIZE) % HISTORY_SIZE;
    h->count--;
    *out = h->entries[h->head];
    return 0;
}

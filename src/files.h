#ifndef FILES_H
#define FILES_H

#include "types.h"

/* Parse command line arguments and populate config */
int parse_args(int argc, char *argv[], Config *config);

/* Load list of image files from directory */
int load_image_list(const char *dir_path, ImageList *list);

/* Free image list memory */
void free_image_list(ImageList *list);

/* Move file to destination directory, returns dest path in out_dest_path */
int move_file(const char *src, const char *dest_dir, char *out_dest_path);

/* Move file from dest back to src (undo) */
int undo_move_file(const char *dest_path, const char *src_path);

#endif /* FILES_H */

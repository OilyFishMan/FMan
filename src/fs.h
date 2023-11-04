#ifndef FS_H_
#define FS_H_

#include "fimale.h"

#include <stddef.h>
#include <stdbool.h>
#include <limits.h>

// arbitrary number
#define MAX_FS_OBJECTS 4096

enum fs_object_enum
{
    E_object_file,
    E_object_folder,
};

struct fs_object
{
    // the actual strings need to be null terminated (because lstat)
    // so there's no point storing their lengths here
    // instead, we store relative pointers to the arena
    size_t path_idx;
    size_t real_path_idx;

    enum fs_object_enum type;
};

struct fs
{
    char path[PATH_MAX];

    size_t objects_len;
    struct fs_object objects[MAX_FS_OBJECTS];

    size_t path_arena_used;
    size_t path_arena_capacity;
    char*  path_arena;
};

#define fs_default ((struct fs) {0})

bool fs_init(struct fs* fs, char* error_message);
void fs_dealloc(struct fs* fs);
bool fs_reload(struct fs* fs, char* error_message);
bool fs_chdir(struct fs* fs, char* path, char* error_message);

#endif // FS_H_

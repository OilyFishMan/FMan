#ifndef FS_H_
#define FS_H_

#include "fimale.h"

#include <stddef.h>
#include <stdbool.h>
#include <limits.h>

// arbitrary number
#define MAX_FS_OBJECTS 512

enum fs_object_enum
{
    E_object_file,
    E_object_folder,
};

struct fs_object
{
    char *path;
    char *real_path;
    enum fs_object_enum type;
};

struct fs
{
    char path[PATH_MAX];

    size_t objects_len;
    struct fs_object objects[MAX_FS_OBJECTS];
};

#define fs_default ((struct fs) { .path = {0}, .objects_len = 0, .objects = {0} })

bool fs_init(struct fs* fs, char* error_message);
void fs_dealloc(struct fs* fs);
bool fs_reload(struct fs* fs, char* error_message);
bool fs_chdir(struct fs* fs, char* path, char* error_message);

#endif // FS_H_

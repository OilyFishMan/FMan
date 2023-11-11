#ifndef FS_H_
#define FS_H_

#include <stddef.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>

// arbitrary number
#define MAX_FS_ENTRIES 4096

enum fs_entry_enum
{
    E_entry_file,
    E_entry_folder,
};

struct fs_entry
{
    enum fs_entry_enum type;

    // The actual strings need to be null terminated (because stat)
    // so there's no point storing their lengths here.
    size_t path_idx;

    time_t last_updated;
};

struct fs
{
    char path[PATH_MAX];

    time_t last_updated;

    size_t entries_len;
    struct fs_entry entries[MAX_FS_ENTRIES];

    size_t path_arena_used;
    size_t path_arena_capacity;
    char*  path_arena;
};

bool fs_init(struct fs* fs, char* error_message);
void fs_dealloc(struct fs* fs);
bool fs_reload(struct fs* fs, char* error_message);
bool fs_update(struct fs* fs, char* error_message);
bool fs_chdir_abs(struct fs* fs, char* path, char* error_message);
bool fs_chdir(struct fs* fs, char* path, char* error_message);

#endif // FS_H_

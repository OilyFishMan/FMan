#include "fimale.h"
#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <curses.h>

bool fs_init(struct fs* fs, char* error_message)
{
    fs->objects_len = 0;

    fs->path_arena_used = 0;
    fs->path_arena_capacity = 0;
    fs->path_arena = malloc(sizeof(*fs->path_arena) * fs->path_arena_capacity);

    if (fs->path_arena == NULL) {
        strcpy(error_message, "malloc failed");
        goto failure;
    }

    if (!fs_chdir(fs, ".", error_message)) {
        goto failure_path_arena;
    }

    if (!fs_reload(fs, error_message)) {
        goto failure_path_arena;
    }

    return true;

failure_path_arena:
    if (fs->path_arena != NULL) {
        free(fs->path_arena);
        fs->path_arena = NULL;
    }

failure:
    return false;
}

void fs_dealloc(struct fs* fs)
{
    // in case of a nasty allocation error we can't recover from.
    if (fs->path_arena != NULL) {
        free(fs->path_arena);
        fs->path_arena = NULL;
    }
}

bool fs_reload(struct fs* fs, char* error_message)
{
    bool exit_success = true;

    fs->objects_len = 0;
    fs->path_arena_used = 0;

    DIR* srcdir = opendir(fs->path);

    if (srcdir == NULL) {
        strcpy(error_message, "opendir failed");
        exit_success = false;
        goto cleanup;
    }

    struct dirent* entry;

    while ((entry = readdir(srcdir)) != NULL) {
        const size_t prev_used = fs->path_arena_used;

        struct fs_object* object = &fs->objects[fs->objects_len];

        object->real_path_idx = fs->path_arena_used;

        fs->path_arena_used += strlen(fs->path) + 1 + strlen(entry->d_name) + 1;

        bool have_to_realloc = false;

        while (fs->path_arena_capacity <= fs->path_arena_used) {
            fs->path_arena_capacity += fs->path_arena_capacity / 2 + 1;
            have_to_realloc = true;
        }

        if (have_to_realloc) {
            char* new_path_arena = realloc(fs->path_arena, sizeof(*fs->path_arena) * fs->path_arena_capacity);

            if (new_path_arena == NULL) {
                // we are NOT calling fs_dealloc here
                // because this code may not end up being equivalent
                // to that function in the future.
                // for instance, we could have multiple allocations,
                // some of which we may not want to free,
                // or OS objects we want to keep alive even afterwards.
                free(fs->path_arena);
                fs->path_arena = NULL;

                strcpy(error_message, "realloc failed");
                exit_success = false;
                goto cleanup_srcdir;
            }

            fs->path_arena = new_path_arena;
        }

        // breakpoint

        strcpy(&fs->path_arena[object->real_path_idx], fs->path);
        strcat(&fs->path_arena[object->real_path_idx], "/");

        object->path_idx = object->real_path_idx + strlen(fs->path) + 1;

        strcat(&fs->path_arena[object->path_idx], entry->d_name);

        struct stat st;

        if (stat(&fs->path_arena[object->real_path_idx], &st) < 0) {
            fs->path_arena_used = prev_used;
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            object->type = E_object_folder;
        } else {
            object->type = E_object_file;
        }

        ++fs->objects_len;
    }
    
cleanup_srcdir:
    closedir(srcdir);

cleanup:
    return exit_success;
}

bool fs_chdir(struct fs* fs, char* path, char* error_message)
{
    if (realpath(path, fs->path) == NULL) {
        strcpy(error_message, "realpath failed");
        return false;
    }

    return true;
}

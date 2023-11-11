#include "fman.h"
#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <curses.h>

bool fs_init(struct fs* fs, char* error_message)
{
    memset(fs->path, 0, PATH_MAX);

    fs->last_updated = (time_t) 0;

    fs->entries_len = 0;

    fs->path_arena_used = 0;
    fs->path_arena_capacity = 0;
    fs->path_arena = malloc(sizeof(*fs->path_arena) * fs->path_arena_capacity);

    if (fs->path_arena == NULL) {
        strcpy(error_message, "malloc failed");
        goto failure;
    }

    if (!fs_chdir_abs(fs, ".", error_message)) {
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

    fs->entries_len = 0;
    fs->path_arena_used = 0;

    DIR* srcdir = opendir(fs->path);

    if (srcdir == NULL) {
        strcpy(error_message, "opendir failed");
        exit_success = false;
        goto cleanup;
    }

    struct dirent* dent;

    while ((dent = readdir(srcdir)) != NULL) {
        struct stat st;

        if (fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0) {
            continue;
        }

        struct fs_entry* entry = &fs->entries[fs->entries_len];

        entry->path_idx = fs->path_arena_used;

        fs->path_arena_used += strlen(dent->d_name) + 1;

        bool have_to_realloc = false;

        while (fs->path_arena_capacity <= fs->path_arena_used) {
            fs->path_arena_capacity += fs->path_arena_capacity / 2 + 1;
            have_to_realloc = true;
        }

        if (have_to_realloc) {
            char* new_path_arena = realloc(fs->path_arena, sizeof(*fs->path_arena) * fs->path_arena_capacity);

            if (new_path_arena == NULL) {
                free(fs->path_arena);
                fs->path_arena = NULL;

                strcpy(error_message, "realloc failed");
                exit_success = false;
                goto cleanup_srcdir;
            }

            fs->path_arena = new_path_arena;
        }

        strcpy(&fs->path_arena[entry->path_idx], dent->d_name);

        if (S_ISDIR(st.st_mode)) {
            entry->type = E_entry_folder;
        } else {
            entry->type = E_entry_file;
        }

        entry->last_updated = st.st_atime;

        ++fs->entries_len;
    }
    
cleanup_srcdir:
    closedir(srcdir);

cleanup:
    return exit_success;
}

bool fs_update(struct fs* fs, char* error_message)
{
    struct stat st;

    if (stat(fs->path, &st) < 0) {
        strcpy(error_message, "stat failed (directory may not exist)");
        return false;
    }

    if (!fs_reload(fs, error_message)) {
        return false;
    }

    fs->last_updated = st.st_atime;

    return true;
}

bool fs_chdir_abs(struct fs* fs, char* path, char* error_message)
{
    if (realpath(path, fs->path) == NULL) {
        strcpy(error_message, "realpath failed");
        return false;
    }

    return true;
}

bool fs_chdir(struct fs* fs, char* path, char* error_message)
{
    assert(strlen(fs->path) + 1 + strlen(path) < PATH_MAX);

    strcat(fs->path, "/");
    strcat(fs->path, path);

    if (realpath(fs->path, fs->path) == NULL) {
        strcpy(error_message, "realpath failed");
        return false;
    }

    return true;
}

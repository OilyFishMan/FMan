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

bool fs_init(struct fs* fs, char error_message[MAX_ERROR_MSG_LEN])
{
    fs->objects_len = 0;

    if (realpath(".", fs->path) == NULL) {
        strcpy(error_message, "realpath failed");
        return false;
    }

    if (!fs_reload(fs, error_message)) {
        return false;
    }

    return true;
}

bool fs_reload(struct fs* fs, char error_message[MAX_ERROR_MSG_LEN])
{
    bool exit_success = true;

    for (size_t i = 0; i < fs->objects_len; ++i) {
        free(fs->objects[i].path);
    }

    fs->objects_len = 0;

    DIR* srcdir = opendir(fs->path);

    if (srcdir == NULL) {
        strcpy(error_message, "opendir failed");
        exit_success = false;
        goto cleanup_last;
    }

    struct dirent* dent = readdir(srcdir);

    for (; dent != NULL; dent = readdir(srcdir)) {
        struct stat st;

        if (fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0) {
            strcpy(error_message, "opening file failed");
            exit_success = false;
            goto cleanup;
        }

        struct fs_object* object = &fs->objects[fs->objects_len++];

        object->path = malloc(sizeof(*object->path) * strlen(dent->d_name));

        if (object->path == NULL) {
            strcpy(error_message, "malloc failed");
            exit_success = false;
            goto cleanup;
        }

        strcpy(object->path, dent->d_name);

        if (S_ISDIR(st.st_mode)) {
            object->type = E_object_folder;
        } else {
            object->type = E_object_file;
        }
    }
    
cleanup:
    closedir(srcdir);

cleanup_last:
    return exit_success;
}

bool fs_chdir(struct fs* fs, char path[PATH_MAX], char error_message[MAX_ERROR_MSG_LEN])
{
    if (realpath(path, fs->path) == NULL) {
        strcpy(error_message, "realpath failed");
        return false;
    }

    return true;
}

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

void fs_object_dealloc(struct fs_object* object)
{
    free(object->path);
    free(object->real_path);
}

bool fs_init(struct fs* fs, char* error_message)
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

void fs_dealloc(struct fs* fs)
{
    for (size_t i = 0; i < fs->objects_len; ++i) {
        fs_object_dealloc(&fs->objects[i]);
    }
}

bool fs_reload(struct fs* fs, char* error_message)
{
    bool exit_success = true;

    for (size_t i = 0; i < fs->objects_len; ++i) {
        fs_object_dealloc(&fs->objects[i]);
    }

    fs->objects_len = 0;

    //static int i = 0;
    //if (i++ == 4) {
    //    fprintf(stderr, "[        %s      ]\n", fs->path);
    //    exit(1);
    //}

    DIR* srcdir = opendir(fs->path);

    if (srcdir == NULL) {
        strcpy(error_message, "opendir failed");
        exit_success = false;
        goto cleanup;
    }

    for (;;) {
        struct dirent* dent = readdir(srcdir);

        if (dent == NULL) {
            break;
        }

        struct stat st;

        if (fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0) {
            strcpy(error_message, "fstat failed");
            exit_success = false;
            goto cleanup;
        }

        struct fs_object* object = &fs->objects[fs->objects_len];

        object->real_path = malloc(sizeof(*object->real_path) * (strlen(fs->path) + 1 + strlen(dent->d_name) + 1));

        if (object->real_path == NULL) {
            strcpy(error_message, "malloc failed");
            exit_success = false;
            goto cleanup_dir;
        }

        strcpy(object->real_path, fs->path);
        strcat(object->real_path, "/");
        strcat(object->real_path, dent->d_name);

        object->real_path = realpath(object->real_path, NULL);

        if (object->real_path == NULL) {
            strcpy(error_message, "realpath failed");
            exit_success = false;
            goto cleanup_dir;
        }

        object->path = malloc(sizeof(*object->path) * (strlen(dent->d_name) + 1));

        if (object->path == NULL) {
            free(object->real_path);
            strcpy(error_message, "malloc failed");
            exit_success = false;
            goto cleanup_dir;
        }

        strcpy(object->path, dent->d_name);

        if (S_ISDIR(st.st_mode)) {
            object->type = E_object_folder;
        } else {
            object->type = E_object_file;
        }

        ++fs->objects_len;
    }
    
cleanup_dir:
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

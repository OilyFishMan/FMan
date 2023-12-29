#include "fman.h"
#include "fs.h"
#include "pattern.h"
#include "editor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>

bool fman_init(struct fman* fman, char* error_message)
{
    fman->mode = E_mode_normal;
    fman->command = E_command_find;

    memset(fman->typing_text, 0, sizeof(*fman->typing_text) * PATH_MAX);

    memset(fman->pattern, 0, sizeof(*fman->pattern) * PATH_MAX);

    fman->pattern_matches_len = 0;

    fman->screen_width = 0;
    fman->screen_height = 0;

    if (!fs_init(&fman->fs, error_message)) {
        return false;
    }

    fman->cursor_y = 0;
    fman->match_cursor_y = 0;
    fman->scroll_y = 0;

    if (!editor_init(&fman->editor, error_message)) {
        return false;
    }

    return true;
}

void fman_delete(struct fman* fman)
{
    fs_delete(&fman->fs);
}

void fman_move_up(struct fman* fman)
{
    if (fman->match_cursor_y > 0) {
        if (fman->match_cursor_y == fman->scroll_y) {
            --fman->scroll_y;
        }

        --fman->match_cursor_y;
    }
}

void fman_move_down(struct fman* fman)
{
    if (fman->match_cursor_y < fman->pattern_matches_len - 1) {
        ++fman->match_cursor_y;

        if (fman->match_cursor_y - fman->scroll_y >= fman_fs_window_height(fman)) {
            ++fman->scroll_y;
        }
    }
}

void fman_move_screen_up(struct fman* fman)
{
    if (fman->match_cursor_y + 1 >= fman->screen_height) {
        fman->match_cursor_y -= fman_fs_window_height(fman);
    } else {
        fman->match_cursor_y = 0;
    }

    fman->scroll_y = fman->match_cursor_y;
}

void fman_move_screen_down(struct fman* fman)
{
    if (fman->pattern_matches_len >= fman->screen_height
     && fman->match_cursor_y + fman_fs_window_height(fman) <= fman->pattern_matches_len - 1) {
        fman->match_cursor_y += fman_fs_window_height(fman);
    } else {
        fman->match_cursor_y = fman->pattern_matches_len - 1;
    }

    if (fman->match_cursor_y - fman->scroll_y >= fman_fs_window_height(fman)) {
        fman->scroll_y = fman->match_cursor_y - fman_fs_window_height(fman) + 1;
    }
}

bool fman_interact(struct fman* fman, char* error_message)
{
    fman->pattern[0] = '\0';

    if (fman->pattern_matches_len == 0) {
        return true;
    }

    struct fs_entry entry = fman->fs.entries[fman->pattern_matches[fman->match_cursor_y]];

    switch (entry.type) {
        case E_entry_file: {
            fman->mode = E_mode_buffer_edit;
            char path[PATH_MAX] = {0};
            strcat(path, fman->fs.path);
            strcat(path, "/");
            strcat(path, &fman->fs.path_arena[entry.path_idx]);

            if (!editor_open(&fman->editor, path, error_message)) {
                return false;
            }
            return true;
        }

        case E_entry_folder: {
            if (!fs_chdir(&fman->fs, &fman->fs.path_arena[entry.path_idx], error_message)) {
                return false;
            }

            fman->match_cursor_y = 0;
            fman->scroll_y = 0;
            return true;
        }
    }
}

bool fman_go_back_dir(struct fman* fman, char* error_message)
{
    fman->pattern[0] = '\0';

    return fs_chdir(&fman->fs, "..", error_message);
}

void fman_begin_command(struct fman* fman, enum fman_command command)
{
    fman->mode = E_mode_typing;

    fman->typing_text[0] = '\0';

    fman->command = command;
}

void fman_type_char(struct fman* fman, const char ch)
{
    const size_t len = strlen(fman->typing_text);

    if (len + 1 < PATH_MAX) {
        fman->typing_text[len] = ch;
        fman->typing_text[len + 1] = '\0';
    }
}

void fman_delete_char(struct fman* fman)
{
    const size_t len = strlen(fman->typing_text);

    if (len > 0) {
        fman->typing_text[len - 1] = '\0';
    }
}

bool fman_submit_command(struct fman* fman, char* error_message)
{
    switch (fman->command) {
        case E_command_select_command: {
            bool command_found = false;

            for (size_t i = 0; i < FMAN_COMMANDS_MAX; ++i) {
                if (strcmp(command_string[i], fman->typing_text) == 0) {
                    fman->command = (enum fman_command) i;
                    command_found = true;
                }
            }

            if (!command_found) {
                return true;
            }

            break;
        }

        case E_command_find: {
            strcpy(fman->pattern, fman->typing_text);
            fman->mode = E_mode_normal;
            break;
        }

        case E_command_remove: {
            if (!fs_remove(&fman->fs, fman->typing_text, error_message)) {
                return false;
            }

            fman->mode = E_mode_normal;
            break;
        }

        case E_command_create_file: {
            if (!fs_create_file(&fman->fs, fman->typing_text, error_message)) {
                return false;
            }
            fman->mode = E_mode_normal;
            break;
        }

        case E_command_create_folder: {
            if (!fs_create_folder(&fman->fs, fman->typing_text, error_message)) {
                return false;
            }
            fman->mode = E_mode_normal;
            break;
        }
    }

    fman->typing_text[0] = '\0';

    return true;
}

bool fman_update(struct fman* fman, char* error_message)
{
    fman->cursor_y = fman->pattern_matches[fman->match_cursor_y];

    if (!fs_update(&fman->fs, error_message)) {
        return false;
    }

    fman->pattern_matches_len = 0;

    bool contains_cursor = false;

    for (size_t i = 0; i < fman->fs.entries_len; ++i) {
        struct fs_entry entry = fman->fs.entries[i];

        if (!pattern_matches(fman->pattern, &fman->fs.path_arena[entry.path_idx])) {
            continue;
        }

        fman->pattern_matches[fman->pattern_matches_len++] = i;

        if (i != fman->cursor_y) {
            continue;
        }

        contains_cursor = true;
        fman->match_cursor_y = fman->pattern_matches_len - 1;
        
        if (fman->scroll_y + fman_fs_window_height(fman) < fman->match_cursor_y) {
            fman->scroll_y = fman->match_cursor_y - fman_fs_window_height(fman) + 1;
        }
    }

    if (!contains_cursor) {
        fman->match_cursor_y = 0;
        fman->cursor_y = 0;
    }

    if (fman->scroll_y > fman->pattern_matches_len) {
        if (fman->pattern_matches_len > fman_fs_window_height(fman)) {
            fman->scroll_y = fman->pattern_matches_len - fman_fs_window_height(fman);
        } else {
            fman->scroll_y = 0;
        }
    }

    return true;
}

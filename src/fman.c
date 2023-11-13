#include "fman.h"
#include "fs.h"
#include "pattern.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>

bool fman_init(struct fman* fman, char* error_message)
{
    fman->mode = E_mode_normal;

    fman->pattern_matches_len = 0;

    fman->screen_width = 0;
    fman->screen_height = 0;

    fman->cursor_y = 0;
    fman->match_cursor_y = 0;
    fman->scroll_y = 0;

    if (!fs_init(&fman->fs, error_message)) {
        return false;
    }

    return true;
}

void fman_dealloc(struct fman* fman)
{
    fs_dealloc(&fman->fs);
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
    if (fman->match_cursor_y + 1 < fman->pattern_matches_len) {
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

    if (entry.type != E_entry_folder) {
        strcpy(error_message, "TODO: interacting with files is not implemented yet");
        return false;
    }

    if (!fs_chdir(&fman->fs, &fman->fs.path_arena[entry.path_idx], error_message)) {
        return false;
    }

    fman->match_cursor_y = 0;
    fman->scroll_y = 0;

    return true;
}

bool fman_go_back_dir(struct fman* fman, char* error_message)
{
    fman->pattern[0] = '\0';

    return fs_chdir(&fman->fs, "..", error_message);
}

void fman_pattern_add_char(struct fman* fman, const char ch)
{
    const size_t len = strlen(fman->pattern);

    if (len + 1 < PATH_MAX) {
        fman->pattern[len] = ch;
        fman->pattern[len + 1] = '\0';
    }
}

void fman_pattern_delete_char(struct fman* fman)
{
    const size_t len = strlen(fman->pattern);

    if (len > 0) {
        fman->pattern[len - 1] = '\0';
    }
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

        if (pattern_matches(fman->pattern, &fman->fs.path_arena[entry.path_idx])) {
            fman->pattern_matches[fman->pattern_matches_len++] = i;

            if (i == fman->cursor_y) {
                contains_cursor = true;
                fman->match_cursor_y = fman->pattern_matches_len - 1;
                
                if (fman->scroll_y + fman_fs_window_height(fman) < fman->match_cursor_y) {
                    fman->scroll_y = fman->match_cursor_y - fman_fs_window_height(fman) + 1;
                }
            }
        }
    }

    if (!contains_cursor) {
        fman->match_cursor_y = 0;
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


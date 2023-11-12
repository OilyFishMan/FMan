#include "fman.h"
#include "fs.h"
#include "pattern.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>

bool state_init(struct fman_state* state, char* error_message)
{
    state->mode = E_mode_normal;

    state->pattern_matches_len = 0;

    state->screen_width = 0;
    state->screen_height = 0;

    state->cursor_y = 0;
    state->match_cursor_y = 0;
    state->scroll_y = 0;

    if (!fs_init(&state->fs, error_message)) {
        return false;
    }

    return true;
}

void state_dealloc(struct fman_state* state)
{
    fs_dealloc(&state->fs);
}

void state_move_up(struct fman_state* state)
{
    if (state->match_cursor_y > 0) {
        if (state->match_cursor_y == state->scroll_y) {
            --state->scroll_y;
        }

        --state->match_cursor_y;
    }
}

void state_move_down(struct fman_state* state)
{
    if (state->match_cursor_y + 1 < state->pattern_matches_len) {
        ++state->match_cursor_y;

        if (state->match_cursor_y - state->scroll_y >= state_fs_window_height(state)) {
            ++state->scroll_y;
        }
    }
}

void state_move_screen_up(struct fman_state* state)
{
    if (state->match_cursor_y + 1 >= state->screen_height) {
        state->match_cursor_y -= state_fs_window_height(state);
    } else {
        state->match_cursor_y = 0;
    }

    state->scroll_y = state->match_cursor_y;
}

void state_move_screen_down(struct fman_state* state)
{
    if (state->pattern_matches_len >= state->screen_height
     && state->match_cursor_y + state_fs_window_height(state) <= state->pattern_matches_len - 1) {
        state->match_cursor_y += state_fs_window_height(state);
    } else {
        state->match_cursor_y = state->pattern_matches_len - 1;
    }

    if (state->match_cursor_y - state->scroll_y >= state_fs_window_height(state)) {
        state->scroll_y = state->match_cursor_y - state_fs_window_height(state) + 1;
    }
}

bool state_interact(struct fman_state* state, char* error_message)
{
    state->pattern[0] = '\0';

    if (state->pattern_matches_len == 0) {
        return true;
    }

    struct fs_entry entry = state->fs.entries[state->pattern_matches[state->match_cursor_y]];

    if (entry.type != E_entry_folder) {
        strcpy(error_message, "TODO: interacting with files is not implemented yet");
        return false;
    }

    if (!fs_chdir(&state->fs, &state->fs.path_arena[entry.path_idx], error_message)) {
        return false;
    }

    state->match_cursor_y = 0;
    state->scroll_y = 0;

    return true;
}

bool state_go_back_dir(struct fman_state* state, char* error_message)
{
    state->pattern[0] = '\0';

    return fs_chdir(&state->fs, "..", error_message);
}

void state_pattern_add_char(struct fman_state* state, const char ch)
{
    const size_t len = strlen(state->pattern);

    if (len + 1 < PATH_MAX) {
        state->pattern[len] = ch;
        state->pattern[len + 1] = '\0';
    }
}

void state_pattern_delete_char(struct fman_state* state)
{
    const size_t len = strlen(state->pattern);

    if (len > 0) {
        state->pattern[len - 1] = '\0';
    }
}

bool state_update(struct fman_state* state, char* error_message)
{
    state->cursor_y = state->pattern_matches[state->match_cursor_y];

    if (!fs_update(&state->fs, error_message)) {
        return false;
    }

    state->pattern_matches_len = 0;

    bool contains_cursor = false;

    for (size_t i = 0; i < state->fs.entries_len; ++i) {
        struct fs_entry entry = state->fs.entries[i];

        if (pattern_matches(state->pattern, &state->fs.path_arena[entry.path_idx])) {
            state->pattern_matches[state->pattern_matches_len++] = i;

            if (i == state->cursor_y) {
                contains_cursor = true;
                state->match_cursor_y = state->pattern_matches_len - 1;
                
                if (state->scroll_y + state_fs_window_height(state) < state->match_cursor_y) {
                    state->scroll_y = state->match_cursor_y - state_fs_window_height(state) + 1;
                }
            }
        }
    }

    if (!contains_cursor) {
        state->match_cursor_y = 0;
    }

    if (state->scroll_y > state->pattern_matches_len) {
        if (state->pattern_matches_len > state_fs_window_height(state)) {
            state->scroll_y = state->pattern_matches_len - state_fs_window_height(state);
        } else {
            state->scroll_y = 0;
        }
    }

    return true;
}


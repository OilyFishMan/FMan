#include "fman.h"
#include "fs.h"

#define __STDC_WANT_LIB_EXT1__ 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <curses.h>
#include <time.h>

#define KEY_ESCAPE 27
#define MAX_FILE_NAME_DISPLAY 20

void fman_public_update(struct fman_state* state)
{
    getmaxyx(stdscr, state->screen_height, state->screen_width);
}

void fman_render(struct fman_state* state)
{
    const size_t fs_window_start = state->screen_height - fman_fs_window_height(state) - 1;

    erase();

    struct tm broken_up_time;
    char time_fmt[128];

    asctime_r(gmtime_r(&state->fs.last_updated, &broken_up_time), time_fmt);

    mvprintw(0, 0, "[folder name: %s]", state->fs.path);
    mvprintw(1, 0, "[last updated: %.*s]", (int) (strlen(time_fmt) - 1), time_fmt);

    for (size_t i = 0; i < fman_fs_window_height(state) && i + state->scroll_y < state->pattern_matches_len; ++i) {
        struct fs_entry* entry = &state->fs.entries[state->pattern_matches[i + state->scroll_y]];

        asctime_r(gmtime_r(&entry->last_updated, &broken_up_time), time_fmt);

        const char* path_str = &state->fs.path_arena[entry->path_idx];
        const char* folder_end = entry->type == E_entry_folder ? "/" : "";
        const char* dots = "..";

        if (strlen(path_str) + strlen(folder_end) <= MAX_FILE_NAME_DISPLAY) {
            mvprintw((int) (i + fs_window_start), 0, "%s%s", path_str, folder_end);
        } else {
            mvprintw( (int) (i + fs_window_start), 0, "%.*s%s%s"
                    , (int) (MAX_FILE_NAME_DISPLAY - strlen(folder_end) - strlen(dots))
                    , path_str
                    , dots
                    , folder_end);
        }

        mvprintw((int) (i + fs_window_start), MAX_FILE_NAME_DISPLAY + 1, "%s", time_fmt);
    }

    for (size_t j = state->pattern_matches_len - state->scroll_y; j + 1 < state->screen_height; ++j) {
        mvprintw((int) (j + fs_window_start), 0, "~");
    }

    mvprintw((int) (state->screen_height - 1), 0, "pattern: %s", state->pattern);

    switch (state->mode) {
        case E_mode_normal: {
            move(state->match_cursor_y - state->scroll_y + fs_window_start, 0);
            break;
        }

        case E_mode_typing: {
            // we should already be at the end of the "pattern" line,
            // as that was where we were last typing
            break;
        }

        default: {
            break;
        }
    }

    refresh();
}

bool event_normal_mode(struct fman_state* state, const int ch, char* error_message)
{
    switch (ch) {
        case KEY_ESCAPE: {
            state->mode = E_mode_stopped;
            break;
        }

        case KEY_UP: {
            fman_move_up(state);
            break;
        }

        case KEY_DOWN: {
            fman_move_down(state);
            break;
        }

        case KEY_LEFT: {
            if (!fman_go_back_dir(state, error_message)) {
                return false;
            }

            break;
        }

        case 'w': {
            fman_move_screen_up(state);
            break;
        }

        case 's': {
            fman_move_screen_down(state);
            break;
        }

        case '\n': {
            if (!fman_interact(state, error_message)) {
                return false;
            }
            break;
        }

        case 'f': {
            state->mode = E_mode_typing;
            break;
        }

        default: {
            break;
        }
    }

    return true;
}

void event_typing_mode(struct fman_state* state, const int ch)
{
    switch (ch) {
        case KEY_ESCAPE:
        case '\n': {
            state->mode = E_mode_normal;
            break;
        }

        case KEY_BACKSPACE: {
            fman_pattern_delete_char(state);
            break;
        }

        case ERR: {
            break;
        }

        default: {
            fman_pattern_add_char(state, ch);
            break;
        }
    }
}

int main(void)
{
    char error_message[MAX_ERROR_MSG_LEN] = {0};
    bool exit_success = true;

    initscr();

    cbreak();
    noecho();
    keypad(stdscr, true);
    timeout(100);
    set_escdelay(10);

    struct fman_state state = {0};

    if (!fman_init(&state, error_message)) {
        exit_success = false;
        goto cleanup;
    }

    while (state.mode != E_mode_stopped) {
        fman_public_update(&state);

        if (!fman_update(&state, error_message)) {
            exit_success = false;
            goto cleanup_state;
        }

        fman_render(&state);

        int ch = getch();

        switch (state.mode) {
            case E_mode_normal: {
                if (!event_normal_mode(&state, ch, error_message)) {
                    exit_success = false;
                    goto cleanup_state;
                }

                break;
            }

            case E_mode_typing: {
                event_typing_mode(&state, ch);
                break;
            }

            default: {
                break;
            }
        }
    }

cleanup_state:
    fman_dealloc(&state);

cleanup:
    endwin();

    if (exit_success) {
        fprintf(stderr, "the program exited successfully.\n");

        return EXIT_SUCCESS;
    }

    fprintf(stderr, "the program crashed:\t`%s`\n", error_message);
    return EXIT_FAILURE;
}

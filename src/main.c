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

void fman_public_update(struct fman* fman)
{
    getmaxyx(stdscr, fman->screen_height, fman->screen_width);
}

void fman_render(struct fman* fman)
{
    const size_t fs_window_start = fman->screen_height - fman_fs_window_height(fman) - 1;

    erase();

    struct tm broken_up_time;
    char time_fmt[128]; // arbitrary

    asctime_r(gmtime_r(&fman->fs.last_updated, &broken_up_time), time_fmt);

    mvprintw(0, 0, "%s", fman->fs.path);
    mvprintw(1, 0, "%.*s", (int) (strlen(time_fmt) - 1), time_fmt);

    for ( size_t i = 0
        ; i < fman_fs_window_height(fman) && i + fman->scroll_y < fman->pattern_matches_len
        ; ++i) {
        struct fs_entry* entry = &fman->fs.entries[fman->pattern_matches[i + fman->scroll_y]];

        asctime_r(gmtime_r(&entry->last_updated, &broken_up_time), time_fmt);

        const char* path_str = &fman->fs.path_arena[entry->path_idx];
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

    for ( size_t j = fman->pattern_matches_len - fman->scroll_y
        ; j + fs_window_start < fman->screen_height - 1
        ; ++j) {
        mvprintw((int) (j + fs_window_start), 0, "~");
    }

    switch (fman->mode) {
        case E_mode_normal: {
            mvprintw((int) (fman->screen_height - 1), 0, "-- PRESS ? TO SELECT COMMAND --");
            move(fman->match_cursor_y - fman->scroll_y + fs_window_start, 0);
            break;
        }

        case E_mode_typing: {
            mvprintw((int) (fman->screen_height - 1), 0, "%s %s", command_string[fman->command], fman->typing_text);
            break;
        }

        default: {
            break;
        }
    }

    refresh();
}

bool event_normal_mode(struct fman* fman, const int ch, char* error_message)
{
    switch (ch) {
        case KEY_ESCAPE: {
            fman->mode = E_mode_stopped;
            break;
        }

        case KEY_UP: {
            fman_move_up(fman);
            break;
        }

        case KEY_DOWN: {
            fman_move_down(fman);
            break;
        }

        case KEY_LEFT: {
            if (!fman_go_back_dir(fman, error_message)) {
                return false;
            }

            break;
        }

        case 'w': {
            fman_move_screen_up(fman);
            break;
        }

        case 's': {
            fman_move_screen_down(fman);
            break;
        }

        case '\n': {
            if (!fman_interact(fman, error_message)) {
                return false;
            }
            break;
        }

        case '?': {
            fman_begin_command(fman, E_command_select_command);
            break;
        }

        default: {
            break;
        }
    }

    return true;
}

bool event_typing_mode(struct fman* fman, const int ch, char* error_message)
{
    switch (ch) {
        case KEY_BACKSPACE: {
            fman_delete_char(fman);
            break;
        }
        
        case KEY_ESCAPE: {
            fman->mode = E_mode_normal;
            break;
        }

        case '\n': {
            if (!fman_submit_command(fman, error_message)) {
                return false;
            }
            break;
        }

        case ERR: {
            break;
        }

        default: {
            fman_type_char(fman, ch);
            break;
        }
    }

    return true;
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
    set_escdelay(0);

    struct fman fman = {0};

    if (!fman_init(&fman, error_message)) {
        exit_success = false;
        goto cleanup;
    }

    while (fman.mode != E_mode_stopped) {
        fman_public_update(&fman);

        if (!fman_update(&fman, error_message)) {
            exit_success = false;
            goto cleanup_fman;
        }

        fman_render(&fman);

        int ch = getch();

        switch (fman.mode) {
            case E_mode_normal: {
                if (!event_normal_mode(&fman, ch, error_message)) {
                    exit_success = false;
                    goto cleanup_fman;
                }
                break;
            }

            case E_mode_typing: {
                if (!event_typing_mode(&fman, ch, error_message)) {
                    exit_success = false;
                    goto cleanup_fman;
                }
                break;
            }

            default: {
                break;
            }
        }
    }

cleanup_fman:
    fman_dealloc(&fman);

cleanup:
    endwin();

    if (exit_success) {
        fprintf(stderr, "the program exited successfully.\n");

        return EXIT_SUCCESS;
    }

    fprintf(stderr, "the program crashed:\t`%s`\n", error_message);
    return EXIT_FAILURE;
}

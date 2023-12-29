#include "fman.h"
#include "fs.h"

#define __STDC_WANT_LIB_EXT1__ 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <curses.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#define KEY_ESCAPE 27
#define MAX_FILE_NAME_DISPLAY 20

double get_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (double) ts.tv_sec + 1e-9 * (double) ts.tv_nsec;
}

#define fixed_delta (1.0/60.0)

void fman_public_update(struct fman* fman)
{
    getmaxyx(stdscr, fman->screen_height, fman->screen_width);
}

void fman_render_fs(struct fman* fman)
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

void fman_render_editor(struct fman* fman)
{
    erase();

    size_t screen_top_offset = 0;
    size_t line_begin = 0;

    bool cursor_there = false;
    size_t target_x = 0;
    size_t target_y = 0;

    const size_t first_line = fman->editor.screen_y;

    for (size_t i = 0; i < first_line; ++i) {
        if (buffer_findr_char(&fman->editor.buffer, '\n', line_begin, &line_begin)) {
            ++line_begin;
        } else {
            line_begin = fman->editor.buffer.text_len;
        }
    }

    for (size_t line = first_line; line_begin <= fman->editor.buffer.text_len && screen_top_offset < fman->screen_height; ++line) {
        size_t line_end;

        if (!buffer_findr_char(&fman->editor.buffer, '\n', line_begin, &line_end)) {
            line_end = fman->editor.buffer.text_len;
        }

        mvprintw(screen_top_offset, 0, "%zu%*s ", line, ((int) log10(fman->screen_height + first_line) + 1) - (line == 0 ? 1 : (int)log10(line) + 1), "");

        if (fman->editor.pos >= line_begin && fman->editor.pos <= (size_t) line_end) {
            int x_pos;
            int y_pos;
            getyx(stdscr, y_pos, x_pos);
            (void) y_pos;

            size_t x = 0;
            for (size_t i = line_begin; i < fman->editor.pos; ++i) {
                if (fman->editor.buffer.text[i] == '\t') {
                    x += 4;
                } else {
                    ++x;
                }
            }
            target_x = x % fman->screen_width + (size_t) x_pos;
            target_y = screen_top_offset + x / fman->screen_width;
            cursor_there = true;
        }

        // because ncurses displays tabs weird, I have to do this
        for (size_t i = line_begin; i < line_end; ++i) {
            const char ch = fman->editor.buffer.text[i];
            if (ch == '\t') {
                printw("    ");
            } else {
                printw("%c", ch);
            }
        }

        screen_top_offset += (line_end - line_begin) / fman->screen_width + 1;
        line_begin = line_end + 1;
    }

    if (cursor_there) {
        curs_set(1);
        move(target_y, target_x);
    } else {
        curs_set(0);
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

bool event_editing_mode(struct fman* fman, const int ch, char* error_message)
{
    switch (ch) {
        case 27: {
            if (!editor_update(&fman->editor, error_message)) {
                return false;
            }

            fman->mode = E_mode_normal;
            break;
        }

        case KEY_UP: {
            editor_move_up(&fman->editor);
            break;
        }

        case KEY_DOWN: {
            editor_move_down(&fman->editor, fman->screen_height);
            break;
        }

        case KEY_LEFT: {
            editor_move_left(&fman->editor);
            break;
        }

        case KEY_RIGHT: {
            editor_move_right(&fman->editor, fman->screen_height);
            break;
        }

        case KEY_BACKSPACE: {
            editor_backspace(&fman->editor);
            break;
        }

        case ERR: {
            break;
        }

        default: {
            if (!editor_insert_char(&fman->editor, ch, error_message)) {
                return false;
            }
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
        const double start_time = get_time();

        fman_public_update(&fman);

        if (!fman_update(&fman, error_message)) {
            exit_success = false;
            goto cleanup_fman;
        }

        if (fman.mode == E_mode_buffer_edit) {
            fman_render_editor(&fman);
        } else {
            fman_render_fs(&fman);
        }

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

            case E_mode_buffer_edit: {
                if (!event_editing_mode(&fman, ch, error_message)) {
                    exit_success = false;
                    goto cleanup_fman;
                }
                break;
            }

            default: {
                break;
            }
        }

        const double end_time = get_time();
        double delta = end_time - start_time;
        if (delta < fixed_delta) {
            usleep((fixed_delta - delta) * 1e+6);
            delta = fixed_delta;
        }
    }

cleanup_fman:
    fman_delete(&fman);

cleanup:
    endwin();

    if (exit_success) {
        fprintf(stderr, "the program exited successfully.\n");

        return EXIT_SUCCESS;
    }

    fprintf(stderr, "the program crashed:\t`%s`\n", error_message);
    return EXIT_FAILURE;
}

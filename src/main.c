#include "fimale.h"
#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <curses.h>

#define KEY_ESCAPE 27

// Because some people have a problem with global variables.
// Note: please put this in a separate library later,
// and have rendering managed by the library's user (inversion of control).
struct state
{
    size_t screen_width, screen_height;
    size_t cursor_y;

    struct fs fs;
};

int state_init(struct state* state, char error_message[MAX_ERROR_MSG_LEN])
{
    state->cursor_y = 0;

    state->fs = fs_default;

    if (!fs_init(&state->fs, error_message)) {
        return false;
    }

    getmaxyx(stdscr, state->screen_height, state->screen_width);

    return true;
}

void state_move_up(struct state* state)
{
    if (state->cursor_y > 0) {
        --state->cursor_y;
    }
}

void state_move_down(struct state* state)
{
    if (state->cursor_y + 1 < state->fs.objects_len) {
        ++state->cursor_y;
    }
}

int state_interact(struct state* state, char error_message[MAX_ERROR_MSG_LEN])
{
    struct fs_object object = state->fs.objects[state->cursor_y];

    if (object.type != E_object_folder) {
        strcpy(error_message, "TODO: interacting with files is not implemented yet");
        return false;
    }

    if (!fs_chdir(&state->fs, object.path, error_message)) {
        return false;
    }

    if (!fs_reload(&state->fs, error_message)) {
        return false;
    }

    state->cursor_y = 0;

    return true;
}

void state_render(struct state* state)
{
    clear();

    char file_buf[PATH_MAX];

    realpath(".", file_buf);

    mvprintw(0, 0, "You are in [%s]", file_buf);

    const size_t fs_window_height = state->screen_height - 1;
    const size_t top_fs_object = state->cursor_y / fs_window_height * fs_window_height;
    const size_t screen_y = state->cursor_y % fs_window_height;

    for (size_t i = 0; i < fs_window_height; ++i) {
        struct fs_object object = state->fs.objects[i + top_fs_object];
        mvprintw((int) i + 1, 0, "%s%s", object.path, object.type == E_object_folder ? "/" : "");
    }

    move(screen_y + 1, 0);
}

int main(void)
{
    char error_message[MAX_ERROR_MSG_LEN] = {0};
    bool exit_success = true;

    initscr();

    cbreak();
    noecho();
    keypad(stdscr, true);
    set_escdelay(100);

    struct state state;

    if (state_init(&state, error_message) != 0) {
        exit_success = false;
        goto loop_end;
    }

    for (;;) {
        state_render(&state);

        int ch = getch();

        switch (ch) {
            case KEY_ESCAPE: {
                goto loop_end;
                break;
            }

            case KEY_UP: {
                state_move_up(&state);
                break;
            }

            case KEY_DOWN: {
                state_move_down(&state);
                break;
            }

            case '\n': {
                if (state_interact(&state, error_message) != 0) {
                    exit_success = false;
                    goto loop_end;
                }
                break;
            }

            default: {
                break;
            }
        }
    }
loop_end:

    endwin();

    if (exit_success) {
        return EXIT_SUCCESS;
    }

    fprintf(stderr, "the program crashed:\t`%s`\n", error_message);

    return EXIT_FAILURE;
}

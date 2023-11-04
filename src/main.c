#include "fimale.h"
#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <limits.h>
#include <curses.h>

#define KEY_ESCAPE 27

// Because some people have a problem with global variables.
// Note: please put this in a separate library later,
// and have rendering managed by the library's user (inversion of control).
struct state
{
    size_t screen_width, screen_height;
    size_t cursor_y;
    size_t scroll_y;

    struct fs fs;
};

bool state_init(struct state* state, char* error_message)
{
    bool exit_success = true;

    state->screen_width = 100;
    state->screen_height = 100;

    state->cursor_y = 0;
    state->scroll_y = 0;

    state->fs = fs_default;

    if (!fs_init(&state->fs, error_message)) {
        exit_success = false;
        goto cleanup;
    }

//remember to add this back in later
//
//cleanup_fs:
//    fs_dealloc(&state->fs);

cleanup:
    return exit_success;
}

size_t state_fs_window_height(struct state* state)
{
    return state->screen_height - 1;
}

void state_dealloc(struct state* state)
{
    fs_dealloc(&state->fs);
}

void state_move_up(struct state* state)
{
    if (state->cursor_y > 0) {
        --state->cursor_y;
    }

    if (state->cursor_y == state->scroll_y - 1) {
        --state->scroll_y;
    }
}

void state_move_down(struct state* state)
{
    if (state->cursor_y + 1 < state->fs.objects_len) {
        ++state->cursor_y;
    }

    if (state->cursor_y - state->scroll_y + 1 > state_fs_window_height(state)) {
        ++state->scroll_y;
    }
}

void state_move_screen_up(struct state* state)
{
    if (state->cursor_y + 1 >= state->screen_height) {
        state->cursor_y -= state_fs_window_height(state);
    } else {
        state->cursor_y = 0;
    }

    state->scroll_y = state->cursor_y;
}

void state_move_screen_down(struct state* state)
{
    // compilers are smart, they can do basic math simplifications
    // so i will not distribute these pluses, and leave it unoptimized
    // for readability and easy refactoring in the future
    if (state->fs.objects_len >= state->screen_height
     && state->cursor_y + (state_fs_window_height(state)) <= state->fs.objects_len - 1) {
        state->cursor_y += state_fs_window_height(state);
    } else {
        state->cursor_y = state->fs.objects_len - 1;
    }

    if (state->cursor_y >= (state_fs_window_height(state))) {
        state->scroll_y = state->cursor_y - (state_fs_window_height(state)) + 1;
    } else {
        state->scroll_y = 0;
    }
}

bool state_interact(struct state* state, char* error_message)
{
    struct fs_object object = state->fs.objects[state->cursor_y];

    if (object.type != E_object_folder) {
        strcpy(error_message, "TODO: interacting with files is not implemented yet");
        return false;
    }

    if (!fs_chdir(&state->fs, &state->fs.path_arena[object.real_path_idx], error_message)) {
        return false;
    }

    if (!fs_reload(&state->fs, error_message)) {
        return false;
    }

    state->cursor_y = 0;
    state->scroll_y = 0;

    return true;
}

void state_public_update(struct state* state)
{
    getmaxyx(stdscr, state->screen_height, state->screen_width);
}

void state_private_update(struct state* state)
{
    if (state->cursor_y - state->scroll_y > state_fs_window_height(state)) {
        state->scroll_y = state->cursor_y - state->screen_height / 2;
    }
}

void state_render(struct state* state)
{
    erase();

    mvprintw(0, 0, "You are in [%s]", state->fs.path);

    for (size_t i = 0; i + 1 < state->screen_height; ++i) {
        if (i + state->scroll_y < state->fs.objects_len) {
            struct fs_object object = state->fs.objects[i + state->scroll_y];

            mvprintw((int) i + 1, 0, "%s%s", &state->fs.path_arena[object.path_idx], object.type == E_object_folder ? "/" : "");
        } else {
            mvprintw((int) i + 1, 0, "~");
        }
    }

    move(state->cursor_y - state->scroll_y + 1, 0);

    refresh();
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
    set_escdelay(100);

    struct state state = {0};

    if (!state_init(&state, error_message)) {
        exit_success = false;
        goto cleanup;
    }

    for (;;) {
        state_public_update(&state);
        state_private_update(&state);

        state_render(&state);

        int ch = getch();

        switch (ch) {
            case KEY_ESCAPE: {
                goto cleanup_state;
            }

            case KEY_UP: {
                state_move_up(&state);
                break;
            }

            case KEY_DOWN: {
                state_move_down(&state);
                break;
            }

            case 'w': {
                state_move_screen_up(&state);
                break;
            }

            case 's': {
                state_move_screen_down(&state);
                break;
            }

            case '\n': {
                if (!state_interact(&state, error_message)) {
                    exit_success = false;
                    goto cleanup_state;
                }
                break;
            }

            default: {
                break;
            }
        }
    }


cleanup_state:
    state_dealloc(&state);

cleanup:
    endwin();

    if (exit_success) {
        fprintf(stderr, "the program exited successfully.\n");

        assert(strlen(error_message) == 0);

        return EXIT_SUCCESS;
    }

    fprintf(stderr, "the program crashed:\t`%s`\n", error_message);
    return EXIT_FAILURE;
}

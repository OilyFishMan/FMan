#ifndef FIMALE_H_
#define FIMALE_H_

#include "fs.h"

#include <stddef.h>
#include <limits.h>

#define MAX_ERROR_MSG_LEN 128

enum state_mode
{
    E_mode_stopped,
    E_mode_normal,
    E_mode_typing
};

struct state
{
    enum state_mode mode;

    char pattern[PATH_MAX];

    size_t pattern_matches_len;
    size_t pattern_matches[MAX_FS_ENTRIES];

    size_t screen_width, screen_height;
    size_t cursor_y;
    size_t match_cursor_y;
    size_t scroll_y;

    struct fs fs;
};

bool state_init(struct state* state, char* error_message);
void state_dealloc(struct state* state);

#define state_fs_window_height(state) ((state)->screen_height - 3)

void state_move_up(struct state* state);
void state_move_down(struct state* state);
void state_move_screen_up(struct state* state);
void state_move_screen_down(struct state* state);

bool state_interact(struct state* state, char* error_message);
bool state_go_back_dir(struct state* state, char* error_message);

void state_pattern_add_char(struct state* state, const char ch);
void state_pattern_delete_char(struct state* state);

bool state_update(struct state* state, char* error_message);

#endif // FIMALE_H_

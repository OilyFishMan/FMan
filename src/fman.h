#ifndef FIMALE_H_
#define FIMALE_H_

#include "fs.h"

#include <stddef.h>
#include <limits.h>

#define MAX_ERROR_MSG_LEN 128

enum fman_state_mode
{
    E_mode_stopped,
    E_mode_normal,
    E_mode_typing
};

struct fman_state
{
    enum fman_state_mode mode;

    char pattern[PATH_MAX];

    size_t pattern_matches_len;
    size_t pattern_matches[MAX_FS_ENTRIES];

    size_t screen_width, screen_height;
    size_t cursor_y;
    size_t match_cursor_y;
    size_t scroll_y;

    struct fs fs;
};

bool state_init(struct fman_state* state, char* error_message);
void state_dealloc(struct fman_state* state);

#define state_fs_window_height(state) ((state)->screen_height - 3)

void state_move_up(struct fman_state* state);
void state_move_down(struct fman_state* state);
void state_move_screen_up(struct fman_state* state);
void state_move_screen_down(struct fman_state* state);

bool state_interact(struct fman_state* state, char* error_message);
bool state_go_back_dir(struct fman_state* state, char* error_message);

void state_pattern_add_char(struct fman_state* state, const char ch);
void state_pattern_delete_char(struct fman_state* state);

bool state_update(struct fman_state* state, char* error_message);

#endif // FIMALE_H_

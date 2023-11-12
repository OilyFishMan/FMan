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

bool fman_init(struct fman_state* state, char* error_message);
void fman_dealloc(struct fman_state* state);

#define fman_fs_window_height(state) ((state)->screen_height - 3)

void fman_move_up(struct fman_state* state);
void fman_move_down(struct fman_state* state);
void fman_move_screen_up(struct fman_state* state);
void fman_move_screen_down(struct fman_state* state);

bool fman_interact(struct fman_state* state, char* error_message);
bool fman_go_back_dir(struct fman_state* state, char* error_message);

void fman_pattern_add_char(struct fman_state* state, const char ch);
void fman_pattern_delete_char(struct fman_state* state);

bool fman_update(struct fman_state* state, char* error_message);

#endif // FIMALE_H_

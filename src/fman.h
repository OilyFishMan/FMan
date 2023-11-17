#ifndef FIMALE_H_
#define FIMALE_H_

#include "fs.h"

#include <stddef.h>
#include <limits.h>

#define MAX_ERROR_MSG_LEN 128

enum fman_mode
{
    E_mode_stopped,
    E_mode_normal,
    E_mode_typing
};

enum fman_command
{
    E_command_select_command,
    E_command_find,
    E_command_remove,
    E_command_create_file,
    E_command_create_folder,
};

#define FMAN_COMMANDS_MAX (E_command_create_folder + 1)

static const char* command_string[FMAN_COMMANDS_MAX] = {
    [E_command_select_command] = "?",
    [E_command_find]           = "find",
    [E_command_remove]         = "rm",
    [E_command_create_file]    = "mkf",
    [E_command_create_folder]  = "mkd",
};

struct fman
{
    enum fman_mode mode;
    enum fman_command command;

    char typing_text[PATH_MAX];

    char pattern[PATH_MAX];

    size_t pattern_matches_len;
    size_t pattern_matches[MAX_FS_ENTRIES];

    size_t screen_width, screen_height;

    struct fs fs;

    size_t cursor_y, scroll_y, match_cursor_y;
};

bool fman_init(struct fman* fman, char* error_message);
void fman_dealloc(struct fman* fman);

#define fman_fs_window_height(fman) ((fman)->screen_height - 3)

void fman_move_up(struct fman* fman);
void fman_move_down(struct fman* fman);
void fman_move_screen_up(struct fman* fman);
void fman_move_screen_down(struct fman* fman);

bool fman_interact(struct fman* fman, char* error_message);
bool fman_go_back_dir(struct fman* fman, char* error_message);

void fman_begin_command(struct fman* fman, enum fman_command command);
void fman_type_char(struct fman* fman, const char ch);
void fman_delete_char(struct fman* fman);
bool fman_submit_command(struct fman* fman, char* error_message);

bool fman_update(struct fman* fman, char* error_message);

#endif // FIMALE_H_

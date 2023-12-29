#ifndef EDITOR_H_
#define EDITOR_H_

#include "stupid_buffer.h"

#include <stddef.h>
#include <limits.h>

struct editor
{
    size_t screen_y;
    size_t pos;
    char file_path[PATH_MAX];
    struct buffer buffer;
};

bool editor_init(struct editor* editor, char* error_message);
void editor_delete(struct editor* editor);
bool editor_open(struct editor* editor, char* file_path, char* error_message);
bool editor_update(struct editor* editor, char* error_message);
bool editor_insert_char(struct editor* editor, const char ch, char* error_message);
void editor_backspace(struct editor* editor);
void editor_move_up(struct editor* editor);
void editor_move_down(struct editor* editor, const size_t screen_heigh);
void editor_move_left(struct editor* editor);
void editor_move_right(struct editor* editor, const size_t screen_height);

#endif // EDITOR_H_

#include "editor.h"
#include "stupid_buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

static size_t get_line(struct editor* editor, const size_t pos)
{
    size_t harvest = 0;

    for (size_t i = 0; i < pos; ++i) {
        if (editor->buffer.text[i] == '\n') {
            ++harvest;
        }
    }

    return harvest;
}

bool editor_init(struct editor* editor, char* error_message)
{
    editor->screen_y = 0;
    editor->pos = 0;
    editor->file_path[0] = '\0';
    return buffer_init(&editor->buffer, 0, "", error_message);
}

void editor_delete(struct editor* editor)
{
    buffer_delete(&editor->buffer);
}

bool editor_open(struct editor* editor, char* file_path, char* error_message)
{
    editor->screen_y = 0;
    editor->pos = 0;

    bool success = true;

    memcpy(editor->file_path, file_path, sizeof(*editor->file_path) * PATH_MAX);
    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        strcpy(error_message, strerror(errno));
        success = false;
        goto end;
    }

    fseek(file, 0, SEEK_END);
    const size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* text = malloc(sizeof(*text) * size);
    if (text == NULL) {
        strcpy(error_message, strerror(errno));
        success = false;
        goto cleanup_file;
    }

    if (fread(text, size, 1, file) < 0) {
        strcpy(error_message, strerror(errno));
        success = false;
        goto cleanup_text;
    }

    editor->buffer.text_len = 0;

    if (!buffer_add_text(&editor->buffer, 0, size, text, error_message)) {
        success = false;
        goto cleanup_text;
    }

cleanup_text:
    free(text);

cleanup_file:
    fclose(file);

end:
    return success;
}

bool editor_update(struct editor* editor, char* error_message)
{
    bool success = true;

    FILE* file = fopen(editor->file_path, "w");
    if (file == NULL) {
        strcpy(error_message, strerror(errno));
        success = false;
        goto end;
    }

    if (fwrite(editor->buffer.text, editor->buffer.text_len, 1, file) < 0) {
        strcpy(error_message, strerror(errno));
        success = false;
        goto cleanup_file;
    }

cleanup_file:
    fclose(file);

end:
    return success;
}

bool editor_insert_char(struct editor* editor, const char ch, char* error_message)
{
    if (!buffer_add_text(&editor->buffer, editor->pos, 1, &ch, error_message)) {
        return false;
    }
    ++editor->pos;
    return true;
}

void editor_backspace(struct editor* editor)
{
    if (editor->pos > 0) {
        editor_move_left(editor);
        buffer_delete_text(&editor->buffer, editor->pos, 1);
    }
}

void editor_move_up(struct editor* editor)
{
    size_t line_begin;
    if (editor->pos >= 1 && buffer_findl_char(&editor->buffer, '\n', editor->pos - 1, &line_begin)) {
        ++line_begin;
    } else {
        line_begin = 0;
    }

    const size_t x_pos = editor->pos - line_begin;

    size_t prev_line_begin;
    if (line_begin >= 2 && buffer_findl_char(&editor->buffer, '\n', line_begin - 2, &prev_line_begin)) {
        prev_line_begin += 1;
    } else {
        prev_line_begin = 0;
    }

    if (prev_line_begin == line_begin) {
        return;
    }

    editor->pos = prev_line_begin + x_pos;
    if (editor->pos >= line_begin) {
        editor->pos = line_begin - 1;
    }
    if (get_line(editor, editor->pos) < editor->screen_y) {
        --editor->screen_y;
    }
}

void editor_move_down(struct editor* editor, const size_t screen_height)
{
    size_t line_begin;
    if (editor->pos >= 1 && buffer_findl_char(&editor->buffer, '\n', editor->pos - 1, &line_begin)) {
        ++line_begin;
    } else {
        line_begin = 0;
    }

    const size_t x_pos = editor->pos - line_begin;

    size_t line_end;
    if (!buffer_findr_char(&editor->buffer, '\n', editor->pos, &line_end)) {
        line_end = editor->buffer.text_len;
    }

    size_t next_line_end;
    if (!buffer_findr_char(&editor->buffer, '\n', line_end + 1, &next_line_end)) {
        next_line_end = editor->buffer.text_len;
    }

    if (next_line_end != line_end) {
        editor->pos = line_end + x_pos + 1;
        if (editor->pos > next_line_end) {
            editor->pos = next_line_end;
        }
        if (get_line(editor, editor->pos) >= editor->screen_y + screen_height) {
            ++editor->screen_y;
        }
    }
}

void editor_move_left(struct editor* editor)
{
    if (editor->pos > 0) {
        --editor->pos;
        if (get_line(editor, editor->pos) < editor->screen_y) {
            --editor->screen_y;
        }
    }
}

void editor_move_right(struct editor* editor, const size_t screen_height)
{
    if (editor->pos < editor->buffer.text_len) {
        ++editor->pos;
        if (get_line(editor, editor->pos) >= editor->screen_y + screen_height) {
            ++editor->screen_y;
        }
    }
}

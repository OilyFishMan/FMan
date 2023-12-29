#include "stupid_buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>

#define default_cap 1024

bool buffer_init( struct buffer* buffer
                , const size_t text_len
                , const char* text
                , char* error_message)
{
    buffer->text_len = 0;
    buffer->text_cap = 0;
    buffer->text = NULL;

    return buffer_add_text(buffer, 0, text_len, text, error_message);
}

void buffer_delete(struct buffer* buffer)
{
    free(buffer->text);
}

bool buffer_add_text( struct buffer* buffer
                    , const size_t pos
                    , const size_t text_len
                    , const char* text
                    , char* error_message)
{

    buffer->text_len += text_len;

    if (buffer->text_len > buffer->text_cap) {
        if (buffer->text_cap == 0) {
            buffer->text_cap = default_cap;
        }

        while (buffer->text_len > buffer->text_cap) {
            buffer->text_cap *= 2;
        }

        char* new_text = realloc(buffer->text, sizeof(*new_text) * buffer->text_cap);

        if (new_text == NULL) {
            strcpy(error_message, strerror(errno));
            return false;
        }
        
        buffer->text = new_text;
    }

    memcpy( &buffer->text[pos + text_len]
          , &buffer->text[pos]
          , buffer->text_len - pos - text_len);

    memcpy(&buffer->text[pos], text, text_len);

    return true;
}

void buffer_delete_text( struct buffer* buffer
                       , const size_t pos
                       , const size_t text_len)
{
    if (pos >= buffer->text_len) {
        return;
    }

    const size_t amount = pos + text_len > buffer->text_len
        ? buffer->text_len - pos
        : text_len;

    memmove(&buffer->text[pos], &buffer->text[pos + amount], buffer->text_len - amount - pos);

    buffer->text_len -= amount;
}

bool buffer_findl_char( struct buffer* buffer
                      , const char ch
                      , const size_t pos
                      , size_t* harvest)
{
    for (size_t i = pos + 1; i > 0; --i) {
        if (buffer->text[i - 1] == ch) {
            *harvest = i - 1;
            return true;
        }
    }

    return false;
}

bool buffer_findr_char( struct buffer* buffer
                     , const char ch
                     , const size_t pos
                     , size_t* harvest)
{
    for (size_t i = pos; i < buffer->text_len; ++i) {
        if (buffer->text[i] == ch) {
            *harvest = i;
            return true;
        }
    }

    return false;
}

char buffer_char_at(struct buffer* buffer, const size_t pos)
{
    return buffer->text[pos];
}

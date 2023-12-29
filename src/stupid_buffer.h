#ifndef STUPID_BUFFER_H_
#define STUPID_BUFFER_H_

#include <stddef.h>
#include <stdbool.h>

struct buffer
{
    size_t text_len;
    size_t text_cap;
    char* text;
};

bool buffer_init( struct buffer* buffer
                , const size_t text_len
                , const char* text
                , char* error_message);
void buffer_delete(struct buffer* buffer);
bool buffer_add_text( struct buffer* buffer
                    , const size_t pos
                    , const size_t text_len
                    , const char* text
                    , char* error_message);
void buffer_delete_text( struct buffer* buffer
                       , const size_t pos
                       , const size_t text_len);
bool buffer_findl_char( struct buffer* buffer
                      , const char ch
                      , const size_t pos
                      , size_t* harvest);
bool buffer_findr_char( struct buffer* buffer
                      , const char ch
                      , const size_t pos
                      , size_t* harvest);
char buffer_char_at(struct buffer* buffer, const size_t pos);

#endif // STUPID_BUFFER_H_
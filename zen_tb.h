/*

 == Single Header Library for a text editor buffer 
 
 == Open a file an create an row, line type buffer
 
*/

#ifndef ZEN_TB_H
#define ZEN_TB_H

#include <malloc.h> /* malloc, calloc, realloc, free */
#include <string.h> /* strlen,  memcpy*/

/*  */
#define ZEN_TB_ALLOC(sz)          malloc(sz)
#define ZEN_TB_FREE(ptr)          free(ptr)
#define ZEN_TB_ZEROALLOC(num, sz) calloc(num, sz)
#define ZEN_TB_REALLOC(ptr, sz)   realloc(ptr, sz)

#define ZEN_TB_NULLPTR 0

/* set this to 1 if you assertion */
#define ZEN_TB_DEBUG_MODE 1

/* default line capacity when creating more space */
#define ZEN_TB_DEFAULT_LINE_SIZE 120

/* default rows capacity when creating more space */
#define ZEN_TB_DEFAULT_TB_CAPACITY 100

#if ZEN_TB_DEBUG_MODE 
#define ZEN_TB_ASSERT(expr) if (!(expr)) *(int *)0 = 1
#else
#define ZEN_TB_ASSERT(expr)
#endif

typedef size_t zen_tb_size;
typedef unsigned int zen_tb_bool32;
typedef char * zen_tb_string;

/* STRUCTS */

typedef union zen_tb_character
{
    unsigned int full;
    struct
    {
        unsigned short codepoint;
        unsigned char unused;
        unsigned char color;
    };
} zen_tb_character;

typedef struct zen_tb_header
{
    zen_tb_bool32 parsed;
    zen_tb_size capacity;
    zen_tb_size length;
    unsigned char *meta_string;
} zen_tb_header;

typedef struct zen_tb_text_buffer
{
    u32 cursor_x;
    u32 cursor_y;
    u32 mark_x;
    u32 mark_y;
    zen_tb_bool32 is_dirty;
    zen_tb_size capacity;
    zen_tb_size length;
    zen_tb_size text_range_x_start;
    zen_tb_size text_range_y_start;
    zen_tb_size text_range_x_end;
    zen_tb_size text_range_y_end;
    zen_tb_string filename;
    zen_tb_string fullpath;
    zen_tb_string *rows;
    
} zen_tb_text_buffer;

#if defined(_MSC_VER)
#define zen_inline __inline
#endif

#define ZEN_TB_HEADER(str) ((zen_tb_header *)str - 1)

/* Declarations */

/* LINE */
/* creates an line with a capacity greater or equal to the length of the init cstr */
static zen_tb_string zen_tb_line_create(const char *cstr, zen_tb_size size);

/* free the string allocated */
static void zen_tb_line_destroy(zen_tb_string str);

/* append a string to the end of the line */
static zen_tb_string zen_tb_line_append(zen_tb_string str, const char *str_to_append);

/* insert an char at the nth position in the line */
static zen_tb_string zen_tb_line_insert_char(zen_tb_string str, char c, unsigned int pos);

/* deletes a char at the ntn position in the line */
static char zen_tb_line_delete_char(zen_tb_string str, unsigned int pos);

/* returns the length of the line */
static zen_inline zen_tb_size zen_tb_line_length(zen_tb_string str);

/* returns the maximum capacity of the line */
static zen_inline zen_tb_size zen_tb_line_capacity(zen_tb_string str);

/* sets the length of the line */
static zen_inline void zen_tb_line_set_length(zen_tb_string str, zen_tb_size length);

/* sets the capacity of the line */
static zen_inline void zen_tb_line_set_capacity(zen_tb_string str, zen_tb_size capacity);

static zen_inline unsigned char * zen_tb_line_get_meta_string(zen_tb_string str);

/* ==========  TEXT BUFFER ========== */

/* create a new text edit buffer with capacity number of rows */
static zen_tb_text_buffer * zen_tb_text_buffer_create(zen_tb_size capacity);

/* NOTE:  internal use only, not used to close the text edit buffer, totally, for that we need to
  **            also free each of the lines one by one */
static void zen_tb_text_buffer_destroy(zen_tb_text_buffer *text_buffer);

static void zen_tb_text_buffer_line_delete(zen_tb_text_buffer *text_buffer, zen_tb_size pos);

/* append an line to the text edit buffer */
static zen_tb_text_buffer * zen_tb_text_buffer_append_line(zen_tb_text_buffer *text_buffer, zen_tb_string line);

static zen_tb_string * zen_tb_text_buffer_make_space(zen_tb_text_buffer *text_buffer, zen_tb_size new_size);

/* insert an line into the text buffer at the nth position */
static zen_tb_text_buffer * zen_tb_text_buffer_insert_line(zen_tb_text_buffer *text_buffer, zen_tb_string line, zen_tb_size pos);

/* set the length in lines of the text_buffer */
static zen_inline void zen_tb_text_buffer_set_length(zen_tb_text_buffer *text_buffer, zen_tb_size length);

#endif /* ZEN_TB_H */

/* Definitions */
#if defined(ZEN_TB_IMPLEMENTATION)

static zen_tb_string
zen_tb_line_create(const char *cstr, zen_tb_size size)
{
    zen_tb_size length;
    zen_tb_size allocation_size;
    zen_tb_size capacity;
    
    zen_tb_header *header;
    
    zen_tb_string str;
    
    length = strlen(cstr);
    
    capacity = length > size ? length : size;
    
    allocation_size = sizeof(*header) + capacity + 1;
    
    header = (zen_tb_header *)ZEN_TB_ALLOC(allocation_size);
    
    header->meta_string = (unsigned char *)ZEN_TB_ZEROALLOC(capacity ,sizeof(header->meta_string[0]));;
    
    if (!header) return ZEN_TB_NULLPTR;
    
    if (!header->meta_string) 
    {
        ZEN_TB_FREE(header);
        return ZEN_TB_NULLPTR;
    }
    
    header->length = length;
    header->capacity = capacity;
    header->parsed = false;
    
    str = (zen_tb_string)header + sizeof(*header);
    
    if (length && cstr) memcpy(str, cstr, length);
    
    str[length] =  '\0';
    
    return str;
}

static void
zen_tb_line_destroy(zen_tb_string str)
{
    if (str == ZEN_TB_NULLPTR) return;
    
    ZEN_TB_FREE(ZEN_TB_HEADER(str)->meta_string);
    
    ZEN_TB_FREE(ZEN_TB_HEADER(str));
}

static zen_tb_string
zen_tb_line_insert_char(zen_tb_string str, char c, unsigned int pos)
{
    zen_tb_size str_length;
    zen_tb_size space_left;
    zen_tb_size copy_size;
    
    zen_tb_string new_str;
    
    str_length = zen_tb_line_length(str);
    space_left = zen_tb_line_capacity(str) - str_length;
    
    if (pos > str_length) return str;
    
    if (space_left >= 1)
    {
        copy_size = str_length - pos;
        memcpy(&str[pos + 1], &str[pos], copy_size);
        zen_tb_line_set_length(str, str_length + 1);
        str[pos] = c;
        str[str_length + 1] = '\0';
        return str;
    }
    
    // reallocate space with a fixed amount
    new_str = zen_tb_line_create(str, str_length + ZEN_TB_DEFAULT_LINE_SIZE);
    if (new_str == ZEN_TB_NULLPTR) return str;
    
    copy_size = str_length - pos;
    memcpy(&new_str[pos + 1], &new_str[pos], copy_size);
    
    zen_tb_line_set_length(new_str, str_length + 1);
    
    new_str[pos] = c;
    new_str[str_length + 1] = '\0';
    
    zen_tb_line_destroy(str);
    
    return new_str;
}

static char
zen_tb_line_delete_char(zen_tb_string str, unsigned int pos)
{
    char c;
    zen_tb_size str_length, copy_size;
    
    str_length = zen_tb_line_length(str);
    if (pos > str_length) return ((char)0);
    
    copy_size = str_length - pos;
    
    c = str[pos];
    
    memcpy(&str[pos], &str[pos + 1], copy_size);
    
    zen_tb_line_set_length(str, str_length - 1);
    
    str[str_length - 1] = '\0';
    
    return c;
}

static zen_tb_string
zen_tb_line_append(zen_tb_string str, const char *str_to_append)
{
    zen_tb_size str_length;
    zen_tb_size append_length;
    zen_tb_size space_left;
    zen_tb_string new_str;
    
    str_length = zen_tb_line_length(str);
    
    append_length = strlen(str_to_append);
    
    space_left = zen_tb_line_capacity(str) - str_length;
    
    if (space_left >= append_length)
    {
        memcpy(str + str_length, str_to_append, strlen(str_to_append));
        zen_tb_line_set_length(str, str_length + append_length);
        str[str_length + append_length] = '\0';
        return str;
    }
    
    new_str = zen_tb_line_create("", str_length + append_length);
    
    // just return without appending
    if (new_str == ZEN_TB_NULLPTR) return str;
    
    memcpy(new_str, str, str_length);
    memcpy(new_str + str_length, str_to_append, append_length);
    
    zen_tb_line_set_length(new_str, str_length + append_length);
    new_str[str_length + append_length] = '\0';
    
    zen_tb_line_destroy(str);
    
    str = new_str;
    
    return str;
}

static zen_inline void
zen_tb_line_set_length(zen_tb_string str, zen_tb_size length)
{
    ZEN_TB_HEADER(str)->length = length;
}

static zen_inline void
zen_tb_line_set_capacity(zen_tb_string str, zen_tb_size capacity)
{
    ZEN_TB_HEADER(str)->capacity = capacity;
}

static zen_inline zen_tb_size
zen_tb_line_length(zen_tb_string str)
{
    return ZEN_TB_HEADER(str)->length;
}

static zen_inline zen_tb_size
zen_tb_line_capacity(zen_tb_string str)
{
    return ZEN_TB_HEADER(str)->capacity;
}

static zen_inline unsigned char *
zen_tb_line_get_meta_string(zen_tb_string str)
{
    return ZEN_TB_HEADER(str)->meta_string;
}

static zen_inline zen_tb_bool32
zen_tb_line_been_parsed(zen_tb_string str)
{
    return ZEN_TB_HEADER(str)->parsed;
}

static zen_inline void
zen_tb_line_set_parse_status(zen_tb_string str, zen_tb_bool32 status)
{
    ZEN_TB_HEADER(str)->parsed = status;
}

/* TEXT BUFFER IMPLEMENTATION */
static zen_tb_text_buffer *
zen_tb_text_buffer_create(zen_tb_size capacity)
{
    zen_tb_text_buffer *tb;
    
    tb = (zen_tb_text_buffer *)ZEN_TB_ZEROALLOC(1, sizeof(*tb));
    
    if (!tb) return ZEN_TB_NULLPTR;
    
    tb->rows = (zen_tb_string *)ZEN_TB_ZEROALLOC(capacity + 1, sizeof(tb->rows));
    if (!tb->rows) return ZEN_TB_NULLPTR;
    
    memset(tb->rows, 0, capacity * sizeof(tb->rows));
    
    tb->capacity = capacity;
    
    return tb;
}

static void
zen_tb_text_buffer_destroy(zen_tb_text_buffer *text_buffer)
{
    ZEN_TB_FREE(text_buffer->rows);
    ZEN_TB_FREE(text_buffer);
}

static zen_tb_text_buffer *
zen_tb_text_buffer_append_line(zen_tb_text_buffer *text_buffer, zen_tb_string line)
{
    if (line == ZEN_TB_NULLPTR) return text_buffer;
    
    if ((text_buffer->length + 1) <= text_buffer->capacity)
    {
        memcpy(&text_buffer->rows[text_buffer->length], &line, sizeof(line));
        zen_tb_text_buffer_set_length(text_buffer, text_buffer->length + 1);
        return text_buffer;
    }
    
    // realloc
    text_buffer->rows = zen_tb_text_buffer_make_space(text_buffer, text_buffer->capacity + ZEN_TB_DEFAULT_TB_CAPACITY + 1);
    
    text_buffer->capacity = text_buffer->capacity + ZEN_TB_DEFAULT_TB_CAPACITY;
    
    text_buffer->rows[text_buffer->length] = line;
    
    zen_tb_text_buffer_set_length(text_buffer, text_buffer->length + 1);
    
    return text_buffer;
}

static zen_tb_string *
zen_tb_text_buffer_make_space(zen_tb_text_buffer *text_buffer, zen_tb_size new_size)
{
    zen_tb_string *old_rows;
    zen_tb_size old_length, old_capacity;
    
    old_length = text_buffer->length;
    old_capacity = text_buffer->capacity;
    old_rows = text_buffer->rows;
    
    text_buffer->rows = 
        (zen_tb_string *)ZEN_TB_REALLOC(text_buffer->rows, new_size * sizeof(text_buffer->rows));
    
    if (!text_buffer->rows) return old_rows;
    
    memset(&(text_buffer->rows[old_length]), 0, (new_size - old_length) * sizeof(text_buffer->rows));
    
    return text_buffer->rows;
}

static void
zen_tb_text_buffer_line_delete(zen_tb_text_buffer *text_buffer, zen_tb_size pos)
{
    if (pos >= text_buffer->length) return;
    
    zen_tb_string line = text_buffer->rows[pos];
    
    zen_tb_size copy_size = text_buffer->length - pos;
    
    memcpy(&text_buffer->rows[pos], &text_buffer->rows[pos + 1], copy_size * sizeof(line));
    
    zen_tb_text_buffer_set_length(text_buffer, text_buffer->length - 1);
    
    zen_tb_line_destroy(line);
}

static zen_tb_text_buffer *
zen_tb_text_buffer_insert_line(zen_tb_text_buffer *text_buffer, zen_tb_string line, zen_tb_size pos)
{
    zen_tb_size copy_size;
    
    if (pos >= text_buffer->length)
    {
        text_buffer = zen_tb_text_buffer_append_line(text_buffer, line);
        return text_buffer;
    }
    
    if ((text_buffer->length + 1) <= text_buffer->capacity)
    {
        copy_size = (text_buffer->length - pos) * sizeof(line);
        memcpy(&text_buffer->rows[pos + 1], &text_buffer->rows[pos], copy_size);
        
        text_buffer->rows[pos] = line;
        
        zen_tb_text_buffer_set_length(text_buffer, text_buffer->length + 1);
        
        return text_buffer;
    }
    
    // realloc
    text_buffer->rows = zen_tb_text_buffer_make_space(text_buffer, text_buffer->capacity + ZEN_TB_DEFAULT_TB_CAPACITY + 1);
    
    text_buffer->capacity = text_buffer->capacity + ZEN_TB_DEFAULT_TB_CAPACITY;
    
    copy_size = (text_buffer->length - pos) * sizeof(line);
    
    memcpy(&text_buffer->rows[pos + 1], &text_buffer->rows[pos], copy_size);
    
    text_buffer->rows[pos] = line;
    
    zen_tb_text_buffer_set_length(text_buffer, text_buffer->length + 1);
    
    return text_buffer;
}

static zen_inline void
zen_tb_text_buffer_set_length(zen_tb_text_buffer *text_buffer, zen_tb_size length)
{
    text_buffer->length = length;
}

#endif /* ZEN_TB_IMPLEMENTATION */

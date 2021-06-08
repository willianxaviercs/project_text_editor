// NOTE(willian): the allocator does not check if you have previosly allocated
//                   a string, causing a memory leak

/* EXAMPLE */

/*  
//  CUSTOM CAPACITY
    editor_string str = editor_string_make_define_capacity("", 1000);
    
    // OR
    
    // default capacity will be the string length
    editor_string str = editor_string_make("hello, world!");
    
    // OR
    
    // empty string with 0 capacity
    editor_string str = editor_string_make("");
    
     editor_usize str_cap = editor_string_capacity(str);
    editor_usize str_len = editor_string_length(str);
    
    str = editor_string_append(str, "Hello, world!");
    
    str_cap = editor_string_capacity(str);
    str_len = editor_string_length(str);
    
    str = editor_string_append(str, " Dynamic string builder!");
    
    str_cap = editor_string_capacity(str);
    str_len = editor_string_length(str);
    */

#ifndef ZEN_STRING_H
#define ZEN_STRING_H

#include <string.h> /* memcpy ,strlen, calloc, free */

#define ZEN_NULLPTR 0

#define STRING_HEADER(s) ((zen_string_header *)s - 1)

/*  */
#define ZEN_ZEROALLOC(num, size) calloc(num, size)
#define ZEN_FREE(p) free(p)

typedef size_t zen_usize;
typedef char * zen_string;

/* STRUCTS */
typedef struct
{
    zen_usize length;
    zen_usize capacity;
} zen_string_header;

typedef struct
{
    zen_string_header header;
    char str[1];
} zen_string_container;

/* DECLARATIONS */

/* builds a string and define a capacity that is equal or greater than the string length */
static zen_string editor_string_make_define_capacity(const char *cstr, zen_usize size);

/* build a string with the the default length */
static zen_string zen_string_make(const char *cstr);

/* return the current length of the stirng */
static inline zen_usize zen_string_length(zen_string str);

/* return the total capacity of the string */
static inline zen_usize zen_string_capacity(zen_string str);

/* set the total capacity */
static inline void zen_string_set_capacity(zen_string str, zen_usize capacity);

/* set the length */
static inline void zen_string_set_length(zen_string str, zen_usize length);

/* append a c style string to the string and update the header, allocting more size if necessary */
static zen_string  zen_string_append(zen_string str, const char *str_to_append);

#endif /* ZEN_STRING_H */

#if defined ZEN_STRING_IMPLEMENTATION

/* IMPLEMENTATION */
static zen_string
zen_string_make_define_capacity(const char *cstr, zen_usize size)
{
    zen_usize length;
    zen_usize allocation_size;
    zen_usize real_size;
    zen_string_container *container;
    
    length = strlen(cstr);
    
    real_size = length > size ? length : size;
    
    allocation_size = sizeof(*container) + real_size + 1;
    
    container = (zen_string_container *)ZEN_ZEROALLOC(allocation_size, 1);
    
    if (!container) return ZEN_NULLPTR;
    
    container->header.length = length;
    container->header.capacity = real_size;
    
    memcpy(container->str, cstr, length);
    
    return container->str;
}

static zen_string
zen_string_make(const char *cstr)
{
    return zen_string_make_define_capacity(cstr, 0);
}

static inline void 
zen_string_set_length(zen_string str, zen_usize length)
{
    STRING_HEADER(str)->length = length;
}

static inline void 
zen_string_set_capacity(zen_string str, zen_usize capacity)
{
    STRING_HEADER(str)->capacity = capacity;
}

static void zen_string_free(zen_string str)
{
    ZEN_FREE(STRING_HEADER(str));
}

static zen_string 
zen_string_append(zen_string str, const char *str_to_append)
{
    zen_usize str_length;
    zen_usize append_length;
    zen_usize space_left;
    zen_string new_str;
    
    str_length = zen_string_length(str);
    
    append_length = strlen(str_to_append);
    
    space_left = zen_string_capacity(str) - str_length;
    
    if (space_left >= append_length)
    {
        memcpy(str + str_length, str_to_append, strlen(str_to_append));
        zen_string_set_length(str, str_length + append_length);
        return str;
    }
    
    new_str = zen_string_make_define_capacity("", str_length + append_length);
    memcpy(new_str, str, str_length);
    memcpy(new_str + str_length, str_to_append, append_length);
    zen_string_set_length(new_str, str_length + append_length);
    
    zen_string_free(str);
    
    str = new_str;
    
    return str;
}

static inline zen_usize
zen_string_length(zen_string str)
{
    zen_usize result = STRING_HEADER(str)->length;
    return result;
}

static inline zen_usize
zen_string_capacity(zen_string str)
{
    zen_usize result = STRING_HEADER(str)->capacity;
    return result;
}

#endif /* ZEN_STRING_IMPLEMENTATION */
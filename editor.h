/* EDITOR_H */
#ifndef EDITOR_H
#define EDITOR_H

#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#include "stb_truetype.h"

// typedefs
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define ZEN_STRING_IMPLEMENTATION
#include "zen_string.h"

#define ZEN_TB_IMPLEMENTATION
#include "zen_tb.h"

//////////////// defines
#define EDITOR_DEBUG 1
#define EDITOR_OPENGL 1

#define OEM_KEYBOARD_KEYS_COUNT 127
#define EDITOR_ROW_POOL_SIZE 10

#define EDITOR_MAX_PATH 250

typedef zen_string editor_string;
typedef zen_tb_text_buffer editor_text_buffer;

#if EDITOR_DEBUG
#define ASSERT(expr) if (!(expr)) *(int *)0 = 1
#else
#define ASSERT(expr)
#endif

union v4
{
    struct
    {
        float x, y, z, w;
    };
    struct
    {
        float r, g, b, a;
    };
    float e[4];
};

v4 
create_v4(float x, float y, float z, float w)
{
    v4 result = {x, y, z, w};
    return result;
}

// this assumes rgba hex color
// TODO(willian): rounding for more precision??

// convertS an hex AARRGGBB U32 to an RGBA v4 
v4 convert_uhex_to_v4(u32 hex)
{
    v4 result;
    
    u8 hexa = (u8)(hex >> 24);
    u8 hexr = (u8)(hex >> 16);
    u8 hexg = (u8)(hex >> 8);
    u8 hexb = (u8)(hex);
    
    result.r = (float)hexr / 255.0f;
    result.g = (float)hexg / 255.0f;
    result.b = (float)hexb / 255.0f;
    result.a = (float)hexa / 255.0f;
    
    return result;
}

// used to classify an operation on the undo stack
enum 
{
    NO_OP,
    INSERT_CHAR,
    BACKSPACE_DELETE_CHAR,
    BACKSPACE_DELETE_WORD,
    DELETE_RANGE,
    COPY_TO_CLIPBOARD,
    PASTE_FROM_CLIPBOARD,
    CUT_TO_CLIPBOARD,
    DELETE_LINE,
    CREATE_LINE
};

// this is struct passed zeored means an NO_OP operation
struct editor_operation
{
    int type;
    u32 cursor_x;
    u32 cursor_y;
    u32 mark_x;
    u32 mark_y;
    u8 single_char;
    char *data;
    u32 compound_op_size;
};

// TODO(willian): the undo stack needs to be dynamic if
// we want 'infinite' undo , or use a ring buffer for wrapping  arouund 
#define MAX_STACK_OPS 10000
struct editor_operation_stack
{
    editor_operation op[MAX_STACK_OPS];
    bool have_op;
    int index;
};

#define MEGABYTE(x) ((x) * 1024 * 1024)

struct editor_memory_arena
{
    u8 *buffer;
    u32 offset;
    u32 total_size;
};

enum
{
    EDIT_MODE,
    SEARCH_MODE,
    OPEN_FILE_MODE,
    SWITCH_FILE_MODE,
    AUTOCOMPLETE_MODE
};

struct editor_theme
{
    // TODO(willian): menus background colors, menus font colors, menu hover color
    //    info bar background color, info bar font color
    v4 default_color;
    v4 cursor_color;
    v4 mark_color;
    v4 identifier_color;
    v4 keyword_color;
    v4 string_constant_color;
    v4 number_constant_color;
    v4 comment_color;
    v4 preprocessor_color;
    v4 font_color;
    v4 line_highlight_color;
    v4 background_color;
};

//////////////// structs
struct key_state
{
    bool wasDown;
    bool endedDown;
};

struct keyboard_input
{
    key_state keys[OEM_KEYBOARD_KEYS_COUNT];
    key_state keyUp;
    key_state keyDown;
    key_state keyLeft;
    key_state keyRight;
    key_state spacebar;
    key_state backspace;
    key_state shift;
    key_state control;
    key_state enter;
    key_state escape;
    key_state alt;
    key_state home;
    key_state end;
    key_state tab;
    bool changedState;
    bool arrowChangedState;
};

// zero initalized
struct editor_clipboard
{
    u32 size;
    bool has_changed;
    char *data;
};

struct editor_rectangle
{
    int x, y, dx, dy;
};

struct editor_screenbuffer
{
    int width;
    int height;
    int bytes_per_pixel;
    u32 pitch;
    void *memory;
};

enum
{
    SEARCH_MODE_NORMAL,
    SEARCH_MODE_REVERSE
};

struct editor_search_buffer
{
    u32 cursor;
    char *data;
    u32 total_size;
    u32 current_size;
    bool found_match;
    bool skip_match;
    u32 search_mode;
};

enum
{
    FILE_TYPE_NORMAL,
    FILE_TYPE_DIR,
    FILE_TYPE_NOT_SUPPORTED
};

struct editor_file_info
{
    u32 size;
    u32 type;
    char *filename;
    u32 filename_size;
};

// TODO(willian): check the MAX_FILES
#define EDITOR_MAX_FILES 1024
struct editor_files_listing
{
    editor_file_info files[EDITOR_MAX_FILES];
    u32 max_index;
    u32 current_index;
};

struct editor_line
{
    char *data;
    u32 buffer_size;
    u32 length;
    bool edited;
};

struct glyphs
{
    int width;
    int height;
    int x0, y0, x1, y1;
    int yOffset;
    int xOffset;
    int advance;
    unsigned char *bitmap;
};

struct editor_font
{
    char *name;
    int  size;
    u32 width;
    int ascent, descent, lineGap;
    int  x0, x1, y0, y1;
    float scale;
    struct glyphs glyph[255];
};

struct editor_text_buffer_node
{
    editor_text_buffer *text_buffer;
    editor_text_buffer_node *next;
};

struct editor_text_buffer_list
{
    editor_text_buffer_node *opened_file;
    u32 number_of_files;
    u32 current_index;
};


typedef struct token_list
{
    size_t tokens_count;
    size_t capacity;
    char **token_position;
    char *tokens_string;
} token_list;

typedef struct token_to_query
{
    char *token;
    u32 length;
    u32 *matches_indexes;
    u32 current_match_index;
    u32 matches_count;
} token_to_query;

struct editor_state
{
    u32 mode;
    bool running;
    float delta_time;
    float sine;
    token_list list_of_tokens;
    token_to_query autocompletion_token;
    editor_text_buffer *current_text_buffer;
    editor_text_buffer_list opened_files_list;
    editor_memory_arena permanent_storage;
    editor_memory_arena frame_storage;
    editor_search_buffer search_buffer;
    editor_search_buffer switch_buffer;
    editor_search_buffer open_file_buffer;
    editor_files_listing files_to_open;
    editor_operation_stack undo_stack;
    editor_clipboard copy_clipboard;
    editor_clipboard paste_clipboard;
    editor_theme theme;
    editor_screenbuffer screen_buffer;
    void * (*platform_memory_alloc)(u32);
    void   (*platform_memory_free)(void *);
};

// TODO(willian): there is more prototypes to pull from editor.cpp
//                    just for the sake of completion

// initalize editor state
static void editor_init(editor_state *ed);

static void editor_text_buffer_list_add_node(editor_text_buffer *text_buffer, editor_state *ed);

//////////////// prototypes
static void editor_switch_current_text_buffer(editor_state *ed, editor_text_buffer_list *files_list);

static void editor_save_file(editor_text_buffer *text_buffer);

// search 
static void editor_edit_search_prompt(keyboard_input *keyboard, editor_search_buffer *search_buffer, editor_state *ed);

static void editor_prompt_delete_char(editor_search_buffer *search_buffer);

static void editor_prompt_insert_char(u8 c, editor_search_buffer *search_buffer);

// undo stack operations 
static editor_operation editor_pop_op_from_stack(editor_operation_stack *stack);

// initialize font atlas from a ttf file
static void editor_font_init(editor_font *font, unsigned char *ttf_buffer);

// update cursor state based in input
static void editor_cursor_update(keyboard_input *keyboard, editor_state *ed, editor_text_buffer *text_buffer);

////// cursor movement
static void editor_cursor_move_foward_word(editor_text_buffer *text_buffer, editor_state *ed);
static void editor_cursor_move_backward_word(editor_text_buffer *text_buffer, editor_state *ed);
static void editor_cursor_move_next_blank_line(editor_text_buffer *text_buffer, editor_state *ed);
static void editor_cursor_move_previos_blank_line(editor_text_buffer *text_buffer, editor_state *ed);

static void editor_cursor_move_left(editor_state *ed, editor_text_buffer *text_buffer);
static void editor_cursor_move_right(editor_state *ed, editor_text_buffer *text_buffer);
static void editor_cursor_move_up(editor_state *ed, editor_text_buffer *text_buffer);
static void editor_cursor_move_down(editor_state *ed, editor_text_buffer *text_buffer);

// set the mark position 
static void editor_set_mark(editor_text_buffer *text_buffer, editor_state *ed);

// swap cursor and mark position
static void editor_cursor_mark_swap(editor_text_buffer *text_buffer, editor_state *ed);

// insert a char into a line
static void editor_insert_char(u8 c , editor_text_buffer *text_buffer, editor_state *ed, bool user_op = true);

// insert a new line into the text buffer
static void editor_insert_newline(editor_text_buffer *text_buffer, editor_state *ed, bool user_op = true);

// create text buffer from a file buffer
static editor_text_buffer *editor_text_buffer_create(char *file_buffer, editor_state *ed);

// make edits to the text buffer based on input
static void editor_text_buffer_edit(editor_text_buffer *text_buffer, keyboard_input *keyboard, editor_state *ed);

// delete the range between the mark and the cursor
static void editor_delete_range(editor_text_buffer *text_buffer, editor_state *ed);

// copy the range  between the mark and the cursor
static void editor_copy_into_clipboard(editor_text_buffer *text_buffer, editor_state *ed);

// copy and delete the range  between the mark and the cursor
static void editor_cut_into_clipboard(editor_text_buffer *text_buffer, editor_state *ed);

// swap the current line with the line above
static void editor_swap_line_up(editor_text_buffer *text_buffer, editor_state *ed);

// swap the current line with the line below
static void editor_swap_line_down(editor_text_buffer *text_buffer, editor_state *ed);

// delete the current line
static void editor_delete_line(editor_text_buffer *text_buffer, editor_state *ed, bool user_op = true);

// delete the caracter left to the cursor and moves the cursor
static void editor_backspace_delete(editor_text_buffer *text_buffer, editor_state *ed, bool user_op = true);

// main functin of the editor
static void editor_update_and_render(editor_screenbuffer *screen_buffer, editor_font *font, editor_text_buffer *text_buffer, 
                                     keyboard_input *keyboard, editor_rectangle rect, editor_state *ed);

#endif /* EDITOR_H */
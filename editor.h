
/* EDITOR_H */
#ifndef EDITOR_H
#define EDITOR_H

//////////////// defines
#define EDITOR_DEBUG 1
#define OEM_KEYBOARD_KEYS_COUNT 127
#define EDITOR_ROW_POOL_SIZE 10
#define EDITOR_OPENGL 1

#if EDITOR_DEBUG
#define ASSERT(expr) if (!(expr)) *(int *)0 = 1
#else
#define ASSERT(expr)
#endif

// typedefs
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t  u64;

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

// STUDY: this function expects a windows little endian ARGB hexadecimal value
v4 convert_uhex_to_v4(u32 hex)
{
    v4 result;
    
    u8 hexa = (u8)(hex >> 24);
    u8 hexb = (u8)(hex >> 16);
    u8 hexg = (u8)(hex >> 8);
    u8 hexr = (u8)(hex);
    
    result.r = (float)hexr / 255.0f;
    result.g = (float)hexg / 255.0f;
    result.b = (float)hexb / 255.0f;
    result.a = (float)hexa / 255.0f;
    
    return result;
}

enum 
{
    NO_OP,
    INSERT_CHAR,
    BACKSPACE_DELETE_CHAR,
    DELETE_RANGE,
    COPY_TO_CLIPBOARD,
    PASTE_FROM_CLIPBOARD,
    CUT_TO_CLIPBOARD,
    DELETE_LINE,
    CREATE_LINE
};

struct editor_operation
{
    int type;
    u32 cursor_x;
    u32 cursor_y;
    u32 mark_x;
    u32 mark_y;
    u8 single_char;
    char *data;
    u32 data_size;
};

#define MAX_STACK_OPS 1000
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

struct editor_theme
{
    v4 cursor_color;
    v4 mark_color;
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
    bool changedState;
    bool arrowChangedState;
};

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
    void *memory;
};

struct editor_state
{
    u32 cursor_x;
    u32 cursor_y;
    u32 mark_x;
    u32 mark_y;
    u32 text_range_x_start;
    u32 text_range_x_end;
    u32 text_range_y_start;
    u32 text_range_y_end;
    u32 smooth_range_x_start;
    u32 smooth_range_x_end;
    u32 smooth_range_y_start;
    u32 smooth_range_y_end;
    bool multi_line_smooth;
    int font_smooth_snap_y;
    bool font_snap_upward;
    bool running;
    float delta_time;
    editor_operation_stack undo_stack;
    editor_clipboard copy_clipboard;
    editor_clipboard paste_clipboard;
    editor_theme theme;
    editor_screenbuffer screen_buffer;
    void * (*platform_memory_alloc)(u32);
    void   (*platform_memory_free)(void *);
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

struct editor_line
{
    char *data;
    u32 buffer_size;
    u32 length;
};

struct editor_text_buffer
{
    editor_line *rows;
    u32 last_row;
    u32 number_of_rows;
    u32 lines_allocated;
};

//////////////// prototypes

// initialize font atlas from a ttf file
static void editor_font_init(editor_font *font, unsigned char *ttf_buffer);

// initalize editor state
static void editor_init(editor_state *ed);

// update cursor state based in input
static void editor_cursor_update(keyboard_input *keyboard, editor_state *ed, 
                                 editor_text_buffer *text_buffer);

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
static void editor_insert_newline(editor_text_buffer *text_buffer, editor_state *ed);

// create text buffer from a file buffer
static editor_text_buffer *editor_text_buffer_create(char *file_buffer, editor_state *ed);

// make edits to the text buffer based on input
static void editor_text_buffer_edit(editor_text_buffer *text_buffer, 
                                    keyboard_input *keyboard, editor_state *ed);

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
static void editor_delete_line(editor_text_buffer *text_buffer, editor_state *ed);

// delete the caracter left to the cursor and moves the cursor
static void editor_backspace_delete(editor_text_buffer *text_buffer, editor_state *ed, bool user_op = true);

// main functin of the editor
static void editor_update_and_render(editor_screenbuffer *screen_buffer, editor_font *font, editor_text_buffer *text_buffer, 
                                     keyboard_input *keyboard, editor_rectangle rect, editor_state *ed);

// TODO(willian): there is more prototypes to pull from editor.cpp 


#endif /* EDITOR_H */
#include <windows.h>
#include <stdint.h>

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t  u64;

#include "editor.cpp"

u8 ttf_buffer[1<<20];

struct win32_offscreen_buffer
{
    void *memory;
    BITMAPINFO bmi;
    int width;
    int height;
    int bytes_per_pixel;
};

struct editor_line
{
    char *data;
    u32 length;
};

struct editor_text_buffer
{
    editor_line *rows;
    u32 last_row;
    u32 number_of_rows;
};


//////////////// prototypes
static void editor_draw_glyph(u32 c, int x, int y, u32 color, editor_rectangle rect, 
                              editor_screenbuffer *screen_buffer, editor_font *font);

static void editor_font_init(editor_font *font, unsigned char *ttf_buffer);

static void editor_init(editor_state *ed);

static void editor_exit();

////// cursor movement
static void editor_cursor_move_foward_word(editor_text_buffer *text_buffer, editor_state *ed);
static void editor_cursor_move_backward_word(editor_text_buffer *text_buffer, editor_state *ed);
static void editor_cursor_move_next_blank_line(editor_text_buffer *text_buffer, editor_state *ed);
static void editor_cursor_move_previos_blank_line(editor_text_buffer *text_buffer, editor_state *ed);

static void editor_cursor_move_left(editor_state *ed, editor_text_buffer *text_buffer);
static void editor_cursor_move_right(editor_state *ed, editor_text_buffer *text_buffer);
static void editor_cursor_move_up(editor_state *ed, editor_text_buffer *text_buffer);
static void editor_cursor_move_down(editor_state *ed, editor_text_buffer *text_buffer);

/////// cursor update
static void editor_cursor_update(keyboard_input *keyboard, editor_state *ed, 
                                 editor_text_buffer *text_buffer);

//////// cursor draw
static void editor_cursor_draw(editor_screenbuffer *screen_buffer, editor_font *font, 
                               u32 cursor_x, u32 cursor_y, u32 cursor_color, 
                               editor_rectangle rect);

//////// mark
static void editor_set_mark(editor_text_buffer *text_buffer, editor_state *ed);

static void editor_cursor_mark_swap(editor_text_buffer *text_buffer, editor_state *ed);

static void editor_insert_char(u8 c , editor_text_buffer *text_buffer, editor_state *ed);

static void editor_insert_newline(editor_text_buffer *text_buffer, editor_state *ed);


static editor_text_buffer *editor_text_buffer_create(char *file_buffer, editor_state *ed);

static void editor_text_buffer_edit(editor_text_buffer *text_buffer, 
                                    keyboard_input *keyboard, editor_state *ed);


static void editor_text_buffer_draw(editor_screenbuffer *screen_buffer, u32 font_color, 
                                    editor_font *font, editor_text_buffer *text_buffer, 
                                    editor_rectangle rect,
                                    editor_state *ed);

static void editor_update_and_render(editor_screenbuffer *screen_buffer, u32 font_color, 
                                     editor_font *font, editor_text_buffer *text_buffer, 
                                     keyboard_input *keyboard, editor_rectangle rect, editor_state *ed);

static void editor_delete_range(editor_text_buffer *text_buffer, editor_state *ed);

static void editor_copy_into_clipboard(editor_text_buffer *text_buffer, editor_state *ed);

static void editor_cut_into_clipboard(editor_text_buffer *text_buffer, editor_state *ed);

static void editor_swap_line_up(editor_text_buffer *text_buffer, editor_state *ed);

static void editor_swap_line_down(editor_text_buffer *text_buffer, editor_state *ed);

static void editor_delete_line(editor_text_buffer *text_buffer, editor_state *ed);

static void editor_backspace_delete(editor_text_buffer *text_buffer, editor_state *ed);

static void editor_draw_line_highlight(editor_screenbuffer *screen_buffer, editor_font *font, 
                                       editor_state *ed, editor_rectangle rect);


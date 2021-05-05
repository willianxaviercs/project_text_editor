#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#include "stb_truetype.h"
#include "editor.h"

#include "editor_render.cpp"

/* EXPERIMENTATION  */
// [ ] - 60fps smooth scrolling

// [ ] - sub pixel rendering for better smooth scrolling, cursor animation, 
//         smooth text scrolling

// [ ] - directdraw clear font rendering

// [ ] - opengl rendering

// [ ] - mouse support for scrolling an setting cursor


/* EASY BUG FIXES TODO */
// [ ] - make sure that the mark never stay out of the last_row range when deleting a range
//      maybe a check then update if necessary into the deleting function

// [ ] - windows allocation an free functions, need more research on that

// [ ] - allowing other types of line endings

// [ ] - mark drawing is not accounting for the screen range

// [ ] - not easy reproducible bug when pasting from the clipboard

// [ ]  - still having bug when inserting new line, gotta verify if allocation
//          is sometimes failing

// [ ] - alt + movement keys is triggering sometimes, check alt.endeddown boolean,
//             check the key press handle

// [ ] - include a font based on the glyph width so we can use for the cursor and mark
//             operations and drawing

// TODO LIST:
// 1 - Implement Mark editing style
//     - [x] set mark
//     - [x] mark/cursor swap
//     - [x] copy
//     - [x] delete
//     - [X] cut
//     - [x] paste

// 2 - searching
//     - [ ] reverse searching
//     - [ ] iterative searching

// 3 - [x] line deleting (backspace)

// 4 - [ ] log system

// 5 - [ ] cursor clipping when drawing

// 6 - [ ] change cursor and mark to be for the current file only

// 7 - FILE MANAGEMENT [INTERATIVE]
//     - [ ] open file 
//     - [ ] new file
//     - [ ] close file
//     - [ ] switch file 
//     - [ ] save file

//     // TODO(willian): make that when we change the rect size
//     // we can account for that when changing the text range
// 8 - text scrolling
//     - [x] vertical scrolling
//     - [x] horizontal scrolling

// 9 - text drawing 
//     - [x] redraw only the screen range of text

// 9 - GUI
//     - [x] line highlighting
//     - [ ] cursor blinking
//     - [ ] text file info footer
//           - [x] filename
//           - [x] cursor position
//           - [ ] line ending type
//           - [ ] dirty file indicator

// 10 - basic text editing and cursor movement
//      - [x] delete a word                      
//      - [x] move cursor between spaces vertically
//      - [x] move cursor to the next blank line
//      - [x] home and end cursor movement
//      - [x] begin of file and end of file cursor movement
//      - [x] line swapping
//      - [ ] undo
//      - [ ] redo
//      - [ ] tab autocomplete

// 11 - [ ] syntax highlighting

// 12 - [ ] editor auto execute build file and display output

// 13 - [ ] editor parse the compiler output line error and jump on command

// 14 - [ ] goto line

// 15 - [x] 60 fps

// 16 - [ ] when the program is inactive or minimized, we just sleep and dont do
//             anything

// 17 - [ ] Editor parser and lexer
//          - [ ] Line indentation and virtual whitespacing
//          - [ ] Save Function definitions lines
//          - [ ] Save definitions for autocompletion


// TODO(willian): unnaligned allocation, need padding for performance
static void *
editor_arena_alloc(u32 size, editor_memory_arena *arena)
{
    // init the arena
    if (!arena->buffer)
    {
        arena->buffer = (u8 *)malloc(MEGABYTE(1) * sizeof(arena->buffer[0]));
        arena->total_size = MEGABYTE(1);
    }
    
    if ((arena->offset + size) > arena->total_size)
    { 
        // realloc the arena for a bigger size
        ASSERT((arena->offset + size) <= arena->total_size);
    }
    
    void *ptr = &arena->buffer[arena->offset];
    arena->offset += size;
    
    // clear to zero
    memset(ptr, 0, size);
    return ptr;
}

static void
editor_execute_op(editor_operation op, editor_state *ed,
                  editor_text_buffer *text_buffer)
{
    switch (op.type)
    {
        case NO_OP:
        {
            break;
        }
        case INSERT_CHAR:
        {
            ed->cursor_x = op.cursor_x;
            ed->cursor_y = op.cursor_y;
            editor_backspace_delete(text_buffer, ed, false);
            break;
        }
        case BACKSPACE_DELETE_CHAR:
        {
            ed->cursor_x = op.cursor_x;
            ed->cursor_y = op.cursor_y;
            editor_insert_char(op.single_char, text_buffer, ed, false);
            break;
        }
        default:
        {
            // we should never come here
            // log:
        }
    }
}

static editor_operation
editor_pop_op_from_stack(editor_operation_stack *stack)
{
    editor_operation op = {};
    
    if (stack->index > 0)
    {
        stack->have_op = false;
        op = stack->op[--stack->index];
    }
    
#if 0
    
    op.type = stack->op[stack->index].type;
    op.cursor_x = stack->op[stack->index].cursor_x;
    op.cursor_y = stack->op[stack->index].cursor_y;
    op.mark_x = stack->op[stack->index].mark_x;
    op.mark_y = stack->op[stack->index].mark_y;
    op.data = stack->op[stack->index].data;
    op.data_size = stack->op[stack->index].data_size;
#endif
    
    
    return op;
}

static void
editor_push_op_into_stack(editor_operation_stack *stack, 
                          editor_operation op)
{
    // TODO(willian): we can either make this stack be a ring
    // that wrap around, a buffer that grows a fixed size, or a 
    // linked list
    
    if (stack->index < MAX_STACK_OPS)
    {
        stack->op[stack->index++] = op;
        stack->have_op = true;
    }
}

static editor_text_buffer *
editor_text_buffer_create(char *file_buffer, editor_state *ed)
{
    editor_text_buffer *text_buffer;
    text_buffer = (editor_text_buffer *)ed->platform_memory_alloc(sizeof(*text_buffer));
    
    // TODO(willian): log
    ASSERT(text_buffer);
    
    char *c = file_buffer;
    u32 row_count = 0;
    
    // count how much rows we need
    while (*c)
    {
        if (*c == '\n') row_count++;
        c++;
    }
    row_count++;
    
    // TODO(willian): idk if this is the correct way to fix the off by one
    // when counting lines in a file
    text_buffer->number_of_rows = row_count;
    u32 lines_to_alloc = row_count + EDITOR_ROW_POOL_SIZE;
    text_buffer->rows = (editor_line *)ed->platform_memory_alloc(sizeof(editor_line ) * lines_to_alloc);
    
    // TODO(willian): LOG
    ASSERT(text_buffer->rows);
    
    text_buffer->last_row = 0;
    
    if (text_buffer->rows == NULL) return 0; 
    
    text_buffer->lines_allocated = lines_to_alloc;
    
    u32 line_start = 0;
    u32 line_length = 0;
    
    for (c = file_buffer; ; c++)
    {
        if (*c == 0)
        {
            u32 buffer_size = line_length + 100;
            
            text_buffer->rows[text_buffer->last_row].data = 
                (char *)ed->platform_memory_alloc(sizeof(char) * buffer_size);
            
            // TODO(willian): LOG
            ASSERT(text_buffer->rows[text_buffer->last_row].data);
            
            memcpy(text_buffer->rows[text_buffer->last_row].data, file_buffer + line_start, line_length);
            text_buffer->rows[text_buffer->last_row].length = line_length;
            text_buffer->rows[text_buffer->last_row].data[line_length] = 0;
            text_buffer->rows[text_buffer->last_row].buffer_size = buffer_size;
            line_start += line_length;
            line_start++;
            //text_buffer->last_row++;
            break;
        }
        // TODO(willian): unix and other EOL
        // found line ending
        if (*c == '\n') 
        {
            u32 buffer_size = line_length + 100;
            
            text_buffer->rows[text_buffer->last_row].data = 
                (char *)ed->platform_memory_alloc(sizeof(char) * buffer_size);
            
            // TODO(willian): LOG
            ASSERT(text_buffer->rows[text_buffer->last_row].data);
            
            memcpy(text_buffer->rows[text_buffer->last_row].data, file_buffer + line_start, line_length);
            text_buffer->rows[text_buffer->last_row].length = line_length;
            text_buffer->rows[text_buffer->last_row].data[line_length] = 0;
            text_buffer->rows[text_buffer->last_row].buffer_size = buffer_size;
            line_start += line_length;
            line_start++;
            text_buffer->last_row++;
            line_length = 0;
        }
        else
        {
            line_length++;
        }
    }
    
    return text_buffer;
}

static void 
editor_font_init(editor_font *font, unsigned char *ttf_buffer)
{
    stbtt_fontinfo info;
    
    int c;
    int width;
    int height;
    
    stbtt_InitFont(&info, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer,0));
    
    font->scale = stbtt_ScaleForPixelHeight(&info, (float)font->size);
    stbtt_GetFontVMetrics(&info, &font->ascent, &font->descent, &font->lineGap);
    
    font->ascent =  (int)(font->ascent * font->scale);
    
    // NOTE(willian): by now we are locked into the ascii printables but is easy to expand
    // once I learn how to make an utf8 editor
    for (c = 32; c < 255; ++c)
    {
        font->glyph[c - 32].bitmap = 
            stbtt_GetCodepointBitmap(&info, 0, font->scale, c, &width, &height, 0, 0);
        
        font->glyph[c - 32].width = width;
        font->glyph[c - 32].height = height;
        
        int c_x1, c_y1, c_x2, c_y2;
        stbtt_GetCodepointBitmapBox(&info, c, font->scale, 
                                    font->scale, &c_x1, &c_y1, &c_x2, &c_y2);
        
        font->glyph[c - 32].yOffset = font->ascent + c_y1;
        
        int ax, lsb;
        stbtt_GetCodepointHMetrics(&info, c, &ax, &lsb);
        font->glyph[c - 32].advance = (int)(ax * font->scale);
        font->glyph[c - 32].xOffset = (int)(lsb * font->scale);
    }
}

// all initialization code goes here except the font init
static void 
editor_init(editor_state *ed)
{
    ed->cursor_x = 0;
    ed->cursor_y = 0;
    ed->mark_x = 0;
    ed->mark_y = 0;
    
    ed->undo_stack = {};
    
    ed->copy_clipboard = {};
    ed->paste_clipboard = {};
    
    ed->theme.cursor_color = convert_uhex_to_v4(0xAA81a1c1);
    ed->theme.mark_color = convert_uhex_to_v4(0x8fbcbbff);
    ed->theme.font_color = convert_uhex_to_v4(0xFFd8dee9);
    ed->theme.line_highlight_color = convert_uhex_to_v4(0xFF3b4252);
    ed->theme.background_color = convert_uhex_to_v4(0xFF2e3440);
    
    ed->running = true;
}

static void
editor_paste_from_clipboard(editor_text_buffer *text_buffer,  editor_state *ed)
{
    // log
    if (!ed->paste_clipboard.data) return;
    
    for (u32 c = 0; c < ed->paste_clipboard.size - 1; c++)
    {
        // TODO(willian): we gotta deal with it later
        if (ed->paste_clipboard.data[c] == '\r')
        {
            continue;
        }
        
        else if (ed->paste_clipboard.data[c] == '\n')
        {
            editor_insert_newline(text_buffer, ed);
        }
        else editor_insert_char(ed->paste_clipboard.data[c] , text_buffer, ed);
    }
}

/* Copy the caracters in our own editor clipboard, that later can be passed
*  to the platform code 
*/
static void
editor_copy_into_clipboard(editor_text_buffer *text_buffer,  editor_state *ed)
{
    bool mark_equal_cursor = (ed->mark_x == ed->cursor_x) &&
        (ed->mark_y == ed->cursor_y);
    
    if (mark_equal_cursor) return;
    
    // clean last clipboard
    if (ed->copy_clipboard.data) ed->platform_memory_free(ed->copy_clipboard.data);
    ed->copy_clipboard.data = 0;
    
    // save positions
    u32 old_cursor_x = ed->cursor_x;
    u32 old_cursor_y = ed->cursor_y;
    u32 old_mark_x = ed->mark_x;
    u32 old_mark_y = ed->mark_y;
    
    bool mark_ahead_of_cursor = (ed->mark_y > ed->cursor_y) ||
        ((ed->mark_y == ed->cursor_y) && (ed->mark_x > ed->cursor_x));
    
    bool cursor_ahead_of_mark = !mark_ahead_of_cursor;
    
    // swap if mark position if ahead of cursor
    if (mark_ahead_of_cursor) editor_cursor_mark_swap(text_buffer, ed);
    
    // NOTE(willian): this is so ugly
    u32 char_count = 0;
    while ((ed->cursor_x > ed->mark_x) ||
           (ed->cursor_y > ed->mark_y))
    {
        editor_cursor_move_left(ed, text_buffer);
        char_count++;
    }
    
    ed->copy_clipboard.size = char_count + 1;
    ed->copy_clipboard.data = (char *)calloc(ed->copy_clipboard.size, sizeof(char));
    ASSERT(ed->copy_clipboard.data);
    
    // restore positions
    ed->cursor_x = old_cursor_x;
    ed->cursor_y = old_cursor_y;
    ed->mark_x = old_mark_x;
    ed->mark_y = old_mark_y;
    
    // now we only swap if cursor is ahead of mark
    if (cursor_ahead_of_mark) editor_cursor_mark_swap(text_buffer, ed);
    
    // TODO(willian): port: when doing this on unix we might want
    //                to use their line ending
    
    // TODO(willian): when we implement or virtual whitespace for code identation
    //               we gonna need to account for each line identation when calculating size
    //               and adding the whitesapce accordinly
    
    u32 i = 0;
    while ((ed->cursor_x < ed->mark_x) ||
           (ed->cursor_y < ed->mark_y))
    {
        if (ed->cursor_x == text_buffer->rows[ed->cursor_y].length)
        {
            ed->copy_clipboard.data[i++] = '\n';
        }
        else
        {
            ed->copy_clipboard.data[i++] = text_buffer->rows[ed->cursor_y].data[ed->cursor_x];
        }
        editor_cursor_move_right(ed, text_buffer);
    }
    
    // debug
    ASSERT(i <= char_count);
    
    ed->copy_clipboard.has_changed = true;
    
    // restore
    ed->cursor_x = old_cursor_x;
    ed->cursor_y = old_cursor_y;
    ed->mark_x = old_mark_x;
    ed->mark_y = old_mark_y;
}

static void editor_cut_into_clipboard(editor_text_buffer *text_buffer,  editor_state *ed)
{
    editor_copy_into_clipboard(text_buffer, ed);
    editor_delete_range(text_buffer, ed);
}

static void
editor_cursor_mark_swap(editor_text_buffer *text_buffer, editor_state *ed)
{
    u32 temp_x = ed->cursor_x;
    u32 temp_y = ed->cursor_y;
    
    ed->cursor_x = ed->mark_x;
    ed->cursor_y = ed->mark_y;
    
    ed->mark_x = temp_x;
    ed->mark_y = temp_y;
}

static void
editor_cursor_move_left(editor_state *ed, editor_text_buffer *text_buffer)
{
    if ((ed->cursor_x == 0) && (ed->cursor_y > 0))
    {
        ed->cursor_y--;
        ed->cursor_x = text_buffer->rows[ed->cursor_y].length;
    }
    else if ((ed->cursor_x > 0) || (ed->cursor_y > 0))
    {
        ed->cursor_x--;
    }
}

static void
editor_cursor_move_right(editor_state *ed, editor_text_buffer *text_buffer)
{
    if (ed->cursor_x < text_buffer->rows[ed->cursor_y].length) 
    {
        ed->cursor_x++;
    }
    else if (ed->cursor_x == text_buffer->rows[ed->cursor_y].length &&
             ed->cursor_y < text_buffer->last_row)
    {
        ed->cursor_x = 0;
        ed->cursor_y++;
    }
}

static void
editor_cursor_move_up(editor_state *ed, editor_text_buffer *text_buffer)
{
    if (ed->cursor_y > 0) ed->cursor_y--;
    
    if (ed->cursor_x > text_buffer->rows[ed->cursor_y].length)
    {
        ed->cursor_x = text_buffer->rows[ed->cursor_y].length;
    }
}

static void
editor_cursor_move_down(editor_state *ed, editor_text_buffer *text_buffer)
{
    // 
    if (ed->cursor_y < text_buffer->last_row) ed->cursor_y++;
    
    if (ed->cursor_x > text_buffer->rows[ed->cursor_y].length)
    {
        ed->cursor_x = text_buffer->rows[ed->cursor_y].length;
    }
}

static void 
editor_cursor_update(keyboard_input *keyboard, editor_state *ed, editor_text_buffer *text_buffer)
{
    // ctrl + right
    if (keyboard->control.endedDown &&
        keyboard->keyRight.endedDown)
    {
        editor_cursor_move_foward_word(text_buffer, ed);
    }
    // // ctrl + right
    else if (keyboard->control.endedDown &&
             keyboard->keyLeft.endedDown)
    {
        editor_cursor_move_backward_word(text_buffer, ed);
        
    }
    
    // // ctrl + down
    else if (keyboard->control.endedDown &&
             keyboard->keyDown.endedDown)
    {
        editor_cursor_move_next_blank_line(text_buffer, ed);
        ed->cursor_x = 0;
    }
    
    // ctrl + up
    else if (keyboard->control.endedDown &&
             keyboard->keyUp.endedDown)
    {
        editor_cursor_move_previos_blank_line(text_buffer, ed);
        ed->cursor_x = 0;
    }
    
    // ctrl + home 
    else if (keyboard->control.endedDown && keyboard->home.endedDown)
    {
        ed->cursor_x = 0;
        ed->cursor_y = 0;
    }
    
    // ctrl + end
    else if (keyboard->control.endedDown && keyboard->end.endedDown)
    {
        ed->cursor_y = text_buffer->last_row;
        ed->cursor_x = text_buffer->rows[ed->cursor_y].length;
    }
    
    // alt + up
    else if (keyboard->alt.endedDown && keyboard->keyUp.endedDown)
    {
        editor_swap_line_up(text_buffer, ed);
    }
    
    // alt + down
    else if (keyboard->alt.endedDown && keyboard->keyDown.endedDown)
    {
        editor_swap_line_down(text_buffer, ed);
    }
    
    // home 
    else if (keyboard->home.endedDown)
    {
        ed->cursor_x = 0;
    }
    
    // end
    else if (keyboard->end.endedDown)
    {
        ed->cursor_x = text_buffer->rows[ed->cursor_y].length;
    }
    
    // arrow right
    else if (keyboard->keyRight.endedDown) 
    {
        editor_cursor_move_right(ed, text_buffer);
    }
    // arrow left
    else if (keyboard->keyLeft.endedDown) 
    {
        editor_cursor_move_left(ed, text_buffer);
    }
    // arrow up
    else if (keyboard->keyUp.endedDown) 
    {
        editor_cursor_move_up(ed, text_buffer);
    }
    // arrow down
    else if (keyboard->keyDown.endedDown) 
    {
        editor_cursor_move_down(ed, text_buffer);
    }
    
    ///// screen range update
    
    // vertical range
    
    // downward
    if (ed->cursor_y > ed->text_range_y_end)
    {
        u32 diff  = (ed->cursor_y - ed->text_range_y_end);
        
        ed->text_range_y_start += diff;
        ed->text_range_y_end += diff;
        
        // TODO(willian): calculate this limit
        // limit the smooth diff to be 2 screens worth of symbols
        ed->smooth_range_y_start = ed->text_range_y_start - (diff);
        ed->smooth_range_y_end = ed->text_range_y_end - (diff);
        
        ed->font_smooth_snap_y = 14; // font_size
        ed->font_snap_upward = false;
        if (diff > 1) ed->multi_line_smooth = true;
    }
    
    // upward
    else if (ed->cursor_y < ed->text_range_y_start)
    {
        int diff = (ed->text_range_y_start - ed->cursor_y);
        
        ed->text_range_y_end -= diff;
        ed->text_range_y_start -= diff;
        
        // TODO(willian): calculate this limit
        // limit the smooth diff to be 2 screens worth of symbols
        // rect_height / font_size
        
        ed->smooth_range_y_start = ed->text_range_y_start + (diff);
        ed->smooth_range_y_end = ed->text_range_y_end + (diff);
        
        ed->font_smooth_snap_y = 14; // font_size
        ed->font_snap_upward = true;
        if (diff > 1) ed->multi_line_smooth = true;
    }
    
    // horizontal range
    if (ed->cursor_x > ed->text_range_x_end)
    {
        u32 diff  = (ed->cursor_x - ed->text_range_x_end);
        ed->text_range_x_end += diff;
        ed->text_range_x_start += diff;
    }
    
    else if (ed->cursor_x < ed->text_range_x_start)
    {
        u32 diff = (ed->text_range_x_start - ed->cursor_x);
        ed->text_range_x_end -= diff;
        ed->text_range_x_start -= diff;
    }
}

static void 
editor_insert_char(u8 c, editor_text_buffer *text_buffer, editor_state *ed, bool user_op)
{
    // debug
    ASSERT(ed->cursor_x <= text_buffer->rows[ed->cursor_y].length);
    
    if (text_buffer->rows[ed->cursor_y].length >= 
        text_buffer->rows[ed->cursor_y].buffer_size)
    {
        u32 old_size = text_buffer->rows[ed->cursor_y].buffer_size;
        u32 new_size = old_size + 120;
        
        char *old_buffer = text_buffer->rows[ed->cursor_y].data;
        
        // TODO(willian): assert for memory alloc failure
        text_buffer->rows[ed->cursor_y].data = (char *)ed->platform_memory_alloc(sizeof(char) * new_size);
        
        ASSERT(text_buffer->rows[ed->cursor_y].data);
        
        CopyMemory(text_buffer->rows[ed->cursor_y].data, old_buffer, old_size );
        
        text_buffer->rows[ed->cursor_y].buffer_size = new_size;
        
        ed->platform_memory_free(old_buffer);
    }
    
    char *src  = text_buffer->rows[ed->cursor_y].data + ed->cursor_x;
    char *dest = text_buffer->rows[ed->cursor_y].data + ed->cursor_x + 1;
    u32 length = text_buffer->rows[ed->cursor_y].length - ed->cursor_x;
    
    ASSERT(length >= 0);
    
    CopyMemory(dest, src, length);
    
    text_buffer->rows[ed->cursor_y].data[ed->cursor_x] = c;
    text_buffer->rows[ed->cursor_y].length++;
    ed->cursor_x++;
    
    if (user_op)
    {
        // push the operation into the operation stack
        editor_operation op = {};
        op.type = INSERT_CHAR;
        op.cursor_x = ed->cursor_x;
        op.cursor_y = ed->cursor_y;
        editor_push_op_into_stack(&ed->undo_stack, op);
    }
}

// TODO(willian): rework this entirely
static void 
editor_insert_newline(editor_text_buffer *text_buffer, editor_state *ed)
{
    ASSERT(ed->cursor_y <= text_buffer->last_row);
    
    // allocate more lines if we need to
    editor_line *old_buffer;
    
    if (text_buffer->last_row == text_buffer->lines_allocated)
    {
        u32 lines_to_alloc = text_buffer->lines_allocated + EDITOR_ROW_POOL_SIZE;
        u32 size_to_alloc  = sizeof(text_buffer->rows[0]) * lines_to_alloc;
        
        // save pointer for copy
        old_buffer = text_buffer->rows;
        
        // alloc a resized  buffer and copy the old contents
        text_buffer->rows = (editor_line *)ed->platform_memory_alloc(size_to_alloc);
        
        ASSERT(text_buffer->rows);
        
        CopyMemory(text_buffer->rows, old_buffer, 
                   (text_buffer->last_row + 1) * sizeof(text_buffer->rows[0]));
        
        text_buffer->lines_allocated = lines_to_alloc;
        ed->platform_memory_free(old_buffer);
    }
    
    // create space for the new line
    editor_line *src  = &text_buffer->rows[ed->cursor_y];
    editor_line *dest = &text_buffer->rows[ed->cursor_y + 1];
    u32 length = (text_buffer->last_row - ed->cursor_y) * sizeof(editor_line);
    
    ASSERT(length >= 0);
    
    // move the pointer 1 ahead
    CopyMemory(dest, src, length);
    
    // allocate new line
    ASSERT(text_buffer->rows[ed->cursor_y].length >=  ed->cursor_x);
    
    u32 copy_len = text_buffer->rows[ed->cursor_y].length - ed->cursor_x;
    char *copy_src = &(text_buffer->rows[ed->cursor_y].data[ed->cursor_x]);
    
    u32 newline_size = sizeof(char) * (copy_len + 100);
    text_buffer->rows[ed->cursor_y + 1].data = (char *)ed->platform_memory_alloc(newline_size);
    text_buffer->rows[ed->cursor_y + 1].buffer_size =  newline_size;
    char *copy_dest = text_buffer->rows[ed->cursor_y + 1].data;
    
    // copy the the contents from cursor to length to the line below
    CopyMemory(copy_dest, copy_src, copy_len);
    
    text_buffer->rows[ed->cursor_y].length = ed->cursor_x;
    
    text_buffer->rows[ed->cursor_y + 1].length = copy_len;
    
    ed->cursor_y++;
    ed->cursor_x = 0;
    text_buffer->last_row++;
    text_buffer->number_of_rows++;
}

static 
void editor_delete_line(editor_text_buffer *text_buffer, editor_state *ed)
{
    // copy the current line content into the line above
    u32 src_len = text_buffer->rows[ed->cursor_y].length;
    u32 dest_len = text_buffer->rows[ed->cursor_y - 1].length;
    
    char *src = text_buffer->rows[ed->cursor_y].data;
    char *dest = text_buffer->rows[ed->cursor_y - 1].data;
    
    // check ifthe line above have sufficient memory to fit
    u32 size_to_fit = text_buffer->rows[ed->cursor_y].length +
        text_buffer->rows[ed->cursor_y - 1].length;
    
    // reallocate if the correct size
    if (size_to_fit > text_buffer->rows[ed->cursor_y - 1].buffer_size)
    {
        u32 new_size = size_to_fit + 100;
        char *new_buffer = 
            (char *)ed->platform_memory_alloc(sizeof(char *) * new_size);
        
        // recopy the content
        CopyMemory(new_buffer, dest, dest_len);
        
        // free old memory and reassign new buffer
        ed->platform_memory_free(dest);
        
        text_buffer->rows[ed->cursor_y - 1].data = new_buffer;
    }
    
    // copy the line now
    CopyMemory(text_buffer->rows[ed->cursor_y - 1].data + dest_len, 
               src, src_len);
    
    text_buffer->rows[ed->cursor_y - 1].length = size_to_fit;
    
    // row many rows we move up
    u32 rows_to_copy = text_buffer->last_row - ed->cursor_y;
    
    CopyMemory(&text_buffer->rows[ed->cursor_y],
               &text_buffer->rows[ed->cursor_y + 1], sizeof(editor_line) * rows_to_copy);
    
    text_buffer->last_row--;
    ed->cursor_y--;
    ed->cursor_x = dest_len;
}

static void
editor_backspace_delete(editor_text_buffer *text_buffer, editor_state *ed, 
                        bool user_op)
{
    // save the char that is going to be deleted for undo op
    u8 deleted_char = text_buffer->rows[ed->cursor_y].data[ed->cursor_x - 1];;
    
    u32 copy_len  = text_buffer->rows[ed->cursor_y].length - ed->cursor_x;
    
    ASSERT(copy_len >= 0);
    
    char *src = &(text_buffer->rows[ed->cursor_y].data[ed->cursor_x]);
    char *dest = &(text_buffer->rows[ed->cursor_y].data[ed->cursor_x - 1]);
    CopyMemory(dest, src, copy_len);
    
    text_buffer->rows[ed->cursor_y].length--;
    ed->cursor_x--;
    
    // we don't save undo command only directly user commands
    if (user_op)
    {
        editor_operation op = {};
        op.type = BACKSPACE_DELETE_CHAR;
        op.cursor_x = ed->cursor_x;
        op.cursor_y = ed->cursor_y;
        op.single_char = deleted_char;
        
        editor_push_op_into_stack(&ed->undo_stack, op);
    }
}

// TODO(willian): pull to parser file
static bool editor_parser_isdelimiter(char c)
{
    // TODO(willian): make we pull this delimiter to the header file
    
    // NOTE(willian): check if we are not missing any delimiters
    
    char *delim = " _|<>(){}+-=*&%#!\'\"[],.:;/";
    while (*delim)
    {
        if (c == (*delim)) return true;
        delim++;
    }
    return false;
}

// TODO(willian): pull to parser file
static bool editor_parser_isalphabetic(char c)
{
    if (((c >= 'a') && (c <= 'z')) ||
        ((c >= 'A') && (c <= 'Z')))
    {
        return true;
    }
    return false;
}

static void
editor_swap_line_up(editor_text_buffer *text_buffer, editor_state *ed)
{
    char *data_temp;
    u32 length_temp;
    u32 buffer_size_temp;
    
    if (ed->cursor_y > 0)
    {
        data_temp  = text_buffer->rows[ed->cursor_y].data;
        length_temp = text_buffer->rows[ed->cursor_y].length;
        buffer_size_temp = text_buffer->rows[ed->cursor_y].buffer_size;
        
        // swap pointers
        text_buffer->rows[ed->cursor_y].data = text_buffer->rows[ed->cursor_y - 1].data;
        text_buffer->rows[ed->cursor_y - 1].data = data_temp;
        
        // swap length
        text_buffer->rows[ed->cursor_y].length = text_buffer->rows[ed->cursor_y - 1].length;
        text_buffer->rows[ed->cursor_y - 1].length = length_temp;
        
        // swap alloc size
        text_buffer->rows[ed->cursor_y].buffer_size = text_buffer->rows[ed->cursor_y - 1].buffer_size;
        text_buffer->rows[ed->cursor_y - 1].buffer_size = buffer_size_temp;
        
        // dec 
        ed->cursor_y--;
    }
}

static void
editor_swap_line_down(editor_text_buffer *text_buffer, editor_state *ed)
{
    char *data_temp;
    u32 length_temp;
    u32 buffer_size_temp;
    
    if (ed->cursor_y < text_buffer->last_row)
    {
        data_temp  = text_buffer->rows[ed->cursor_y].data;
        length_temp = text_buffer->rows[ed->cursor_y].length;
        buffer_size_temp = text_buffer->rows[ed->cursor_y].buffer_size;
        
        // swap pointers
        text_buffer->rows[ed->cursor_y].data = text_buffer->rows[ed->cursor_y + 1].data;
        text_buffer->rows[ed->cursor_y + 1].data = data_temp;
        
        // swap length
        text_buffer->rows[ed->cursor_y].length = text_buffer->rows[ed->cursor_y + 1].length;
        text_buffer->rows[ed->cursor_y + 1].length = length_temp;
        
        // swap alloc size
        text_buffer->rows[ed->cursor_y].buffer_size = text_buffer->rows[ed->cursor_y + 1].buffer_size;
        text_buffer->rows[ed->cursor_y + 1].buffer_size = buffer_size_temp;
        
        // dec 
        ed->cursor_y++;
    }
}

static void
editor_delete_word_backward(editor_text_buffer *text_buffer, editor_state *ed)
{
    // TODO(willian): case then we are at 0 index and we go the line above
    // maybe it's good but im not certain ...
    
    if (ed->cursor_x > 0)
    {
        bool deleted_alphabetic_char = false;
        while (ed->cursor_x > 0) 
        {
            char c = text_buffer->rows[ed->cursor_y].data[ed->cursor_x - 1];
            
            if (editor_parser_isdelimiter(c) &&
                deleted_alphabetic_char)
            {
                break;
            }
            else if (editor_parser_isalphabetic(c))
            {
                editor_backspace_delete(text_buffer, ed);
                deleted_alphabetic_char = true;
            }
            else editor_backspace_delete(text_buffer, ed);
        }
    }
}

static void
editor_cursor_move_next_blank_line(editor_text_buffer *text_buffer, editor_state *ed)
{
    // TODO(willian): change that we scan if the line have only whitespaces
    // we consider it to be a blank line too
    if (ed->cursor_y < text_buffer->last_row)
    {
        bool passed_non_blank_line = false;
        while (ed->cursor_y < text_buffer->last_row) 
        {
            u32 line_length = text_buffer->rows[ed->cursor_y].length;
            
            if (line_length == 0 && passed_non_blank_line)
            {
                break;
            }
            else if (line_length > 0)
            {
                passed_non_blank_line = true;
                ed->cursor_y++;
            }
            else ed->cursor_y++;
        }
    }
}

static void 
editor_cursor_move_previos_blank_line(editor_text_buffer *text_buffer, editor_state *ed)
{
    // TODO(willian): test it without this if check here
    if (ed->cursor_y > 0)
    {
        bool passed_non_blank_line = false;
        while (ed->cursor_y > 0)
        {
            u32 line_length = text_buffer->rows[ed->cursor_y].length;
            
            if (line_length == 0 && passed_non_blank_line)
            {
                break;
            }
            else if (line_length > 0)
            {
                passed_non_blank_line = true;
                ed->cursor_y--;
            }
            else ed->cursor_y--;
        }
    }
}

static void
editor_cursor_move_foward_word(editor_text_buffer *text_buffer, editor_state *ed)
{
    if (ed->cursor_x < text_buffer->rows[ed->cursor_y].length)
    {
        bool passed_alphabetic_char = false;
        while (ed->cursor_x < text_buffer->rows[ed->cursor_y].length) 
        {
            char c = text_buffer->rows[ed->cursor_y].data[ed->cursor_x];
            
            if (editor_parser_isdelimiter(c) &&
                passed_alphabetic_char)
            {
                break;
            }
            else if (editor_parser_isalphabetic(c))
            {
                passed_alphabetic_char = true;
                ed->cursor_x++;
            }
            else ed->cursor_x++;
        }
    }
}

static void 
editor_cursor_move_backward_word(editor_text_buffer *text_buffer, editor_state *ed)
{
    if (ed->cursor_x > 0)
    {
        bool passed_alphabetic_char = false;
        while (ed->cursor_x > 0) 
        {
            char c = text_buffer->rows[ed->cursor_y].data[ed->cursor_x - 1];
            
            if (editor_parser_isdelimiter(c) &&
                passed_alphabetic_char)
            {
                break;
            }
            else if (editor_parser_isalphabetic(c))
            {
                passed_alphabetic_char = true;
                ed->cursor_x--;
            }
            else ed->cursor_x--;
        }
    }
}

/* Delete text in the range between the cursor and the mark */
static void
editor_delete_range(editor_text_buffer *text_buffer, editor_state *ed)
{
    bool mark_equal_cursor = (ed->mark_x == ed->cursor_x) &&
        (ed->mark_y == ed->cursor_y);
    
    if (mark_equal_cursor) return;
    
    bool mark_ahead_of_cursor = (ed->mark_y > ed->cursor_y) ||
        ((ed->mark_y == ed->cursor_y) && (ed->mark_x > ed->cursor_x));
    
    // swap if mark position if ahead of cursor
    if (mark_ahead_of_cursor)
    {
        editor_cursor_mark_swap(text_buffer, ed);
    }
    
    while ((ed->cursor_x > ed->mark_x) ||
           (ed->cursor_y > ed->mark_y))
    {
        if (ed->cursor_x == 0) editor_delete_line(text_buffer, ed);
        else editor_backspace_delete(text_buffer, ed);
    }
}

static void 
editor_text_buffer_edit(editor_text_buffer *text_buffer, 
                        keyboard_input *keyboard, editor_state *ed)
{
    
    // control + z
    if (keyboard->control.endedDown &&
        keyboard->keys['z'].endedDown)
    {
        editor_operation op = editor_pop_op_from_stack(&ed->undo_stack);
        editor_execute_op(op, ed, text_buffer);
        return;
    }
    
    // control + d
    if (keyboard->control.endedDown &&
        keyboard->keys['d'].endedDown)
    {
        editor_delete_range(text_buffer, ed);
        return;
    }
    
    // control + X
    if (keyboard->control.endedDown &&
        keyboard->keys['d'].endedDown)
    {
        editor_cut_into_clipboard(text_buffer, ed);
        return;
    }
    
    // control + c
    if (keyboard->control.endedDown &&
        keyboard->keys['c'].endedDown)
    {
        editor_copy_into_clipboard(text_buffer, ed);
        return;
    }
    
    // control + v
    if (keyboard->control.endedDown &&
        keyboard->keys['v'].endedDown)
    {
        editor_paste_from_clipboard(text_buffer, ed);
        return;
    }
    
    for (u8 c = 32; c < 127; c++)
    {
        if (keyboard->keys[c].endedDown)
        {
            editor_insert_char(c, text_buffer, ed);
        }
    }
    
    // TODO(willian): might make a control flow that just filters
    // if the control and alt modifiers keys are being held
    
    // new line
    if (keyboard->enter.endedDown)
    {
        editor_insert_newline(text_buffer, ed);
    }
    
    // control + space
    else if(keyboard->control.endedDown && keyboard->spacebar.endedDown)
    {
        editor_set_mark(text_buffer, ed);
    }
    
    // space
    else if (keyboard->spacebar.endedDown)
    {
        editor_insert_char(' ', text_buffer, ed);
    }
    
    // control + backspace
    else if (keyboard->control.endedDown && keyboard->backspace.endedDown)
    {
        editor_delete_word_backward(text_buffer, ed);
    }
    
    // delete char or line
    else if (keyboard->backspace.endedDown)
    {
        if (ed->cursor_x > 0)
        {
            editor_backspace_delete(text_buffer, ed);
        }
        else if ((ed->cursor_x == 0) && (ed->cursor_y > 0))
        {
            editor_delete_line(text_buffer, ed);
        }
    }
}

static void
editor_set_mark(editor_text_buffer *text_buffer, editor_state *ed)
{
    ed->mark_x = ed->cursor_x;
    ed->mark_y = ed->cursor_y;
}

static void
editor_update_and_render(editor_screenbuffer *screen_buffer, editor_font *font, 
                         editor_text_buffer *text_buffer, keyboard_input *keyboard, 
                         editor_rectangle rect, editor_state *ed)
{
    u32 font_width = 9;
    
    // render bg
    editor_draw_background(screen_buffer,  ed->theme.background_color, rect);
    
    editor_text_buffer_edit(text_buffer, keyboard, ed);
    
    // update cursor
    editor_cursor_update(keyboard, ed, text_buffer);
    
    // TODO(willian): mark update function??
    
    editor_draw_line_highlight(screen_buffer, font, ed->theme.line_highlight_color,
                               ed, rect);
    
    editor_text_buffer_draw(screen_buffer, font, text_buffer, rect, ed);
    
    // render cursor
    
    editor_cursor_draw(screen_buffer, ed, font, ed->theme.cursor_color, rect);
    
    editor_mark_draw(screen_buffer, font, ed, ed->theme.mark_color, rect);
    
    char *text_name = "text name placeholder";
    
    editor_footer_draw(text_name, screen_buffer, rect, font, ed);
}
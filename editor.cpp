
/* EXPERIMENTATION  */

// [ ] - 60fps smooth scrolling

// [ ] - sub pixel rendering for better smooth scrolling, cursor animation, 
//         smooth text scrolling

// [ ] - directdraw clear font rendering -- directwrite

// [ ] - opengl rendering

// [ ] - mouse support for scrolling an setting cursor

/* EASY BUG FIXES TODO */
// [x] - make sure that the mark never stay out of the last_row range when deleting a range
//      maybe a check then update if necessary into the deleting function

// [x] - windows allocation an free functions, need more research on that

// [ ] - allowing other types of line endings

// [ ] - mark drawing is not accounting for the screen range

// [x] - not easy reproducible bug when pasting from the clipboard

// [x]  - still having bug when inserting new line, gotta verify if allocation
//          is sometimes failing

// [ ] - alt + movement keys is triggering sometimes, check alt.endeddown boolean,
//             check the key press handle

// [ ] - include a font based on the glyph width so we can use for the cursor and mark
//             operations and drawing

// TODO LIST THAT IS NOT CRUCIAL BUT IS NICE TO HAVE
// //     [] needs undo for cut op, and a compound for the insert char op, until we find
//                               a different op or a blank space op

//     [ ] not sure if I want multiple windows, but is nice to have it

//     [ ] config file for the editor

//     [ ] not really liking the "autocomple_mode" way, it conflicts with the edit mode
//                  making that every other keyboard key press than tab I need to
//                       switch back the modes 

// TODO LIST:
// 1 - Implement Mark editing style
//     - [x] set mark
//     - [x] mark/cursor swap
//     - [x] copy
//     - [x] delete
//     - [X] cut
//     - [x] paste

// 2 - searching
//     - [x] reverse searching
//     - [x] iterative searching

// 3 - [x] line deleting (backspace)

// 4 - [ ] log system

// 5 - [x] cursor clipping when drawing

// 6 - [x] change cursor and mark to be for the current file only

// 7 - FILE MANAGEMENT [INTERATIVE]
//     - [x] open file 
//     - [x] switch file 
//     - [x] save file
//     - [ ] new file
//     - [ ] close file
//     - [ ] handle failed open and save requests, alerting the user
//     - [ ] support for unix and dos end of line
//     - [ ] implement searching on the switch file prompt
//     - [ ] implement searching on the close file prompt

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
//           - [x] dirty file indicator
//           - [ ] end of line type
//           - [ ] encoding

// 10 - basic text editing and cursor movement
//      - [x] delete a word                      
//      - [x] move cursor between spaces vertically
//      - [x] move cursor to the next blank line
//      - [x] home and end cursor movement
//      - [x] begin of file and end of file cursor movement
//      - [x] line swapping
//      - [x] undo 
//      - [ ] redo
//      - [ ] tab autocomplete

// 11 - [ ] editor auto execute build file and display output

// 12 - [ ] editor parse the compiler output line error and jump on command

// 13 - [ ] goto line

// 14 - [x] 60 fps

// 15 - [ ] when the program is inactive or minimized, we just sleep and dont do
//             anything

// 16 - [ ] Editor parser and lexer
//          - [x] syntax highlighting
//          - [ ] Line indentation and virtual whitespacing
//          - [ ] Save Function definitions lines
//          - [ ] Save idenfitifers definitions for autocompletion

#include "editor.h"

#define EDITOR_PARSER_IMPLEMENTATION
#include "parser.h"

#include "editor_render.cpp"

static void *
editor_arena_alloc(u32 size, editor_memory_arena *arena)
{
    // TODO(willian): unnaligned allocation
    // init the arena
    if (!arena->buffer)
    {
        arena->buffer = (u8 *)malloc(MEGABYTE(1) * sizeof(arena->buffer[0]));
        arena->total_size = MEGABYTE(1);
    }
    
    // realloc the arena with a bigger size
    if ((arena->offset + size) > arena->total_size)
    { 
        ASSERT((arena->offset + size) <= arena->total_size);
    }
    
    void *ptr = &arena->buffer[arena->offset];
    arena->offset += size;
    
    // clear to zero
    memset(ptr, 0, size);
    return ptr;
}

/* add an opened file to the list */
void editor_text_buffer_list_add_node(editor_text_buffer *text_buffer,
                                      editor_state *ed)
{
    editor_text_buffer_node *new_node;
    new_node = (editor_text_buffer_node *)
        editor_arena_alloc(sizeof(*new_node), &ed->permanent_storage);
    
    if (!new_node) return;
    
    new_node->text_buffer = text_buffer;
    new_node->next = NULL;
    
    if (ed->opened_files_list.opened_file == NULL)
    {
        ed->opened_files_list.opened_file = new_node;
        ed->opened_files_list.number_of_files++;
        return;
    }
    
    editor_text_buffer_node *current = ed->opened_files_list.opened_file;
    while (current->next != NULL)
    {
        current = current->next;
    }
    
    current->next = new_node;
    ed->opened_files_list.number_of_files++;
}

static void
editor_execute_op(editor_operation op, editor_state *ed,
                  editor_text_buffer *text_buffer)
{
    editor_operation compound_op = {};
    switch (op.type)
    {
        case NO_OP:
        {
            break;
        }
        case INSERT_CHAR:
        {
            text_buffer->cursor_x = op.cursor_x;
            text_buffer->cursor_y = op.cursor_y;
            editor_backspace_delete(text_buffer, ed, false);
            break;
        }
        case BACKSPACE_DELETE_CHAR:
        {
            text_buffer->cursor_x = op.cursor_x;
            text_buffer->cursor_y = op.cursor_y;
            editor_insert_char(op.single_char, text_buffer, ed, false);
            break;
        }
        case BACKSPACE_DELETE_WORD:
        {
            for (u32 count = 0; count < op.compound_op_size; count++)
            {
                compound_op = editor_pop_op_from_stack(&ed->undo_stack);
                ASSERT(compound_op.type == BACKSPACE_DELETE_CHAR);
                text_buffer->cursor_x = compound_op.cursor_x;
                text_buffer->cursor_y = compound_op.cursor_y;
                editor_insert_char(compound_op.single_char, text_buffer, ed, false);
            }
            break;
        }
        case DELETE_LINE:
        {
            text_buffer->cursor_x = op.cursor_x;
            text_buffer->cursor_y = op.cursor_y;
            editor_insert_newline(text_buffer, ed, false);
            break;
        }
        case CREATE_LINE:
        {
            text_buffer->cursor_x = op.cursor_x;
            text_buffer->cursor_y = op.cursor_y;
            editor_delete_line(text_buffer, ed, false);
            break;
        }
        case PASTE_FROM_CLIPBOARD:
        {
            for (u32 count = 0; count < op.compound_op_size; count++)
            {
                compound_op = editor_pop_op_from_stack(&ed->undo_stack);
                ASSERT((compound_op.type == CREATE_LINE) ||
                       (compound_op.type == INSERT_CHAR));
                text_buffer->cursor_x = compound_op.cursor_x;
                text_buffer->cursor_y = compound_op.cursor_y;
                
                if (compound_op.type == CREATE_LINE)
                {
                    editor_delete_line(text_buffer, ed, false);
                }
                else
                {
                    editor_backspace_delete(text_buffer, ed, false);
                }
                
            }
            break;
        }
        case DELETE_RANGE:
        {
            for (u32 count = 0; count < op.compound_op_size; count++)
            {
                compound_op = editor_pop_op_from_stack(&ed->undo_stack);
                ASSERT((compound_op.type == BACKSPACE_DELETE_CHAR) ||
                       (compound_op.type == DELETE_LINE));
                text_buffer->cursor_x = compound_op.cursor_x;
                text_buffer->cursor_y = compound_op.cursor_y;
                
                if (compound_op.type == DELETE_LINE)
                {
                    editor_insert_newline(text_buffer, ed, false);
                }
                else
                {
                    editor_insert_char(compound_op.single_char, text_buffer, ed, false);
                }
                
            }
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
    editor_text_buffer *text_buffer = zen_tb_text_buffer_create(ZEN_TB_DEFAULT_TB_CAPACITY);
    
    // TODO(willian): log
    ASSERT(text_buffer);
    
    u32 line_start = 0;
    
    char *current_line = &file_buffer[0];
    
    editor_string line;
    
    for (char *c = file_buffer; ; c++)
    {
        char c0 = *c;
        char c1 = *(c + 1);
        
        if (c0 == 0)
        {
            file_buffer[line_start] = 0;
            
            line = zen_tb_line_create(current_line, ZEN_TB_DEFAULT_LINE_SIZE);
            
            // TODO(willian): LOG
            ASSERT(line);
            
            text_buffer = zen_tb_text_buffer_append_line(text_buffer, line);
            
            break;
        }
        
        // UNIX/LINUX LF
        else if (c0 == '\r' && c1 == '\n') 
        {
            
            file_buffer[line_start] = 0;
            
            line = zen_tb_line_create(current_line, ZEN_TB_DEFAULT_LINE_SIZE);
            
            // TODO(willian): LOG
            ASSERT(line);
            
            text_buffer = zen_tb_text_buffer_append_line(text_buffer, line);
            
            line_start += 2;
            
            current_line = &file_buffer[line_start];
            c++;
        }
        
        // UNIX/LINUX LF
        else if (c0 == '\n') 
        {
            
            file_buffer[line_start] = 0;
            
            line = zen_tb_line_create(current_line, ZEN_TB_DEFAULT_LINE_SIZE);
            
            // TODO(willian): LOG
            ASSERT(line);
            
            text_buffer = zen_tb_text_buffer_append_line(text_buffer, line);
            
            line_start++;
            
            current_line = &file_buffer[line_start];
        }
        
        else
            line_start++;
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
    // TODO(willian): pull to file buffer
    
    // iterative search buffer
    ed->search_buffer.data = (char *)calloc(1024 + 1, sizeof(char));
    ed->search_buffer.cursor = 0;
    ed->search_buffer.current_size = 0;
    ed->search_buffer.total_size = 1024;
    ed->search_buffer.found_match = false;
    
    // open file buffer
    ed->open_file_buffer.data = (char *)calloc(1024 + 1, sizeof(char));
    ed->open_file_buffer.cursor = 0;
    ed->open_file_buffer.current_size = 0;
    ed->open_file_buffer.total_size = 1024;
    ed->open_file_buffer.found_match = false;
    
    // swtich file buffer
    ed->switch_buffer.data = (char *)calloc(1024 + 1, sizeof(char));
    ed->switch_buffer.cursor = 0;
    ed->switch_buffer.current_size = 0;
    ed->switch_buffer.total_size = 1024;
    ed->switch_buffer.found_match = false;
    
    ed->list_of_tokens.tokens_string = parser_autocomplete_buffer_create(AUTOCOMPLETE_BUFFER_SIZE);
    ed->list_of_tokens.token_position = (char **)calloc(1024, sizeof (ed->list_of_tokens.token_position));
    ed->list_of_tokens.capacity = 1024;
    // 
    ed->mode = EDIT_MODE;
    
    // undo
    ed->undo_stack = {};
    
    // clipboards
    ed->copy_clipboard = {};
    ed->paste_clipboard = {};
    
    // color theme
    ed->theme.cursor_color = convert_uhex_to_v4(0xAA81a1c1);
    ed->theme.mark_color = convert_uhex_to_v4(0x8fbcbbff);
    ed->theme.identifier_color = convert_uhex_to_v4(0xFFd8dee9);
    ed->theme.keyword_color = convert_uhex_to_v4(0xFF5267ff);
    ed->theme.font_color = convert_uhex_to_v4(0xFFd8dee9);
    ed->theme.line_highlight_color = convert_uhex_to_v4(0xFF3b4252);
    ed->theme.background_color = convert_uhex_to_v4(0xFF2e3440);
    ed->theme.string_constant_color = convert_uhex_to_v4(0xFF6bb869);
    ed->theme.number_constant_color = ed->theme.string_constant_color;
    ed->theme.comment_color = ed->theme.string_constant_color;
    
    ed->running = true;
}

static void
editor_paste_from_clipboard(editor_text_buffer *text_buffer, editor_state *ed)
{
    // log
    if (!ed->paste_clipboard.data) return;
    
    for (u32 c = 0; c < ed->paste_clipboard.size - 1; c++)
    {
        // ignore CARRIAGE RETURN
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
    
    // push op to undo stack
    editor_operation op = {};
    op.type = PASTE_FROM_CLIPBOARD;
    op.cursor_x = text_buffer->cursor_x;
    op.cursor_y = text_buffer->cursor_y;
    op.compound_op_size = ed->paste_clipboard.size - 1;
    
    editor_push_op_into_stack(&ed->undo_stack, op);
}

/* Copy the caracters in our own editor clipboard, that is passed
*  to the platform code 
*/
static void
editor_copy_into_clipboard(editor_text_buffer *text_buffer,  editor_state *ed)
{
    bool mark_equal_cursor = (text_buffer->mark_x == text_buffer->cursor_x) &&
        (text_buffer->mark_y == text_buffer->cursor_y);
    
    if (mark_equal_cursor) return;
    
    // clean last clipboard
    if (ed->copy_clipboard.data) ed->platform_memory_free(ed->copy_clipboard.data);
    ed->copy_clipboard.data = 0;
    
    // save positions
    u32 old_cursor_x = text_buffer->cursor_x;
    u32 old_cursor_y = text_buffer->cursor_y;
    u32 old_mark_x = text_buffer->mark_x;
    u32 old_mark_y = text_buffer->mark_y;
    
    bool mark_ahead_of_cursor = (text_buffer->mark_y > text_buffer->cursor_y) ||
        ((text_buffer->mark_y == text_buffer->cursor_y) && (text_buffer->mark_x > text_buffer->cursor_x));
    
    bool cursor_ahead_of_mark = !mark_ahead_of_cursor;
    
    // swap if mark position if ahead of cursor
    if (mark_ahead_of_cursor) editor_cursor_mark_swap(text_buffer, ed);
    
    // NOTE(willian): this is so ugly
    u32 char_count = 0;
    while ((text_buffer->cursor_x > text_buffer->mark_x) ||
           (text_buffer->cursor_y > text_buffer->mark_y))
    {
        editor_cursor_move_left(ed, text_buffer);
        char_count++;
    }
    
    ed->copy_clipboard.size = char_count + 1;
    ed->copy_clipboard.data = (char *)calloc(ed->copy_clipboard.size, sizeof(char));
    ASSERT(ed->copy_clipboard.data);
    
    // restore positions
    text_buffer->cursor_x = old_cursor_x;
    text_buffer->cursor_y = old_cursor_y;
    text_buffer->mark_x = old_mark_x;
    text_buffer->mark_y = old_mark_y;
    
    // now we only swap if cursor is ahead of mark
    if (cursor_ahead_of_mark) editor_cursor_mark_swap(text_buffer, ed);
    
    u32 i = 0;
    while ((text_buffer->cursor_x < text_buffer->mark_x) ||
           (text_buffer->cursor_y < text_buffer->mark_y))
    {
        zen_tb_string line = text_buffer->rows[text_buffer->cursor_y];
        if (text_buffer->cursor_x == zen_tb_line_length(line))
        {
            ed->copy_clipboard.data[i++] = '\n';
        }
        else
        {
            ed->copy_clipboard.data[i++] = line[text_buffer->cursor_x];
        }
        editor_cursor_move_right(ed, text_buffer);
    }
    
    // debug
    ASSERT(i <= char_count);
    
    ed->copy_clipboard.has_changed = true;
    
    // restore
    text_buffer->cursor_x = old_cursor_x;
    text_buffer->cursor_y = old_cursor_y;
    text_buffer->mark_x = old_mark_x;
    text_buffer->mark_y = old_mark_y;
}

static void editor_cut_into_clipboard(editor_text_buffer *text_buffer,  editor_state *ed)
{
    editor_copy_into_clipboard(text_buffer, ed);
    editor_delete_range(text_buffer, ed);
}

static void
editor_cursor_mark_swap(editor_text_buffer *text_buffer, editor_state *ed)
{
    u32 temp_x = text_buffer->cursor_x;
    u32 temp_y = text_buffer->cursor_y;
    
    text_buffer->cursor_x = text_buffer->mark_x;
    text_buffer->cursor_y = text_buffer->mark_y;
    
    text_buffer->mark_x = temp_x;
    text_buffer->mark_y = temp_y;
}

static void
editor_cursor_move_left(editor_state *ed, editor_text_buffer *text_buffer)
{
    if ((text_buffer->cursor_x == 0) && (text_buffer->cursor_y > 0))
    {
        text_buffer->cursor_y--;
        text_buffer->cursor_x = zen_tb_line_length(text_buffer->rows[text_buffer->cursor_y]);
    }
    else if ((text_buffer->cursor_x > 0) || (text_buffer->cursor_y > 0))
    {
        text_buffer->cursor_x--;
    }
}

static void
editor_cursor_move_right(editor_state *ed, editor_text_buffer *text_buffer)
{
    if (((text_buffer->cursor_y + 1) >= text_buffer->length) &&
        (text_buffer->cursor_x == zen_tb_line_length(text_buffer->rows[text_buffer->cursor_y])))
    {
        return;
    }
    
    zen_tb_size line_length = zen_tb_line_length(text_buffer->rows[text_buffer->cursor_y]);
    
    if (text_buffer->cursor_x < line_length) 
    {
        text_buffer->cursor_x++;
    }
    else if (text_buffer->cursor_x == line_length &&
             text_buffer->cursor_y < text_buffer->length)
    {
        text_buffer->cursor_x = 0;
        text_buffer->cursor_y++;
    }
}

static void
editor_cursor_move_up(editor_state *ed, editor_text_buffer *text_buffer)
{
    if (text_buffer->cursor_y > 0) text_buffer->cursor_y--;
    
    zen_tb_size line_length = zen_tb_line_length(text_buffer->rows[text_buffer->cursor_y]);
    
    if (text_buffer->cursor_x > line_length)
    {
        text_buffer->cursor_x = line_length;
    }
}

static void
editor_cursor_move_down(editor_state *ed, editor_text_buffer *text_buffer)
{
    if ((text_buffer->cursor_y + 1) >= (text_buffer->length )) return;
    
    zen_tb_size line_length = zen_tb_line_length(text_buffer->rows[text_buffer->cursor_y + 1]);
    
    if (text_buffer->cursor_y < text_buffer->length)
    {
        text_buffer->cursor_y++;
    }
    
    if (text_buffer->cursor_x > line_length)
    {
        text_buffer->cursor_x = line_length;
    }
}

static void 
editor_cursor_update(keyboard_input *keyboard, editor_state *ed, editor_text_buffer *text_buffer)
{
    if (ed->mode == EDIT_MODE || ed->mode == AUTOCOMPLETE_MODE)
    {
        // ctrl + right
        if (keyboard->control.endedDown &&
            keyboard->keyRight.endedDown)
        {
            ed->mode = EDIT_MODE;
            editor_cursor_move_foward_word(text_buffer, ed);
        }
        // // ctrl + right
        else if (keyboard->control.endedDown &&
                 keyboard->keyLeft.endedDown)
        {
            ed->mode = EDIT_MODE;
            editor_cursor_move_backward_word(text_buffer, ed);
            
        }
        
        // // ctrl + down
        else if (keyboard->control.endedDown &&
                 keyboard->keyDown.endedDown)
        {
            ed->mode = EDIT_MODE;
            editor_cursor_move_next_blank_line(text_buffer, ed);
            text_buffer->cursor_x = 0;
        }
        
        // ctrl + up
        else if (keyboard->control.endedDown &&
                 keyboard->keyUp.endedDown)
        {
            ed->mode = EDIT_MODE;
            editor_cursor_move_previos_blank_line(text_buffer, ed);
            text_buffer->cursor_x = 0;
        }
        
        // ctrl + home 
        else if (keyboard->control.endedDown && keyboard->home.endedDown)
        {
            ed->mode = EDIT_MODE;
            text_buffer->cursor_x = 0;
            text_buffer->cursor_y = 0;
        }
        
        // ctrl + end
        else if (keyboard->control.endedDown && keyboard->end.endedDown)
        {
            if (text_buffer->length > 0)
            {
                
                text_buffer->cursor_y = text_buffer->length - 1;
            }
            ed->mode = EDIT_MODE;
            text_buffer->cursor_x = zen_tb_line_length(text_buffer->rows[text_buffer->cursor_y]);
        }
        
        // alt + up
        else if (keyboard->alt.endedDown && keyboard->keyUp.endedDown)
        {
            ed->mode = EDIT_MODE;
            editor_swap_line_up(text_buffer, ed);
        }
        
        // alt + down
        else if (keyboard->alt.endedDown && keyboard->keyDown.endedDown)
        {
            ed->mode = EDIT_MODE;
            editor_swap_line_down(text_buffer, ed);
        }
        
        // home 
        else if (keyboard->home.endedDown)
        {
            ed->mode = EDIT_MODE;
            text_buffer->cursor_x = 0;
        }
        
        // end
        else if (keyboard->end.endedDown)
        {
            ed->mode = EDIT_MODE;
            text_buffer->cursor_x = zen_tb_line_length(text_buffer->rows[text_buffer->cursor_y]);
        }
        
        // arrow right
        else if (keyboard->keyRight.endedDown) 
        {
            ed->mode = EDIT_MODE;
            editor_cursor_move_right(ed, text_buffer);
        }
        // arrow left
        else if (keyboard->keyLeft.endedDown) 
        {
            ed->mode = EDIT_MODE;
            editor_cursor_move_left(ed, text_buffer);
        }
        // arrow up
        else if (keyboard->keyUp.endedDown) 
        {
            ed->mode = EDIT_MODE;
            editor_cursor_move_up(ed, text_buffer);
        }
        // arrow down
        else if (keyboard->keyDown.endedDown) 
        {
            ed->mode = EDIT_MODE;
            editor_cursor_move_down(ed, text_buffer);
        }
    }
    ///// screen range update
    
    // vertical range
    
    // downward
    if (text_buffer->cursor_y >= text_buffer->text_range_y_end)
    {
        u32 diff  = (text_buffer->cursor_y - text_buffer->text_range_y_end);
        
        text_buffer->text_range_y_start += diff;
        text_buffer->text_range_y_end += diff;
    }
    
    // upward
    else if (text_buffer->cursor_y < text_buffer->text_range_y_start)
    {
        int diff = (text_buffer->text_range_y_start - text_buffer->cursor_y);
        
        text_buffer->text_range_y_end -= diff;
        text_buffer->text_range_y_start -= diff;
    }
    
    // TODO(willian): smooth transition
    // horizontal range
    if (text_buffer->cursor_x > text_buffer->text_range_x_end)
    {
        u32 diff  = (text_buffer->cursor_x - text_buffer->text_range_x_end);
        text_buffer->text_range_x_end += diff;
        text_buffer->text_range_x_start += diff;
    }
    
    else if (text_buffer->cursor_x < text_buffer->text_range_x_start)
    {
        u32 diff = (text_buffer->text_range_x_start - text_buffer->cursor_x);
        text_buffer->text_range_x_end -= diff;
        text_buffer->text_range_x_start -= diff;
    }
}

static void 
editor_insert_char(u8 c, editor_text_buffer *text_buffer, editor_state *ed, bool user_op)
{
    zen_tb_string line = text_buffer->rows[text_buffer->cursor_y];
    line = zen_tb_line_insert_char(line, c, text_buffer->cursor_x);
    text_buffer->rows[text_buffer->cursor_y] = line;
    text_buffer->cursor_x++;
    text_buffer->is_dirty = true;
    
    if (user_op)
    {
        editor_operation op = {};
        op.type = INSERT_CHAR;
        op.cursor_x = text_buffer->cursor_x;
        op.cursor_y = text_buffer->cursor_y;
        editor_push_op_into_stack(&ed->undo_stack, op);
    }
}

static void 
editor_insert_newline(editor_text_buffer *text_buffer, editor_state *ed, bool user_op)
{
    zen_tb_string line_to_slice = text_buffer->rows[text_buffer->cursor_y];
    zen_tb_string line_slice = &line_to_slice[text_buffer->cursor_x];
    
    zen_tb_string new_line = zen_tb_line_create(line_slice, ZEN_TB_DEFAULT_LINE_SIZE);
    
    ASSERT(new_line);
    
    zen_tb_line_set_length(line_to_slice, text_buffer->cursor_x);
    
    line_to_slice[zen_tb_line_length(line_to_slice)] = '\0';
    
    text_buffer->cursor_y++;
    text_buffer->cursor_x = 0;
    
    text_buffer = zen_tb_text_buffer_insert_line(text_buffer, new_line, text_buffer->cursor_y);
    
    text_buffer->is_dirty = true;
    
    if (user_op)
    {
        editor_operation op = {};
        op.type = CREATE_LINE;
        op.cursor_x = text_buffer->cursor_x;
        op.cursor_y = text_buffer->cursor_y;
        editor_push_op_into_stack(&ed->undo_stack, op);
    }
}

static void
editor_delete_line(editor_text_buffer *text_buffer, editor_state *ed, bool user_op)
{
    zen_tb_string line_to_append = text_buffer->rows[text_buffer->cursor_y - 1];
    
    zen_tb_string line_to_delete = text_buffer->rows[text_buffer->cursor_y];
    
    text_buffer->cursor_x = zen_tb_line_length(line_to_append);
    
    line_to_append = zen_tb_line_append(line_to_append, line_to_delete);
    
    zen_tb_text_buffer_line_delete(text_buffer, text_buffer->cursor_y);
    text_buffer->cursor_y--;
    text_buffer->is_dirty = true;
    
    if (user_op)
    {
        editor_operation op = {};
        op.type = DELETE_LINE;
        op.cursor_x = text_buffer->cursor_x;
        op.cursor_y = text_buffer->cursor_y;
        editor_push_op_into_stack(&ed->undo_stack, op);
    }
}

static void
editor_backspace_delete(editor_text_buffer *text_buffer, editor_state *ed, 
                        bool user_op)
{
    text_buffer->cursor_x--;
    zen_tb_string line = text_buffer->rows[text_buffer->cursor_y];
    char deleted_char = zen_tb_line_delete_char(line, text_buffer->cursor_x);
    
    text_buffer->is_dirty = true;
    if (user_op)
    {
        editor_operation op = {};
        op.type = BACKSPACE_DELETE_CHAR;
        op.cursor_x = text_buffer->cursor_x;
        op.cursor_y = text_buffer->cursor_y;
        op.single_char = deleted_char;
        
        editor_push_op_into_stack(&ed->undo_stack, op);
    }
}

static void
editor_swap_line_up(editor_text_buffer *text_buffer, editor_state *ed)
{
    if (text_buffer->cursor_y > 0)
    {
        zen_tb_string temp;
        temp = text_buffer->rows[text_buffer->cursor_y];
        text_buffer->rows[text_buffer->cursor_y] = text_buffer->rows[text_buffer->cursor_y - 1];
        text_buffer->rows[text_buffer->cursor_y - 1] = temp;
        text_buffer->cursor_y--;
        text_buffer->is_dirty = true;
    }
}

static void
editor_swap_line_down(editor_text_buffer *text_buffer, editor_state *ed)
{
    if ((text_buffer->cursor_y + 1) < text_buffer->length)
    {
        zen_tb_string temp;
        temp = text_buffer->rows[text_buffer->cursor_y];
        text_buffer->rows[text_buffer->cursor_y] = text_buffer->rows[text_buffer->cursor_y + 1];
        text_buffer->rows[text_buffer->cursor_y + 1] = temp;
        text_buffer->cursor_y++;
        text_buffer->is_dirty = true;
    }
}

static void
editor_delete_token_backward(editor_text_buffer *text_buffer, editor_state *ed)
{
    // NOTE(willian): when deleting a token backward we stop when encounter a token delimiter
    // or the beginning of a line
    u32 compound_op_count = 0;
    
    if (text_buffer->cursor_x > 0)
    {
        bool deleted_alphabetic_char = false;
        zen_tb_string line = text_buffer->rows[text_buffer->cursor_y];
        while (text_buffer->cursor_x > 0) 
        {
            char c = line[text_buffer->cursor_x - 1];
            
            if (editor_parser_is_token_delimiter(c) &&
                deleted_alphabetic_char)
            {
                break;
            }
            else if (editor_parser_isalphabetic(c))
            {
                editor_backspace_delete(text_buffer, ed);
                compound_op_count++;
                deleted_alphabetic_char = true;
            }
            else 
            {
                editor_backspace_delete(text_buffer, ed);
                compound_op_count++;
            }
        }
        // push into undo stack
        editor_operation op = {};
        op.type = BACKSPACE_DELETE_WORD;
        op.cursor_x = text_buffer->cursor_x;
        op.cursor_y = text_buffer->cursor_y;
        op.compound_op_size = compound_op_count;
        
        editor_push_op_into_stack(&ed->undo_stack, op);
    }
}

static void
editor_delete_word_backward(editor_text_buffer *text_buffer, editor_state *ed)
{
    // TODO(willian): we might want to implement the case where are at the beginning of a line
    // so we go up one line and do the backward delete
    u32 compound_op_count = 0;
    
    if (text_buffer->cursor_x > 0)
    {
        bool deleted_alphabetic_char = false;
        zen_tb_string line = text_buffer->rows[text_buffer->cursor_y];
        while (text_buffer->cursor_x > 0) 
        {
            char c = line[text_buffer->cursor_x - 1];
            
            if (editor_parser_isdelimiter(c) &&
                deleted_alphabetic_char)
            {
                break;
            }
            else if (editor_parser_isalphabetic(c))
            {
                editor_backspace_delete(text_buffer, ed);
                compound_op_count++;
                deleted_alphabetic_char = true;
            }
            else 
            {
                editor_backspace_delete(text_buffer, ed);
                compound_op_count++;
            }
        }
        // push into undo stack
        editor_operation op = {};
        op.type = BACKSPACE_DELETE_WORD;
        op.cursor_x = text_buffer->cursor_x;
        op.cursor_y = text_buffer->cursor_y;
        op.compound_op_size = compound_op_count;
        
        editor_push_op_into_stack(&ed->undo_stack, op);
    }
}

static void
editor_cursor_move_next_blank_line(editor_text_buffer *text_buffer, editor_state *ed)
{
    bool passed_non_blank_line = false;
    while ((text_buffer->cursor_y + 1) < text_buffer->length) 
    {
        u32 line_length = zen_tb_line_length(text_buffer->rows[text_buffer->cursor_y]);
        
        if (line_length == 0 && passed_non_blank_line)
        {
            break;
        }
        else if (line_length > 0)
        {
            passed_non_blank_line = true;
            text_buffer->cursor_y++;
        }
        else text_buffer->cursor_y++;
    }
}

static void 
editor_cursor_move_previos_blank_line(editor_text_buffer *text_buffer, editor_state *ed)
{
    bool passed_non_blank_line = false;
    while (text_buffer->cursor_y > 0)
    {
        u32 line_length = zen_tb_line_length(text_buffer->rows[text_buffer->cursor_y]);
        
        if (line_length == 0 && passed_non_blank_line)
        {
            break;
        }
        else if (line_length > 0)
        {
            passed_non_blank_line = true;
            text_buffer->cursor_y--;
        }
        else text_buffer->cursor_y--;
    }
}

static void
editor_cursor_move_foward_word(editor_text_buffer *text_buffer, editor_state *ed)
{
    bool passed_alphabetic_char = false;
    zen_tb_string line = text_buffer->rows[text_buffer->cursor_y];
    
    while (text_buffer->cursor_x < zen_tb_line_length(text_buffer->rows[text_buffer->cursor_y]))
    {
        char c = line[text_buffer->cursor_x];
        
        if (editor_parser_isdelimiter(c) &&
            passed_alphabetic_char)
        {
            break;
        }
        else if (editor_parser_isalphabetic(c))
        {
            passed_alphabetic_char = true;
            text_buffer->cursor_x++;
        }
        else text_buffer->cursor_x++;
    }
    
}

static void 
editor_cursor_move_backward_word(editor_text_buffer *text_buffer, editor_state *ed)
{
    bool passed_alphabetic_char = false;
    zen_tb_string line = text_buffer->rows[text_buffer->cursor_y];
    
    while (text_buffer->cursor_x > 0) 
    {
        char c = line[text_buffer->cursor_x - 1];
        if (editor_parser_isdelimiter(c) &&
            passed_alphabetic_char)
        {
            break;
        }
        else if (editor_parser_isalphabetic(c))
        {
            passed_alphabetic_char = true;
            text_buffer->cursor_x--;
        }
        else text_buffer->cursor_x--;
    }
}

/* Delete text in the range between the cursor and the mark */
static void
editor_delete_range(editor_text_buffer *text_buffer, editor_state *ed)
{
    bool mark_equal_cursor = (text_buffer->mark_x == text_buffer->cursor_x) &&
        (text_buffer->mark_y == text_buffer->cursor_y);
    
    if (mark_equal_cursor) return;
    
    bool mark_ahead_of_cursor = (text_buffer->mark_y > text_buffer->cursor_y) ||
        ((text_buffer->mark_y == text_buffer->cursor_y) && (text_buffer->mark_x > text_buffer->cursor_x));
    
    // swap if mark position if ahead of cursor
    if (mark_ahead_of_cursor)
    {
        editor_cursor_mark_swap(text_buffer, ed);
    }
    
    u32 delete_count = 0;
    while ((text_buffer->cursor_x > text_buffer->mark_x) ||
           (text_buffer->cursor_y > text_buffer->mark_y))
    {
        if (text_buffer->cursor_x == 0)
        {
            editor_delete_line(text_buffer, ed);
            delete_count++;
        }
        else
        {
            editor_backspace_delete(text_buffer, ed);
            delete_count++;
        }
    }
    
    // TODO(willian): we might want to save the mark too
    // so we can restore were it was before the delete
    editor_operation op = {};
    op.type = DELETE_RANGE;
    op.cursor_x = text_buffer->cursor_x;
    op.cursor_y = text_buffer->cursor_y;
    op.compound_op_size = delete_count;
    editor_push_op_into_stack(&ed->undo_stack, op);
}

static bool
editor_token_match(char *token, u32 token_length, char *match_str)
{
    if (token_length > strlen(match_str)) return false;
    
    for (u32 i = 0; i < token_length; i++)
    {
        if (token[i] != match_str[i]) return false;
    }
    return true;
}

static void
editor_autocomplete_query_matches(token_list list_of_tokens, token_to_query *token,
                                  editor_state *ed)
{
    token->matches_indexes = (u32 *)editor_arena_alloc(1024, &ed->frame_storage);
    token->current_match_index = 0;
    token->matches_count = 0;
    
    for (u32 index = 0; index < list_of_tokens.tokens_count; index++)
    {
        if (editor_token_match(token->token, token->length, list_of_tokens.token_position[index]))
        {
            token->matches_indexes[token->matches_count++] = index;
        }
    }
}

static u32
editor_seek_delimiter_to_the_left(editor_text_buffer *text_buffer)
{
    zen_tb_string line = text_buffer->rows[text_buffer->cursor_y];
    
    for (u32 pos = text_buffer->cursor_x; pos > 0; pos--)
    {
        if (editor_parser_is_token_delimiter(line[pos - 1]))
        {
            return pos;
        }
    }
    
    return 0;
}

static void
editor_autocomplete_token_init(editor_text_buffer *text_buffer, editor_state *ed)
{
    zen_tb_string line = text_buffer->rows[text_buffer->cursor_y]; 
    u32 line_pos = editor_seek_delimiter_to_the_left(text_buffer);
    
    char *tok_start = &line[line_pos];
    
    if (tok_start)
    {
        ASSERT(text_buffer->cursor_x >= line_pos);
        
        u32 tok_length = text_buffer->cursor_x - line_pos;
        
        if ( tok_length == 0) return;
        
        ed->frame_storage.offset = 0;
        ed->autocompletion_token.token = (char *)editor_arena_alloc(tok_length + 1, &ed->frame_storage);
        ed->autocompletion_token.length = tok_length;
        
        memcpy(ed->autocompletion_token.token, tok_start, tok_length);
        
        // create the buffer with the matches
        editor_autocomplete_query_matches(ed->list_of_tokens, &ed->autocompletion_token, ed);
        
        ed->mode = AUTOCOMPLETE_MODE;
    }
}

static void
editor_insert_match_into_line(char *match, editor_text_buffer *text_buffer, editor_state *ed)
{
    for (char *c = match; *c; c++)
    {
        editor_insert_char(*c, text_buffer, ed);
    }
}

#if 0
static void 
editor_text_buffer_edit(editor_text_buffer *text_buffer, 
                        keyboard_input *keyboard, editor_state *ed)
{
    if (keyboard->tab.endedDown)
    {
        // enter the autocomplete mode and return the matches
        if (ed->mode != AUTOCOMPLETE_MODE)
        {
            editor_autocomplete_token_init(text_buffer, ed);
        }
        
        // if we are alredy in the autocompletion mode, every tab press
        // we cycle through the matches and insert then in the text buffer
        
        // any other action when we are in the auto completion mode, we go back
        // to edit mode 
        
        if (ed->mode == AUTOCOMPLETE_MODE)
        {
            if ( ed->autocompletion_token.matches_count == 0) return;
            
            u32 index =
                ed->autocompletion_token.matches_indexes[ed->autocompletion_token.current_match_index];
            char *match = ed->list_of_tokens.token_position[index];
            
            if (match)
            {
                editor_delete_token_backward(text_buffer, ed);
                
                editor_insert_match_into_line(match, text_buffer, ed);
            }
            
            if ((ed->autocompletion_token.current_match_index + 1) >= ed->autocompletion_token.matches_count)
            {
                ed->autocompletion_token.current_match_index = 0;
            }
            else ed->autocompletion_token.current_match_index++;
        }
        
        // TODO(willian): any other action we get out of the AUTOCOMPLETE MODE AND CLEAR FRAME STORAGE
        
        return;
    }
    
    // control + i
    if (keyboard->control.endedDown &&
        keyboard->keys['i'].endedDown)
    {
        ed->mode = SWITCH_FILE_MODE;
        return;
    }
    
    // control + o
    if (keyboard->control.endedDown &&
        keyboard->keys['o'].endedDown)
    {
        ed->mode = OPEN_FILE_MODE;
        return;
    }
    
    // control + s
    if (keyboard->control.endedDown &&
        keyboard->keys['s'].endedDown)
    {
        if (text_buffer->is_dirty) editor_save_file(text_buffer);
        return;
    }
    
    // control + f
    if (keyboard->control.endedDown &&
        keyboard->keys['f'].endedDown)
    {
        ed->mode = SEARCH_MODE;
        return;
    }
    
    // control + z
    if (keyboard->control.endedDown &&
        keyboard->keys['z'].endedDown)
    {
        ed->mode = EDIT_MODE;
        editor_operation op = editor_pop_op_from_stack(&ed->undo_stack);
        editor_execute_op(op, ed, text_buffer);
        return;
    }
    
    // control + d
    if (keyboard->control.endedDown &&
        keyboard->keys['d'].endedDown)
    {
        ed->mode = EDIT_MODE;
        editor_delete_range(text_buffer, ed);
        return;
    }
    
    // control + X
    if (keyboard->control.endedDown &&
        keyboard->keys['x'].endedDown)
    {
        ed->mode = EDIT_MODE;
        editor_cut_into_clipboard(text_buffer, ed);
        return;
    }
    
    // control + c
    if (keyboard->control.endedDown &&
        keyboard->keys['c'].endedDown)
    {
        ed->mode = EDIT_MODE;
        editor_copy_into_clipboard(text_buffer, ed);
        return;
    }
    
    // control + v
    if (keyboard->control.endedDown &&
        keyboard->keys['v'].endedDown)
    {
        ed->mode = EDIT_MODE;
        editor_paste_from_clipboard(text_buffer, ed);
        return;
    }
    
    for (u8 c = 32; c < 127; c++)
    {
        if (keyboard->keys[c].endedDown)
        {
            ed->mode = EDIT_MODE;
            editor_insert_char(c, text_buffer, ed);
        }
    }
    
    // TODO(willian): might make a control flow that just filters
    // if the control and alt modifiers keys are being held
    
    // new line
    if (keyboard->enter.endedDown)
    {
        ed->mode = EDIT_MODE;
        editor_insert_newline(text_buffer, ed);
    }
    
    // control + space
    else if(keyboard->control.endedDown && keyboard->spacebar.endedDown)
    {
        ed->mode = EDIT_MODE;
        editor_set_mark(text_buffer, ed);
    }
    
    // space
    else if (keyboard->spacebar.endedDown)
    {
        ed->mode = EDIT_MODE;
        editor_insert_char(' ', text_buffer, ed);
    }
    
    // control + backspace
    else if (keyboard->control.endedDown && keyboard->backspace.endedDown)
    {
        ed->mode = EDIT_MODE;
        editor_delete_word_backward(text_buffer, ed);
    }
    
    // delete char or line
    else if (keyboard->backspace.endedDown)
    {
        if (text_buffer->cursor_x > 0)
        {
            ed->mode = EDIT_MODE;
            editor_backspace_delete(text_buffer, ed);
        }
        else if ((text_buffer->cursor_x == 0) && (text_buffer->cursor_y > 0))
        {
            ed->mode = EDIT_MODE;
            editor_delete_line(text_buffer, ed);
        }
    }
}
#endif

static void
editor_edit_list_files_to_open(editor_search_buffer *open_file_buffer, editor_state *ed)
{
    if (ed->open_file_buffer.found_match) return;
    
    ed->files_to_open.current_index = 0;
    
    // clear our temp storage
    ed->frame_storage.offset = 0;
    
    // wild card "*"
    char *search_str = (char *)editor_arena_alloc(1024, &ed->frame_storage);
    char *wild_card =  "*";
    
    memcpy(search_str, open_file_buffer->data, open_file_buffer->current_size);
    memcpy(search_str + open_file_buffer->current_size, wild_card, 1);
    
    WIN32_FIND_DATA find_data = {};
    HANDLE find_handle;
    
    u32 find_data_index = 0;
    
    find_handle = FindFirstFile(search_str, &find_data);
    
    if (find_handle)
    {
        // file attribute type
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            ed->files_to_open.files[find_data_index].type = FILE_TYPE_DIR;
        }
        else if (find_data.dwFileAttributes & FILE_ATTRIBUTE_NORMAL)
        {
            ed->files_to_open.files[find_data_index].type = FILE_TYPE_NORMAL;
        }
        else ed->files_to_open.files[find_data_index].type = FILE_TYPE_NOT_SUPPORTED;
        
        // only 32 bit sized files supported by now
        ed->files_to_open.files[find_data_index].size = find_data.nFileSizeLow;
        
        
        u32 filename_size = strlen(find_data.cFileName);
        
        ed->files_to_open.files[find_data_index].filename = 
            (char *)editor_arena_alloc(filename_size + 1, &ed->frame_storage);
        
        memcpy(ed->files_to_open.files[find_data_index].filename, find_data.cFileName, filename_size);
        
        
        find_data_index++;
        
        ed->open_file_buffer.found_match = true;
    }
    
    // TODO(willian): we gonna only hold the 100 first matches by now
    //  just for testing, then go bigger to shipping
    
    while (FindNextFile(find_handle, &find_data) != 0)
    {
        // file attribute type
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            ed->files_to_open.files[find_data_index].type = FILE_TYPE_DIR;
        }
        else if (find_data.dwFileAttributes & FILE_ATTRIBUTE_NORMAL)
        {
            ed->files_to_open.files[find_data_index].type = FILE_TYPE_NORMAL;
        }
        else ed->files_to_open.files[find_data_index].type = FILE_TYPE_NOT_SUPPORTED;
        
        // only 32 bit sized files supported by now
        ed->files_to_open.files[find_data_index].size = find_data.nFileSizeLow;
        
        u32 filename_size = strlen(find_data.cFileName);
        
        ed->files_to_open.files[find_data_index].filename = 
            (char *)editor_arena_alloc(filename_size + 1, &ed->frame_storage);
        
        memcpy(ed->files_to_open.files[find_data_index].filename, find_data.cFileName, filename_size);
        
        ed->files_to_open.files[find_data_index].filename_size = filename_size;
        
        if (find_data_index < 100)
        {
            find_data_index++;
        }
        else break;
    }
    
    ed->files_to_open.max_index = find_data_index;
    
}

static void
editor_draw_files_listing(editor_screenbuffer *screen_buffer,
                          editor_search_buffer *open_file_buffer, 
                          editor_files_listing *files_to_open, editor_state *ed,
                          editor_rectangle rect, editor_font *font)
{
    v4 list_bg_color = create_v4(0.5f, 0, 0.1f, 0.8f);
    v4 list_bg_color_hover = create_v4(0.6f, 0, 0.2f, 1.0f);
    
    u32 list_x = 0;
    u32 list_y = font->size;
    
    u32 text_x = 0;
    u32 text_y = font->size;
    
    for (u32 index = 0; index < files_to_open->max_index; index++)
    {
        v4 bg_color = files_to_open->current_index == index ?
            list_bg_color_hover : list_bg_color;
        
        editor_draw_rectangle(list_x, list_y, rect.dx - rect.x, font->size, 
                              bg_color, rect, screen_buffer);
        
        editor_draw_string(screen_buffer, files_to_open->files[index].filename,
                           files_to_open->files[index].filename_size, text_x, text_y,
                           ed, rect, font);
        
        text_y += font->size;
        list_y += font->size;
    }
}

static void editor_open_file_buffer(editor_search_buffer *open_file_buffer,
                                    editor_files_listing *files_to_open, editor_state *ed)
{
    char *filename = files_to_open->files[files_to_open->current_index].filename;
    u32 filename_length = files_to_open->files[files_to_open->current_index].filename_size;
    
    // find the slash of the directory
    while(open_file_buffer->current_size > 0)
    {
        if ((open_file_buffer->data[open_file_buffer->current_size - 1] == '\\') ||
            (open_file_buffer->data[open_file_buffer->current_size - 1] == '/'))
        {
            break;
        }
        else editor_prompt_delete_char(open_file_buffer);
    }
    
    editor_string fullpath = zen_string_make_define_capacity("", open_file_buffer->current_size + filename_length);
    
    // append file path
    fullpath = zen_string_append(fullpath, open_file_buffer->data);
    
    // append file name
    fullpath = zen_string_append(fullpath, filename);
    
    HANDLE file_handle = CreateFile(fullpath, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    
    if (file_handle == INVALID_HANDLE_VALUE)
    {
        zen_string_free(fullpath);
        return;
    }
    
    DWORD file_size = GetFileSize(file_handle, 0);
    
    char *buffer; 
    buffer = (char *)calloc((file_size) + 1, sizeof(char));
    
    if(!buffer)
    {
        CloseHandle(file_handle);
        zen_string_free(fullpath);
        return;
    }
    
    DWORD bytes_written;
    ReadFile(file_handle, buffer, file_size, &bytes_written, 0);
    
    // debug only
    ASSERT(file_size == bytes_written);
    
    CloseHandle(file_handle);
    
    ed->current_text_buffer = editor_text_buffer_create(buffer, ed);
    
    ed->current_text_buffer->filename = zen_string_make(filename);
    ed->current_text_buffer->fullpath = fullpath;
    
    editor_text_buffer_list_add_node(ed->current_text_buffer, ed);
    
    free(buffer);
}

static void
editor_open_file_prompt_append_dir(editor_files_listing *files_to_open,
                                   editor_search_buffer *open_file_buffer)
{
    char *dir_name = files_to_open->files[files_to_open->current_index].filename;
    u32 length = files_to_open->files[files_to_open->current_index].filename_size;
    
    for (u32 c = 0; c < length; c++)
    {
        editor_prompt_insert_char(dir_name[c], open_file_buffer);
    }
    editor_prompt_insert_char('/', open_file_buffer);
}

static void
editor_edit_open_file_prompt(editor_files_listing *files_to_open,
                             keyboard_input *keyboard, editor_search_buffer *open_file_buffer,
                             editor_state *ed)
{
    // space
    if (keyboard->control.endedDown)
    {
        return;
    }
    
    if (keyboard->enter.endedDown)
    {
        if (files_to_open->files[files_to_open->current_index].type ==
            FILE_TYPE_DIR)
        {
            editor_open_file_prompt_append_dir(files_to_open, open_file_buffer);
            open_file_buffer->found_match = false;
        }
        else
        {
            editor_open_file_buffer(open_file_buffer, files_to_open, ed);
            ed->frame_storage.offset = 0;
            ed->mode = EDIT_MODE;
        }
        return;
    }
    
    // reset arena and change to EDIT_MODE
    if (keyboard->escape.endedDown)
    {
        open_file_buffer->found_match = false;
        ed->frame_storage.offset = 0;
        ed->mode = EDIT_MODE;
        return;
    }
    
    // navigate files
    if (keyboard->keyDown.endedDown)
    {
        if ((files_to_open->current_index + 1 ) == files_to_open->max_index)
        {
            files_to_open->current_index = 0;
        }
        else files_to_open->current_index++;
        return;
    }
    
    if (keyboard->keyUp.endedDown)
    {
        if (files_to_open->current_index == 0)
        {
            if (files_to_open->max_index > 0)
            {
                files_to_open->current_index = files_to_open->max_index - 1;
            }
            else files_to_open->current_index = 0;
            
        }
        else files_to_open->current_index--;
        return;
    }
    
    if (keyboard->backspace.endedDown)
    {
        editor_prompt_delete_char(open_file_buffer);
        open_file_buffer->found_match = false;
        return;
    }
    
    for (u8 c = 32; c < 127; c++)
    {
        if (keyboard->keys[c].endedDown)
        {
            editor_prompt_insert_char(c, open_file_buffer);
            open_file_buffer->found_match = false;
        }
    }
    
    // space
    if (keyboard->spacebar.endedDown)
    {
        editor_prompt_insert_char(' ', open_file_buffer);
        open_file_buffer->found_match = false;
    }
}

static void
editor_set_mark(editor_text_buffer *text_buffer, editor_state *ed)
{
    text_buffer->mark_x = text_buffer->cursor_x;
    text_buffer->mark_y = text_buffer->cursor_y;
}

static void editor_prompt_delete_char(editor_search_buffer *prompt_buffer)
{ 
    if (prompt_buffer->cursor > 0)
    {
        prompt_buffer->cursor--;
        prompt_buffer->current_size--;
        prompt_buffer->data[prompt_buffer->cursor] = 0;
    }
}

static void editor_prompt_insert_char(u8 c, editor_search_buffer *prompt_buffer)
{
    if (prompt_buffer->cursor < prompt_buffer->total_size)
    {
        prompt_buffer->data[prompt_buffer->cursor++] = c;
        prompt_buffer->current_size++;
    }
}

static void
editor_edit_switch_prompt(keyboard_input *keyboard, editor_search_buffer *switch_buffer,
                          editor_state *ed)
{
    if (keyboard->control.endedDown)
    {
        return;
    }
    
    if (keyboard->enter.endedDown)
    {
        editor_switch_current_text_buffer(ed, &ed->opened_files_list);
        ed->mode = EDIT_MODE;
        return;
    }
    
    // reset the search buffer and change to EDIT_MODE
    if (keyboard->escape.endedDown)
    {
        ed->mode = EDIT_MODE;
        return;
    }
    
    if (keyboard->keyDown.endedDown)
    {
        if ((ed->opened_files_list.current_index + 1) ==
            ed->opened_files_list.number_of_files)
        {
            ed->opened_files_list.current_index = 0;
        }
        else ed->opened_files_list.current_index++;
        return;
    }
    
    if (keyboard->keyUp.endedDown)
    {
        if (ed->opened_files_list.current_index == 0)
        {
            if (ed->opened_files_list.number_of_files > 0)
            {
                ed->opened_files_list.current_index = 
                    ed->opened_files_list.number_of_files - 1;
            }
            else ed->opened_files_list.current_index = 0;
        }
        else ed->opened_files_list.current_index--;
        return;
    }
    
    if (keyboard->backspace.endedDown)
    {
        editor_prompt_delete_char(switch_buffer);
        switch_buffer->found_match = false;
        return;
    }
    
    for (u8 c = 32; c < 127; c++)
    {
        if (keyboard->keys[c].endedDown)
        {
            editor_prompt_insert_char(c, switch_buffer);
            switch_buffer->found_match = false;
        }
    }
    
    // space
    if (keyboard->spacebar.endedDown)
    {
        editor_prompt_insert_char(' ', switch_buffer);
        switch_buffer->found_match = false;
    }
}

static void
editor_edit_search_prompt(keyboard_input *keyboard,
                          editor_search_buffer *search_buffer,
                          editor_state *ed)
{
    
    if (keyboard->control.endedDown)
    {
        return;
    }
    
    // reset the search buffer and change to EDIT_MODE
    if (keyboard->escape.endedDown)
    {
        search_buffer->cursor = 0;
        search_buffer->current_size = 0;
        search_buffer->found_match = false;
        search_buffer->search_mode = SEARCH_MODE_NORMAL;
        ed->mode = EDIT_MODE;
        return;
    }
    
    if (keyboard->keyDown.endedDown)
    {
        search_buffer->found_match = false;
        search_buffer->skip_match = true;
        search_buffer->search_mode = SEARCH_MODE_NORMAL;
        return;
    }
    
    if (keyboard->keyUp.endedDown)
    {
        search_buffer->found_match = false;
        search_buffer->skip_match = true;
        search_buffer->search_mode = SEARCH_MODE_REVERSE;
        return;
    }
    
    if (keyboard->backspace.endedDown)
    {
        editor_prompt_delete_char(search_buffer);
        search_buffer->found_match = false;
        return;
    }
    
    for (u8 c = 32; c < 127; c++)
    {
        if (keyboard->keys[c].endedDown)
        {
            editor_prompt_insert_char(c, search_buffer);
            search_buffer->found_match = false;
        }
    }
    
    // space
    if (keyboard->spacebar.endedDown)
    {
        editor_prompt_insert_char(' ', search_buffer);
        search_buffer->found_match = false;
    }
}

/* strings need to be of the same size */
bool editor_string_compare(char *s1, char *s2, u32 size)
{
    for (u32 i = 0; i < size; i++)
    {
        if (s2[i] != s1[i])
        {
            return false;
        }
    }
    return true;
}

static void
editor_switch_current_text_buffer(editor_state *ed, editor_text_buffer_list *files_list)
{
    if (files_list->number_of_files == 0) return;
    
    ASSERT(files_list->current_index <=  files_list->number_of_files);
    
    editor_text_buffer_node *current = files_list->opened_file;
    
    for (u32 i = 0; i < files_list->current_index; i++)
    {
        current = current->next;
    }
    
    ed->current_text_buffer = current->text_buffer;
}

static void
editor_draw_opened_files(editor_screenbuffer *screen_buffer, editor_state *ed,
                         editor_rectangle rect, editor_font *font,
                         editor_text_buffer_list *files_list)
{
    v4 list_bg_color = create_v4(0.5f, 0, 0.1f, 0.8f);
    v4 list_bg_color_hover = create_v4(0.6f, 0, 0.2f, 1.0f);
    
    u32 list_x = 0;
    u32 list_y = font->size;
    
    u32 text_x = 0;
    u32 text_y = font->size;
    
    editor_text_buffer_node *current  = files_list->opened_file;
    
    for (u32 index = 0; index < files_list->number_of_files; index++)
    {
        v4 bg_color = files_list->current_index == index ?
            list_bg_color_hover : list_bg_color;
        
        editor_draw_rectangle(list_x, list_y, rect.dx - rect.x, font->size, 
                              bg_color, rect, screen_buffer);
        
        size_t filename_length = zen_string_length(current->text_buffer->filename);
        for (u32 c = 0; c < filename_length; c++)
        {
            char char_to_draw = current->text_buffer->filename[c];
            
            editor_draw_glyph(char_to_draw, text_x, text_y, ed->theme.font_color,
                              rect, screen_buffer, font);
            
            text_x += font->glyph[char_to_draw - 32].advance;
        }
        text_x = 0;
        text_y += font->size;
        list_y += font->size;
        current = current->next;
    }
}

/* updates the cursor postion if it find a match into the text buffer */
static void
editor_search_text_buffer(editor_text_buffer *text_buffer, 
                          editor_search_buffer *search_buffer,
                          editor_state *ed)
{
    // TODO(willian): bug proof this, some potential bugs in here
    
    if (search_buffer->current_size == 0) return;
    
    if (search_buffer->found_match) return;
    
    char *string = search_buffer->data;
    u32 size = search_buffer->current_size;
    
    // normal searching
    if (search_buffer->search_mode == SEARCH_MODE_NORMAL)
    {
        u32 row_start = text_buffer->cursor_y;
        for (zen_tb_string *row = &text_buffer->rows[text_buffer->cursor_y];
             (row + 1) < &text_buffer->rows[text_buffer->length];
             row++)
        {
            zen_tb_string line = *row;
            for (u32 c = 0; (c + size) < zen_tb_line_length(line); c++)
            {
                char *p = &line[c];
                if (editor_string_compare(string, p, size))
                {
                    if (search_buffer->skip_match)
                    {
                        search_buffer->skip_match = false;
                        continue;
                    }
                    text_buffer->cursor_x = c;
                    text_buffer->cursor_y = row_start;
                    search_buffer->found_match = true;
                    return;
                }
            }
            row_start++;
        }
    }
    
    // reverse searching
    else if (search_buffer->search_mode == SEARCH_MODE_REVERSE)
    {
        s32 c = text_buffer->cursor_x;
        
        u32 row_start = text_buffer->cursor_y;
        
        for (zen_tb_string *row = &text_buffer->rows[text_buffer->cursor_y];
             row >= &text_buffer->rows[0];
             row--)
        {
            zen_string line = *row;
            
            if ((zen_tb_line_length(line)) >= size)
            {
                for (; c >= 0; c--)
                {
                    char *p = &line[c];
                    if (editor_string_compare(string, p, size))
                    {
                        if (search_buffer->skip_match)
                        {
                            search_buffer->skip_match = false;
                            continue;
                        }
                        text_buffer->cursor_x = c;
                        text_buffer->cursor_y = row_start;
                        search_buffer->found_match = true;
                        return;
                    }
                    
                }
                c = zen_tb_line_length(line);
            }
            row_start--;
        }
    }
}

static void
editor_save_file(editor_text_buffer *text_buffer)
{
    // TODO(willian): any failure when saving the archive should be logged,
    // and have some visual cue alerting the user that something failed 
    // when trying to save, so the user can try to save again
    
    if (!text_buffer->fullpath) return;
    
    zen_tb_size size = 0;
    
    // size of each line + 1 for the line ending
    for (zen_tb_string *row = &text_buffer->rows[0];
         row < &text_buffer->rows[text_buffer->length];
         row++)
    {
        size += (zen_tb_line_length(*row) + 1);
    }
    
    char *buffer = (char *)calloc(size, sizeof(char));
    
    if (!buffer) return;
    
    char *dest = buffer;
    
    for (zen_tb_string *row = &text_buffer->rows[0];
         row < &text_buffer->rows[text_buffer->length];
         row++)
    {
        memcpy(dest, *row, zen_tb_line_length(*row));
        dest += (zen_tb_line_length(*row));
        *dest++ = '\n';
    }
    
    FILE *fp;
    fopen_s(&fp, text_buffer->fullpath, "w");
    
    if (!fp)
    {
        free(buffer);
    }
    
    size_t result = fwrite(buffer, 1, size, fp);
    
    // TODO(willian): LOG AND ALTERT THE USER
    ASSERT(result == size);
    
    text_buffer->is_dirty = false;
    
    fclose(fp);
    free(buffer);
}


static void 
editor_text_buffer_edit(editor_text_buffer *text_buffer, 
                        keyboard_input *keyboard, editor_state *ed)
{
    /* CONTROL + 'KEY' COMMANDS */
    if (keyboard->control.endedDown)
    {
        // switch files
        if (keyboard->keys['i'].endedDown) ed->mode = SWITCH_FILE_MODE;
        
        // open files
        else if (keyboard->keys['o'].endedDown) ed->mode = OPEN_FILE_MODE;
        
        // save current focused file
        else if (keyboard->keys['s'].endedDown && text_buffer->is_dirty) editor_save_file(text_buffer);
        
        // iterative search mode
        else if (keyboard->keys['f'].endedDown) ed->mode = SEARCH_MODE;
        
        // undo
        else if (keyboard->keys['z'].endedDown)
        {
            if (ed->mode == AUTOCOMPLETE_MODE) ed->mode = EDIT_MODE;
            
            if (ed->mode == EDIT_MODE)
            {
                
                editor_operation op = editor_pop_op_from_stack(&ed->undo_stack);
                editor_execute_op(op, ed, text_buffer);
            }
        }
        
        // delete range
        else if (keyboard->keys['d'].endedDown)
        {
            if (ed->mode == AUTOCOMPLETE_MODE) ed->mode = EDIT_MODE;
            
            if (ed->mode == EDIT_MODE) editor_delete_range(text_buffer, ed);
        }
        
        // cut range
        else if (keyboard->keys['x'].endedDown)
        {
            if (ed->mode == AUTOCOMPLETE_MODE) ed->mode = EDIT_MODE;
            
            if (ed->mode == EDIT_MODE) editor_cut_into_clipboard(text_buffer, ed);
        }
        
        // copy range
        else if (keyboard->keys['c'].endedDown)
        {
            if (ed->mode == AUTOCOMPLETE_MODE) ed->mode = EDIT_MODE;
            
            if (ed->mode == EDIT_MODE) editor_copy_into_clipboard(text_buffer, ed);
        }
        
        // paste
        else if (keyboard->keys['v'].endedDown)
        {
            if (ed->mode == AUTOCOMPLETE_MODE) ed->mode = EDIT_MODE;
            
            if (ed->mode == EDIT_MODE) editor_paste_from_clipboard(text_buffer, ed);
        }
        
        // set mark
        else if(keyboard->spacebar.endedDown)
        {
            if (ed->mode == EDIT_MODE) editor_set_mark(text_buffer, ed);
        }
        
        // delte token backward
        else if (keyboard->backspace.endedDown)
        {
            if (ed->mode == AUTOCOMPLETE_MODE) ed->mode = EDIT_MODE;
            
            if (ed->mode == EDIT_MODE) editor_delete_word_backward(text_buffer, ed);
        }
        return;
    }
    
    // character insertion
    for (u8 c = 32; c < 127; c++)
    {
        if (keyboard->keys[c].endedDown)
        {
            if (ed->mode == AUTOCOMPLETE_MODE) ed->mode = EDIT_MODE;
            
            if (ed->mode == EDIT_MODE) editor_insert_char(c, text_buffer, ed);
            
            else if (ed->mode == OPEN_FILE_MODE)
            {
                editor_prompt_insert_char(c, &ed->open_file_buffer);
                ed->open_file_buffer.found_match = false;
            }
            
            else if (ed->mode == SWITCH_FILE_MODE)
            {
                editor_prompt_insert_char(c, &ed->switch_buffer);
                ed->switch_buffer.found_match = false;
            }
            
            else if (ed->mode == SEARCH_MODE)
            {
                editor_prompt_insert_char(c, &ed->search_buffer);
                ed->search_buffer.found_match = false;
            }
        }
    }
    
    if (keyboard->enter.endedDown)
    {
        if (ed->mode == AUTOCOMPLETE_MODE) ed->mode = EDIT_MODE;
        
        // insert new line
        if (ed->mode == EDIT_MODE) editor_insert_newline(text_buffer, ed);
        
        // confirm open file prompt
        else if (ed->mode == OPEN_FILE_MODE)
        {
            if (ed->files_to_open.files[ed->files_to_open.current_index].type ==
                FILE_TYPE_DIR)
            {
                editor_open_file_prompt_append_dir(&ed->files_to_open, &ed->open_file_buffer);
                ed->open_file_buffer.found_match = false;
            }
            else
            {
                editor_open_file_buffer(&ed->open_file_buffer, &ed->files_to_open, ed);
                ed->frame_storage.offset = 0;
                ed->mode = EDIT_MODE;
            }
            return;
        }
        
        // confirm buffer switch
        else if (ed->mode == SWITCH_FILE_MODE)
        {
            editor_switch_current_text_buffer(ed, &ed->opened_files_list);
            ed->mode = EDIT_MODE;
        }
        
        // confirm the search
        else if (ed->mode == SEARCH_MODE)
        {
            ed->search_buffer.cursor = 0;
            ed->search_buffer.current_size = 0;
            ed->search_buffer.found_match = false;
            ed->search_buffer.search_mode = SEARCH_MODE_NORMAL;
            ed->mode = EDIT_MODE;
        }
    }
    
    // space
    else if (keyboard->spacebar.endedDown)
    {
        if (ed->mode == AUTOCOMPLETE_MODE) ed->mode = EDIT_MODE;
        
        if (ed->mode == EDIT_MODE) editor_insert_char(' ', text_buffer, ed);
        
        
        else if (ed->mode == OPEN_FILE_MODE)
        {
            editor_prompt_insert_char(' ', &ed->open_file_buffer);
            ed->open_file_buffer.found_match = false;
        }
        
        else if (ed->mode == SWITCH_FILE_MODE)
        {
            editor_prompt_insert_char(' ', &ed->switch_buffer);
            ed->switch_buffer.found_match = false;
        }
        
        
        else if (ed->mode == SEARCH_MODE)
        {
            editor_prompt_insert_char(' ', &ed->search_buffer);
            ed->search_buffer.found_match = false;
        }
    }
    
    // delete char or line
    else if (keyboard->backspace.endedDown)
    {
        if (ed->mode == AUTOCOMPLETE_MODE) ed->mode = EDIT_MODE;
        
        if (ed->mode == EDIT_MODE)
        {
            if (text_buffer->cursor_x > 0) editor_backspace_delete(text_buffer, ed);
            
            else if ((text_buffer->cursor_x == 0) && (text_buffer->cursor_y > 0)) editor_delete_line(text_buffer, ed);
        }
        
        else if (ed->mode == OPEN_FILE_MODE)
        {
            editor_prompt_delete_char(&ed->open_file_buffer);
            ed->open_file_buffer.found_match = false;
        }
        
        else if (ed->mode == SWITCH_FILE_MODE)
        {
            editor_prompt_delete_char(&ed->switch_buffer);
            ed->switch_buffer.found_match = false;
        }
        
        else if (ed->mode == SEARCH_MODE)
        {
            editor_prompt_delete_char(&ed->search_buffer);
            ed->search_buffer.found_match = false;
        }
    }
    
    // reset arena and change to EDIT_MODE
    else if (keyboard->escape.endedDown)
    {
        // exit open file mode
        if (ed->mode == OPEN_FILE_MODE)
        {
            ed->open_file_buffer.found_match = false;
            ed->frame_storage.offset = 0;
            ed->mode = EDIT_MODE;
        }
        
        // exit swtich file mode
        else if (ed->mode == SWITCH_FILE_MODE)
        {
            ed->mode = EDIT_MODE;
        }
        
        // exit search mode
        else if (ed->mode == SEARCH_MODE)
        {
            // // TODO(willian): GO BACK TO WHERE THE CURSOR WAS BEFORE THE SEARCH
        }
    }
    
    // navigate lister menu
    else if (keyboard->keyDown.endedDown)
    {
        if (ed->mode == OPEN_FILE_MODE)
        {
            if ((ed->files_to_open.current_index + 1 ) == ed->files_to_open.max_index)
            {
                ed->files_to_open.current_index = 0;
            }
            else ed->files_to_open.current_index++;
            
        }
        
        else if (ed->mode == SWITCH_FILE_MODE)
        {
            
            if ((ed->opened_files_list.current_index + 1) ==
                ed->opened_files_list.number_of_files)
            {
                ed->opened_files_list.current_index = 0;
            }
            else ed->opened_files_list.current_index++;
        }
        
        else if (ed->mode == SEARCH_MODE)
        {
            ed->search_buffer.found_match = false;
            ed->search_buffer.skip_match = true;
            ed->search_buffer.search_mode = SEARCH_MODE_NORMAL;
        }
    }
    
    // navigate lister menu
    else if (keyboard->keyUp.endedDown)
    {
        if (ed->mode == OPEN_FILE_MODE)
        {
            if (ed->files_to_open.current_index == 0)
            {
                if (ed->files_to_open.max_index > 0)
                {
                    ed->files_to_open.current_index = ed->files_to_open.max_index - 1;
                }
                else ed->files_to_open.current_index = 0;
                
            }
            else ed->files_to_open.current_index--;
        }
        
        
        else if (ed->mode == SWITCH_FILE_MODE)
        {
            if (ed->opened_files_list.current_index == 0)
            {
                if (ed->opened_files_list.number_of_files > 0)
                {
                    ed->opened_files_list.current_index = 
                        ed->opened_files_list.number_of_files - 1;
                }
                else ed->opened_files_list.current_index = 0;
            }
            else ed->opened_files_list.current_index--;
        }
        
        
        else if (ed->mode == SEARCH_MODE)
        {
            ed->search_buffer.found_match = false;
            ed->search_buffer.skip_match = true;
            ed->search_buffer.search_mode = SEARCH_MODE_REVERSE;
        }
    }
    
    else if (keyboard->tab.endedDown)
    {
        // enter the autocomplete mode and return the matches
        if (ed->mode != AUTOCOMPLETE_MODE)
        {
            editor_autocomplete_token_init(text_buffer, ed);
        }
        
        // if we are alredy in the autocompletion mode, every tab press
        // we cycle through the matches and insert then in the text buffer
        
        // any other action when we are in the auto completion mode, we go back
        // to edit mode 
        
        if (ed->mode == AUTOCOMPLETE_MODE)
        {
            if ( ed->autocompletion_token.matches_count == 0) return;
            
            u32 index =
                ed->autocompletion_token.matches_indexes[ed->autocompletion_token.current_match_index];
            char *match = ed->list_of_tokens.token_position[index];
            
            if (match)
            {
                editor_delete_token_backward(text_buffer, ed);
                
                editor_insert_match_into_line(match, text_buffer, ed);
            }
            
            if ((ed->autocompletion_token.current_match_index + 1) >= ed->autocompletion_token.matches_count)
            {
                ed->autocompletion_token.current_match_index = 0;
            }
            else ed->autocompletion_token.current_match_index++;
        }
    }
}

static void
editor_update_and_render(editor_screenbuffer *screen_buffer, editor_font *font, 
                         editor_text_buffer *text_buffer, keyboard_input *keyboard, 
                         editor_rectangle rect, editor_state *ed)
{
    /* ============== UPDATE ==============*/
    
    if (ed->mode == SEARCH_MODE)
    {
        // TODO(willian): no need to have this pointer here
        //editor_edit_search_prompt(keyboard, &ed->search_buffer, ed);
        
        editor_search_text_buffer(ed->current_text_buffer, &ed->search_buffer,ed);
    }
    
    editor_text_buffer_edit(ed->current_text_buffer, keyboard, ed);
    
    if (ed->mode == EDIT_MODE || ed->mode == AUTOCOMPLETE_MODE)
    {
        editor_parse_text_buffer(text_buffer, &ed->list_of_tokens);
    }
    
    if (ed->mode == OPEN_FILE_MODE)
    {
        //editor_edit_open_file_prompt(&ed->files_to_open, keyboard, &ed->open_file_buffer, ed);
        editor_edit_list_files_to_open(&ed->open_file_buffer, ed);
    }
    
    if (ed->mode == SWITCH_FILE_MODE)
    {
        //editor_edit_switch_prompt(keyboard, &ed->switch_buffer, ed);
    }
    
    editor_cursor_update(keyboard, ed, ed->current_text_buffer);
    
    /* ============== RENDER ==============*/
    
    // render background
    editor_draw_background(screen_buffer,  ed->theme.background_color, rect);
    
#if 1
    // remove this
    v4 rect3_color = {0, 0 , 1.0f, 0.6f};
    
    editor_draw_rectangle(300, 300, 100, 100, rect3_color,
                          rect, screen_buffer);
    
    v4 rect_color = {1.0f, 0 , 0, 0.1f};
    
    editor_draw_rectangle(250, 250, 100, 100, rect_color,
                          rect, screen_buffer);
    
    
    v4 rect2_color = {0, 1.0f , 0, 0.1f};
    
    editor_draw_rectangle(200, 200, 100, 100, rect2_color,
                          rect, screen_buffer);
#endif
    
    // line highlighting
    editor_draw_line_highlight(screen_buffer, font, ed->theme.line_highlight_color,
                               ed, rect);
    
    // text buffer
    editor_text_buffer_draw(screen_buffer, font, ed->current_text_buffer, rect, ed);
    
    //  cursor
    editor_cursor_draw(screen_buffer, ed, font, ed->theme.cursor_color, rect);
    
    // mark
    editor_mark_draw(screen_buffer, font, ed, ed->theme.mark_color, rect);
    
    if (ed->mode == SEARCH_MODE)
    {
        editor_prompt_draw("Search: ", &ed->search_buffer, ed->theme.background_color,
                           ed->theme.font_color, rect, screen_buffer, font);
        
    }
    
    if (ed->mode == OPEN_FILE_MODE)
    {
        editor_prompt_draw("Open: ", &ed->open_file_buffer, ed->theme.background_color,
                           ed->theme.font_color, rect, screen_buffer, font);
        
        editor_draw_files_listing(screen_buffer, &ed->open_file_buffer, &ed->files_to_open, ed,
                                  rect, font);
    }
    
    if (ed->mode == SWITCH_FILE_MODE)
    {
        editor_prompt_draw("Switch: ", &ed->switch_buffer, ed->theme.background_color,
                           ed->theme.font_color, rect, screen_buffer, font);
        
        editor_draw_opened_files(screen_buffer, ed, rect, font, &ed->opened_files_list);
    }
    
    // footer
    editor_footer_draw(ed->current_text_buffer, screen_buffer, rect, font, ed);
}
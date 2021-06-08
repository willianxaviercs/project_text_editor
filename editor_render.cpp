#include "editor_render.h"

/*  math  */
inline u32
editor_u32_clamp(u32 min, u32 x, u32 max)
{
    if (x < min) return min;
    else if (x > max) return max;
    return x;
}

inline u32
round_float_to_u32(float n)
{
    u32 result = (u32)((n) + 0.5f);
    return result;
}

static void
editor_draw_rectangle(u32 x, u32 y, u32 width, u32 height, v4 color, 
                      editor_rectangle clipping_rect, editor_screenbuffer *screen_buffer)
{
    u32 buffer_x = clipping_rect.x + x;
    u32 buffer_y = clipping_rect.y + y;
    
    u32 clipped_width = editor_u32_clamp(0, (width + x), (clipping_rect.dx - clipping_rect.x));
    u32 clipped_height = editor_u32_clamp(0, (height + y), (clipping_rect.dy - clipping_rect.y));
    
    u32 sr = round_float_to_u32(color.r * 255.0f);
    u32 sg = round_float_to_u32(color.g * 255.0f);
    u32 sb = round_float_to_u32(color.b * 255.0f);
    float alpha = color.a;
    
    u8 *row = (u8 *)screen_buffer->memory;
    row += (buffer_x * screen_buffer->bytes_per_pixel) + (buffer_y * screen_buffer->pitch);
    
    for(u32 pos_y = y; pos_y < clipped_height; pos_y++)
    {
        u32 *pixel = (u32 *)row;
        for(u32 pos_x = x; pos_x < clipped_width; pos_x++)
        {
            u8 dr = (u8)(*pixel >> 16);
            u8 dg = (u8)(*pixel >> 8);
            u8 db = (u8)(*pixel);
            
            // ALPHA BLENDING
            // Value = Value0(1.0 - Alpha) + Value1(Alpha)
            
            u8 red   = (u8)(dr * (1.0f - alpha) + sr * alpha + 0.5f);
            u8 green = (u8)(dg * (1.0f - alpha) + sg * alpha + 0.5f);
            u8 blue  = (u8)(db * (1.0f - alpha) + sb * alpha + 0.5f);
            
            *pixel++ = (red << 16) | (green << 8) | (blue);
        }
        row += screen_buffer->pitch;
    }
}

static void 
editor_draw_glyph(u32 c, int x, int y, v4 color, editor_rectangle rect, 
                  editor_screenbuffer *screen_buffer, editor_font *font)
{
    int c_index;
    if (c < 32 || c > 255) c_index = 64;
    else c_index = c - 32; 
    
    u32 pos_x = x + font->glyph[c_index].xOffset;
    u32 pos_y = y + font->glyph[c_index].yOffset;
    
    u32 width = font->glyph[c_index].width;
    u32 height = font->glyph[c_index].height;
    
    u32 clipped_width = editor_u32_clamp(0, (width + pos_x), (rect.dx - rect.x));
    u32 clipped_height = editor_u32_clamp(0, (height + pos_y), (rect.dy - rect.y));
    
    u32 buffer_x = rect.x + pos_x;
    u32 buffer_y = rect.y + pos_y;
    
    u32 sr = round_float_to_u32(color.r * 255.0f);
    u32 sg = round_float_to_u32(color.g * 255.0f);
    u32 sb = round_float_to_u32(color.b * 255.0f);
    float alpha = color.a;
    
    u8 *row = (u8 *)screen_buffer->memory;
    row += (buffer_x * screen_buffer->bytes_per_pixel) + (buffer_y * screen_buffer->pitch);
    
    u8 *bitmap = font->glyph[c_index].bitmap;
    
    for (u32 j = pos_y; j < clipped_height; ++j)
    {
        u32 *pixel = (u32 *)row;
        u8 *bmp_point = bitmap; 
        for (u32 i = pos_x; i < clipped_width; ++i)
        {
            u8 alpha_point = *bmp_point++;
            
            float alpha = (float)alpha_point / 255.0f;
            
            u8 dr = (u8)(*pixel >> 16);
            u8 dg = (u8)(*pixel >> 8);
            u8 db = (u8)(*pixel);
            
            u8 red   = (u8)(dr * (1.0f - alpha) + sr * alpha);
            u8 green = (u8)(dg * (1.0f - alpha) + sg * alpha);
            u8 blue  = (u8)(db * (1.0f - alpha) + sb * alpha);
            
            *pixel++ = (red << 16) | (green << 8) | (blue);
        }
        bitmap += width;
        row += screen_buffer->pitch;
    }
}

static void
editor_draw_string(editor_screenbuffer *screen_buffer,
                   char *string, zen_tb_size string_size, u32 x, u32 y,
                   editor_state *ed,
                   editor_rectangle rect, editor_font *font)
{
    u32 str_x = x;
    
    for (u32 c = 0; c < string_size; c++)
    {
        char char_to_draw = string[c];
        
        editor_draw_glyph(char_to_draw, str_x, y, ed->theme.font_color,
                          rect, screen_buffer, font);
        
        str_x += font->glyph[char_to_draw - 32].advance;
    }
}

static void
editor_prompt_draw(const char *lister_label, editor_search_buffer *lister_buffer,
                   v4 bg_color, v4 text_color, editor_rectangle rect, 
                   editor_screenbuffer *screen_buffer,editor_font *font)
{
    // prompt background
    editor_draw_rectangle(0, 0, rect.dx - rect.x, font->size,
                          bg_color, rect, screen_buffer);
    
    int text_x = 0;
    int text_y = 0;
    
    // label
    // TODO(willian): pull 
    v4 prompt_label_color = {1.0f, 0 , 1.0f , 1.0f};
    u32 label_size = strlen(lister_label);
    
    for (u32 index = 0; index < label_size; index++)
    {
        editor_draw_glyph(lister_label[index], text_x, text_y, prompt_label_color, rect, screen_buffer, font);
        text_x += font->glyph[lister_label[index] - 32].advance;
    }
    
    // search text
    for (u32 index = 0; index < lister_buffer->current_size; index++)
    {
        u8 c = lister_buffer->data[index];
        
        editor_draw_glyph(c, text_x, text_y, text_color, rect, screen_buffer, font);
        text_x += font->glyph[c - 32].advance;
    }
}

static void
editor_draw_line_highlight(editor_screenbuffer *screen_buffer, editor_font *font, 
                           v4 color, editor_state *ed, editor_rectangle rect)
{
    u32 x = 0;
    u32 y = (ed->current_text_buffer->cursor_y  - ed->current_text_buffer->text_range_y_start) * 
        font->size;
    
    u32 width = rect.dx - rect.x;
    u32 height = font->size;
    
    editor_draw_rectangle(x, y, width, height, color, rect, screen_buffer);
}

static void 
editor_mark_draw(editor_screenbuffer *screen_buffer, editor_font *font, editor_state *ed,
                 v4 color, editor_rectangle rect)
{
    editor_text_buffer *tb = ed->current_text_buffer;
    // NOTE(willian): check if the mark is in the screen range
    if ((tb->mark_x < tb->text_range_x_start) ||
        (tb->mark_x > tb->text_range_x_end)) return;
    if ((tb->mark_y < tb->text_range_y_start) ||
        (tb->mark_y > tb->text_range_y_end)) return;
    
    u32 mark_height = font->size;
    u32 mark_width = 7;
    
    u32 mark_x = (tb->mark_x * mark_width) + rect.x;
    u32 mark_y = (tb->mark_y * mark_height) + rect.y;
    
    u32 mark_dx = mark_x + mark_width;
    u32 mark_dy = mark_y + mark_height;
    
    // TODO(willian): clipping
    
    // TODO(willian): we move to a pre calculation in the game loop
    u32 pitch = screen_buffer->width * screen_buffer->bytes_per_pixel;
    
    //basic super ugly clipping
    u32 *screen_range = (u32 *)screen_buffer->memory + 
        screen_buffer->width * screen_buffer->height;
    
    u8 sr = (u8)(color.r * 255.0f);
    u8 sg = (u8)(color.g * 255.0f);
    u8 sb = (u8)(color.b * 255.0f);
    float alpha = color.a;
    
    if(screen_buffer->memory) 
    {
        u8 *row = (u8 *)screen_buffer->memory + ((mark_x * screen_buffer->bytes_per_pixel) 
                                                 + (mark_y * pitch));
        
        for(u32 y = mark_y; y < mark_dy; ++y)
        {
            u32 *pixel = (u32 *)row;
            for(u32 x = mark_x; x < mark_dx; ++x)
            {
                if ((x == mark_x) || x == (mark_dx - 1) ||
                    (y == mark_y) || (y == (mark_dy - 1)))
                {
                    if (pixel <= screen_range)
                    {
                        u8 dr = (u8)(*pixel);
                        u8 dg = (u8)(*pixel >> 8);
                        u8 db = (u8)(*pixel >> 16);
                        
                        float red = (1.0f - alpha) * dr + alpha * sr;
                        float green = (1.0f - alpha) * dg + alpha * sg;
                        float blue = (1.0f - alpha) * db + alpha * sb;
                        
                        *pixel = (((u8)blue) << 16) | (((u8)green) << 8) | ((u8)(red));
                    }
                }
                
                pixel++;
            }
            row += pitch;
        }
    }
}

static void 
editor_draw_background(editor_screenbuffer *screen_buffer, v4 color, editor_rectangle rect)
{
    editor_draw_rectangle(0, 0, rect.dx - rect.x, rect.dy, color, rect, screen_buffer);
}

static void 
editor_cursor_draw(editor_screenbuffer *screen_buffer, editor_state *ed, editor_font *font, 
                   v4 cursor_color, editor_rectangle rect)
{
    editor_text_buffer *tb = ed->current_text_buffer;
    
    u32 x = (tb->cursor_x - tb->text_range_x_start) * font->width;
    u32 y = (tb->cursor_y - tb->text_range_y_start) * font->size;
    
    u32 width = font->width;
    u32 height = font->size;
    
    editor_draw_rectangle(x, y, width, height, cursor_color, rect, screen_buffer);
}

static inline v4
editor_render_get_glyph_color(unsigned char meta_c, editor_theme theme)
{
    
    if (meta_c == IDENTIFIER_TOKEN)
    {
        return theme.identifier_color;
    }
    
    else if (meta_c == KEYWORD_TOKEN)
    {
        return theme.keyword_color;
    }
    
    
    else if (meta_c == MULTILINE_STRING_CONSTANT_TOKEN)
    {
        return theme.string_constant_color;
    }
    
    else if (meta_c == STRING_CONSTANT_TOKEN)
    {
        return theme.string_constant_color;
    }
    
    else if (meta_c == NUMBER_CONSTANT_TOKEN)
    {
        return theme.number_constant_color;
    }
    
    else if (meta_c == SINGLE_COMMENT_TOKEN)
    {
        return theme.comment_color;
    }
    
    else if (meta_c == MULTI_COMMENT_TOKEN)
    {
        return theme.comment_color;
    }
    
    else if (meta_c == PREPROCESSOR_TOKEN)
    {
        return theme.preprocessor_color;
    }
    
    
    else if (meta_c == DELIMITER_TOKEN)
    {
        return {1.0f, 0, 1.0f, 1.0f};
    }
    
    
    else
    {
        return theme.default_color;
    }
    
}

/* draw the rect range of the text buffer */
static void 
editor_text_buffer_draw(editor_screenbuffer *screen_buffer,editor_font *font, 
                        editor_text_buffer *text_buffer, editor_rectangle rect,
                        editor_state *ed)
{
    u32 x = 0;
    u32 y = 0;
    
    u32 end = text_buffer->text_range_y_end >= text_buffer->length ?
        text_buffer->length : text_buffer->text_range_y_end;
    
    for (zen_tb_string *row = &text_buffer->rows[text_buffer->text_range_y_start];
         row < (&text_buffer->rows[end]);
         row++)
    {
        unsigned char *meta_line = zen_tb_line_get_meta_string(*row);
        
        for (size_t i = 0; i < zen_tb_line_length(*row); i++)
        {
            
            v4 char_color = editor_render_get_glyph_color(meta_line[i], ed->theme);
            
            char c = (*row)[i];
            
            editor_draw_glyph(c, x, y, char_color, 
                              rect, screen_buffer, font);
            x += font->glyph[(c) - 32].advance;
        }
        x = 0;
        y += font->size;
    }
    
#if 0
    // NOTE(willian): experimental smooth scrolling prototype
    
    // TODO(willian): we might change it all of this to use float values all the time,
    //                 and just round truncate it to integer when we
    //                     actually gonna use it for indexing
    
    // render line by line
    int x = 0;
    int y = 0;
    
    // TODO(willian): gotta test for when the range is greater then the rows with
    // have allocated, we might overrun the buffer
    
    //u32 row_count = ed->text_range_y_end > text_buffer->last_row ? 
    //text_buffer->last_row : ed->text_range_y_end;
    
    u32 y_range_start = ed->multi_line_smooth ?
        ed->smooth_range_y_start : ed->text_range_y_start;
    
#if 1
    u32 row_count = ed->smooth_range_y_end > text_buffer->last_row ? 
        text_buffer->last_row : ed->smooth_range_y_end;
#endif
#if 0
    u32 row_count = ed->multi_line_smooth ?
        ed->smooth_range_y_end : ed->text_range_y_end;
#endif
    
    u32 bg_row = 0;
    for (u32  row = y_range_start; row <= row_count; row++)
    {
        u32 text_count = ed->text_range_x_end > text_buffer->rows[row].length ? 
            text_buffer->rows[row].length: ed->text_range_x_end;
        
        for (u32 c = ed->text_range_x_start; c < text_buffer->rows[row].length; c++)
        {
            char char_to_draw = text_buffer->rows[row].data[c];
            
            //s32 smooth_x;
#if 0
            s32 smooth_y = ed->font_snap_upward ? 
                -ed->font_smooth_snap_y : ed->font_smooth_snap_y;
#endif
            s32 smooth_y = 0;
            
            editor_draw_glyph(char_to_draw, x, y + smooth_y, ed->theme.font_color, 
                              rect, screen_buffer, font);
            x += font->glyph[char_to_draw - 32].advance;
        }
        x = 0;
        y += font->size;
    }
    
    u32 line_inc = (u32)((0.5f * ed->delta_time * 255.0f) + 0.5f);
    if (ed->font_snap_upward)
    {
        
        // TODO(willian): if the number go into negative it will wrap
        // and cause a bug
        // upward smooth scrolling
        if (ed->smooth_range_y_start > ed->text_range_y_start)
        {
            ed->smooth_range_y_start -= line_inc;
        }
        if (ed->smooth_range_y_end > ed->text_range_y_end)
        {
            ed->smooth_range_y_end -= line_inc;
        }
        // correction if we pass too much
        if (ed->smooth_range_y_end < ed->text_range_y_end)
        {
            ed->smooth_range_y_end = ed->text_range_y_end;
            ed->smooth_range_y_start = ed->text_range_y_start;
        }
    }
    
    else
    {
        // downward smooth scrolling 
        if (ed->smooth_range_y_start < ed->text_range_y_start)
        {
            ed->smooth_range_y_start += line_inc;
        }
        
        if (ed->smooth_range_y_end < ed->text_range_y_end)
        {
            ed->smooth_range_y_end += line_inc;
        }
        
        // correction if we pass too much
        if (ed->smooth_range_y_end > ed->text_range_y_end)
        {
            ed->smooth_range_y_end = ed->text_range_y_end;
            ed->smooth_range_y_start = ed->text_range_y_start;
        }
    }
    
    // TODO(willian): if the number go into negative it will wrap
    // and cause a bug
    if (ed->smooth_range_y_start == ed->text_range_y_start)
    {
        u32 pixel_inc = (u32)((.5f * ed->delta_time * 255.0f) + 0.5f);
        
        // unsiged wrap bug could happen here
        if (ed->font_smooth_snap_y > 0) ed->font_smooth_snap_y -= pixel_inc;
        
        ed->multi_line_smooth = false;
    }
    
    // NOTE(willian): font size snap animation (experimental)
#endif
    
}

static void
editor_footer_draw(editor_text_buffer *text_buffer, editor_screenbuffer *screen_buffer, 
                   editor_rectangle rect, editor_font *font, editor_state *ed)
{
    // footer background color
    v4 bg_color = create_v4(0.5f, 0.5f, 0.5f, 1.0f);
    
    u32 bg_x = 1;
    u32 bg_y = (rect.dy - rect.y) - font->size - 1;
    u32 bg_width = rect.dx - rect.x - 1;
    u32 bg_height = font->size;
    
    editor_draw_rectangle(bg_x, bg_y, bg_width, bg_height,
                          bg_color, rect, screen_buffer);
    
    char footer_buffer[2048];
    
    // render line by line
    int x = 2;
    int y = (rect.dy - rect.y) - font->size;
    
    u32 color = 0x11ddFFFF; // footer font color
    
    // TODO(willian): write our own string format funtion
    int cx;
    cx = _snprintf_s(footer_buffer, 2048, _TRUNCATE, "%s - L#%d C#%d - %c",
                     text_buffer->filename, text_buffer->cursor_y + 1, text_buffer->cursor_x + 1,
                     text_buffer->is_dirty ? '*': ' ');
    
    // draw text_buffer name
    for (char *c = footer_buffer; *c; c++)
    {
        editor_draw_glyph(*c, x, y, convert_uhex_to_v4(color), rect, screen_buffer, font);
        x += font->glyph[(*c) - 32].advance;
    }
}

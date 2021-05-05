#include "editor_render.h"

static void 
editor_draw_glyph(u32 c, int x, int y, v4 color, editor_rectangle rect, 
                  editor_screenbuffer *screen_buffer, editor_font *font)
{
    u8 point;
    int c_index = c - 32;
    int i, j;
    
    if (c_index < 0) return;
    
    int actual_x = rect.x + x + font->glyph[c_index].xOffset;
    int actual_y = rect.y + y + font->glyph[c_index].yOffset;
    
    u32 *start_memory = (u32 *)screen_buffer->memory;
    u32 *end_memory = start_memory + screen_buffer->width * screen_buffer->height;
    
    u32 *pixel = start_memory + actual_x + (actual_y * screen_buffer->width);
    u32 *pixel_end = start_memory + rect.dx + rect.dy * screen_buffer->width;
    
    u8 sr = (u8)(color.r * 255.0f);
    u8 sg = (u8)(color.g * 255.0f);
    u8 sb = (u8)(color.b * 255.0f);
    
    for (j = 0; j < font->glyph[c_index].height; ++j)
    {
        u32 *col_bound = pixel + (rect.dx - actual_x);
        for (i = 0; i < font->glyph[c_index].width; ++i)
        {
            if (pixel > end_memory || pixel < start_memory || pixel > pixel_end)
            {
                break;
            }
            else if (pixel >= col_bound)
            {
                break;
            }
            
            point = font->glyph[c_index].bitmap[j * font->glyph[c_index].width + i];
            
            // TODO(willian): by now we are using linear blending
            // but should experiment with other types of blending too
            // also this code need heavily optimization
            // because its is very slow
            float alpha = (float)point / 255.0f;
            
            u8 dr = (u8)(*pixel);
            u8 dg = (u8)(*pixel >> 8);
            u8 db = (u8)(*pixel >> 16);
            
            float red = (1.0f - alpha) * dr + alpha * sr;
            float green = (1.0f - alpha) * dg + alpha * sg;
            float blue = (1.0f - alpha) * db + alpha * sb;
            
            // TODO(willian): make the safe truncate here
            
            *pixel++ = (((u8)(blue)) << 16) | (((u8)(green) << 8) | ((u8)(red)));
        }
        pixel += screen_buffer->width - i;
    }
}

// TODO(willian): clipping/ out of bound prevention
static void
editor_draw_footer_background(editor_screenbuffer *screen_buffer,editor_font *font,  
                              editor_rectangle rect)
{
    u32 line_x = rect.x;
    u32 line_y = rect.dy - font->size - 2;
    
    u32 line_dy = rect.dy;
    u32 line_dx = rect.dx - rect.x;
    
    // TODO(willian): pull this off
    u32 color = 0x33cccccc;
    
    u32 pitch = screen_buffer->width * screen_buffer->bytes_per_pixel;
    
    u8 sr = (u8)(color >> 16);
    u8 sg = (u8)(color >> 8);
    u8 sb = (u8)(color);
    float alpha = (float)(color >> 24) / 255.0f;
    
    if(screen_buffer->memory) 
    {
        u8 *row = (u8 *)screen_buffer->memory + ((line_x * screen_buffer->bytes_per_pixel) 
                                                 + (line_y * pitch));
        
        for(u32 y = line_y; y < line_dy; ++y)
        {
            u32 *pixel = (u32 *)row;
            for(u32 x = line_x; x < line_dx; ++x)
            {
                u8 dr = (u8)(*pixel);
                u8 dg = (u8)(*pixel >> 8);
                u8 db = (u8)(*pixel >> 16);
                
                float red = (1.0f - alpha) * dr + alpha * sr;
                float green = (1.0f - alpha) * dg + alpha * sg;
                float blue = (1.0f - alpha) * db + alpha * sb;
                
                *pixel++ = (((u8)blue) << 16) | (((u8)green) << 8) | ((u8)(red));
                
            }
            row += pitch;
        }
    }
}

// TODO(willian): clipping/ out of bound prevention
static void
editor_draw_line_highlight(editor_screenbuffer *screen_buffer, editor_font *font, 
                           v4 color, editor_state *ed, editor_rectangle rect)
{
    u32 line_y = (ed->cursor_y  - ed->text_range_y_start) * font->size + rect.y;
    u32 line_x = rect.x;
    
    u32 line_dy = line_y + font->size;
    u32 line_dx = rect.dx - rect.x;
    
    u32 pitch = screen_buffer->width * screen_buffer->bytes_per_pixel;
    
    u8 sr = (u8)(color.r * 255.0f);
    u8 sg = (u8)(color.g * 255.0f);
    u8 sb = (u8)(color.b * 255.0f);
    float alpha = (u8)(color.a * 255.0f);
    
    u32 *screen_range = (u32 *)screen_buffer->memory + 
        screen_buffer->width * screen_buffer->height;
    
    if(screen_buffer->memory) 
    {
        u8 *row = (u8 *)screen_buffer->memory + ((line_x * screen_buffer->bytes_per_pixel) 
                                                 + (line_y * pitch));
        
        for(u32 y = line_y; y < line_dy; ++y)
        {
            u32 *pixel = (u32 *)row;
            for(u32 x = line_x; x < line_dx; ++x)
            {
                if (pixel <= screen_range)
                {
                    u8 dr = (u8)(*pixel);
                    u8 dg = (u8)(*pixel >> 8);
                    u8 db = (u8)(*pixel >> 16);
                    
                    float red = (1.0f - alpha) * dr + alpha * sr;
                    float green = (1.0f - alpha) * dg + alpha * sg;
                    float blue = (1.0f - alpha) * db + alpha * sb;
                    
                    *pixel++ = (((u8)blue) << 16) | (((u8)green) << 8) | ((u8)(red));
                }
            }
            row += pitch;
        }
    }
}

static void 
editor_mark_draw(editor_screenbuffer *screen_buffer, editor_font *font, editor_state *ed,
                 v4 color, editor_rectangle rect)
{
    // NOTE(willian): check if the mark is in the screen range
    if ((ed->mark_x < ed->text_range_x_start) ||
        (ed->mark_x > ed->text_range_x_end)) return;
    if ((ed->mark_y < ed->text_range_y_start) ||
        (ed->mark_y > ed->text_range_y_end)) return;
    
    u32 mark_height = font->size;
    u32 mark_width = 7;
    
    u32 mark_x = (ed->mark_x * mark_width) + rect.x;
    u32 mark_y = (ed->mark_y * mark_height) + rect.y;
    
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
    
    u32 pitch = screen_buffer->width * screen_buffer->bytes_per_pixel;
    
    u8 red = (u8)(color.r * 255.0f);
    u8 green = (u8)(color.g * 255.0f);
    u8 blue = (u8)(color.b * 255.0f);
    u8 alpha  = (u8)(color.a * 255.0f);
    
    u32 pixel_color = (((u8)blue) << 16) | (((u8)green) << 8) | ((u8)red);
    
    if(screen_buffer->memory) 
    {
        u8 *row = (u8 *)screen_buffer->memory;
        
        for(s32  y = 0; y < screen_buffer->height; ++y)
        {
            u32 *pixel = (u32 *)row;
            for(s32  x = 0; x < screen_buffer->width; ++x)
            {
                *pixel++ = pixel_color;
            }
            row += pitch;
        }
    }
}

static void 
editor_cursor_draw(editor_screenbuffer *screen_buffer, editor_state *ed, editor_font *font, 
                   v4 cursor_color, editor_rectangle rect)
{
    
    u32 cursor_x = (ed->cursor_x - ed->text_range_x_start) * font->width + rect.x;
    u32 cursor_y = (ed->cursor_y  - ed->text_range_y_start) * font->size + rect.y;
    
    u32 dx = cursor_x + font->width;
    u32 dy = cursor_y + font->size;
    
    // // TODO we gonna experiment later with non monospaced fonts, so we gonna change
    // width and advance based on the character width
    
    // TODO(willian): clipping
    
    u32 pitch = screen_buffer->width * screen_buffer->bytes_per_pixel;
    
    u32 *screen_range = (u32 *)screen_buffer->memory + 
        screen_buffer->width * screen_buffer->height;
    
    u8 sr = (u8)(cursor_color.r * 255.0f);
    u8 sg = (u8)(cursor_color.g * 255.0f);
    u8 sb = (u8)(cursor_color.b * 255.0f);
    u8 alpha = (u8)(cursor_color.a * 255.0f);
    
    if(screen_buffer->memory) 
    {
        u8 *row = (u8 *)screen_buffer->memory + ((cursor_x * screen_buffer->bytes_per_pixel) + (cursor_y * pitch));
        
        for(u32 y = cursor_y; y < dy; ++y)
        {
            u32 *pixel = (u32 *)row;
            for(u32 x = cursor_x; x < dx; ++x)
            {
                u8 dr = (u8)(*pixel);
                u8 dg = (u8)(*pixel >> 8);
                u8 db = (u8)(*pixel >> 16);
                
                float red = (1.0f - alpha) * dr + alpha * sr;
                float green = (1.0f - alpha) * dg + alpha * sg;
                float blue = (1.0f - alpha) * db + alpha * sb;
                
                *pixel++ = (((u8)(blue)) << 16) | 
                    (((u8)(green)) << 8) | ((u8)(red));
            }
            row += pitch;
        }
    }
}

/* draw the rect range of the text buffer */
static void 
editor_text_buffer_draw(editor_screenbuffer *screen_buffer,editor_font *font, 
                        editor_text_buffer *text_buffer, editor_rectangle rect,
                        editor_state *ed)
{
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
    
#if 0
    u32 row_count = ed->smooth_range_y_end > text_buffer->last_row ? 
        text_buffer->last_row : ed->smooth_range_y_end;
#endif
    
    u32 row_count = ed->multi_line_smooth ?
        ed->smooth_range_y_end : ed->text_range_y_end;
    
    for (u32  row = y_range_start; row <= row_count; row++)
    {
        u32 text_count = ed->text_range_x_end > text_buffer->rows[row].length ? 
            text_buffer->rows[row].length: ed->text_range_x_end;
        
        for (u32 c = ed->text_range_x_start; c < text_buffer->rows[row].length; c++)
        {
            char char_to_draw = text_buffer->rows[row].data[c];
            
            //s32 smooth_x;
            s32 smooth_y = ed->font_snap_upward ? 
                -ed->font_smooth_snap_y : ed->font_smooth_snap_y;
            
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
}

static void
editor_footer_draw(char *text_name, editor_screenbuffer *screen_buffer, 
                   editor_rectangle rect, editor_font *font, editor_state *ed)
{
    editor_draw_footer_background(screen_buffer, font, rect);
    
    char footer_buffer[1024];
    
    // render line by line
    int x = 1;
    int y = rect.dy - font->size - 1;
    u32 color = 0x1100FFFF;
    
    // TODO(willian): write our own string format funtion
    int cx;
    cx = _snprintf_s(footer_buffer, 1024, 1024, "%s - L#%d C#%d", text_name, ed->cursor_y + 1, ed->cursor_x + 1);
    
    // draw text_buffer name
    for (char *c = footer_buffer; *c; c++)
    {
        editor_draw_glyph(*c, x, y, convert_uhex_to_v4(color), rect, screen_buffer, font);
        x += font->glyph[(*c) - 32].advance;
    }
}

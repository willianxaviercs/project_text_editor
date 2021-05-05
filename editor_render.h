/* EDITOR_RENDE_H */
#ifndef EDITOR_RENDER_H
#define EDITORRENDER_H

static void editor_text_buffer_draw(editor_screenbuffer *screen_buffer, editor_font *font, 
                                    editor_text_buffer *text_buffer, editor_rectangle rect,
                                    editor_state *ed);


static void editor_draw_glyph(u32 c, int x, int y, v4 color, editor_rectangle rect, 
                              editor_screenbuffer *screen_buffer, editor_font *font);


//////// cursor draw
static void editor_cursor_draw(editor_screenbuffer *screen_buffer, editor_state *ed,
                               editor_font *font, v4 cursor_color, editor_rectangle rect);

static void editor_draw_footer_background(editor_screenbuffer *screen_buffer, v4 color, 
                                          editor_font *font,  editor_rectangle rect);

static void editor_draw_line_highlight(editor_screenbuffer *screen_buffer,editor_font *font,
                                       v4 color, editor_state *ed, editor_rectangle rect);


static void editor_mark_draw(editor_screenbuffer *screen_buffer, editor_font *font,
                             editor_state *ed,v4 color, editor_rectangle rect);

static void editor_draw_background(editor_screenbuffer *screen_buffer, v4 color, editor_rectangle rect);

static void
editor_draw_line_highlight(editor_screenbuffer *screen_buffer, editor_font *font, 
                           v4 color, editor_state *ed, editor_rectangle rect);

static void editor_footer_draw(char *text_name, editor_screenbuffer *screen_buffer,
                               editor_rectangle rect, editor_font *font, editor_state *ed);

#endif /* EDITOR_RENDER_H */
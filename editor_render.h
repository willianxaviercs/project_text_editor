#ifndef EDITOR_RENDER_H
#define EDITORRENDER_H

/* draws a rectangle and clips to the parent rectangle */
static void editor_draw_rectangle(u32 x, u32 y, u32 width, u32 height, v4 color,
                                  editor_rectangle clipping_rect,
                                  editor_screenbuffer *screen_buffer);

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

static void editor_footer_draw(editor_text_buffer *text_buffer, editor_screenbuffer *screen_buffer,
                               editor_rectangle rect, editor_font *font, editor_state *ed);

static void editor_search_prompt_draw(editor_search_buffer *search_buffer,
                                      v4 bg_color, v4 text_color, editor_rectangle rect, 
                                      editor_screenbuffer *screen_buffer,editor_font *font);

static void editor_lister_menu_draw(char *lister_label, editor_search_buffer *lister_buffer,
                                    v4 bg_color, v4 text_color, editor_rectangle rect, 
                                    editor_screenbuffer *screen_buffer,editor_font *font);

#endif /* EDITOR_RENDER_H */
#ifndef PARSER_H
#define PARSER_H

typedef struct parser_token
{
    char *start;
    zen_tb_size length;
} parser_token;

#define AUTOCOMPLETE_BUFFER_SIZE 1024 * 1824
typedef struct parser_autocomplete_buffer_header
{
    size_t length;
    size_t capacity;
} parser_autocomplete_buffer_header;

typedef char * autocomplete_buffer;

#define AUTO_BUFFER_HEADER(str) ((parser_autocomplete_buffer_header *)str - 1)

enum token_type
{
    UNKNOWN_TOKEN,
    IDENTIFIER_TOKEN,
    KEYWORD_TOKEN,
    STRING_CONSTANT_TOKEN,
    MULTILINE_STRING_CONSTANT_TOKEN,
    NUMBER_CONSTANT_TOKEN,
    SINGLE_COMMENT_TOKEN,
    MULTI_COMMENT_TOKEN,
    PREPROCESSOR_TOKEN,
    DELIMITER_TOKEN,
    LEFT_CURLY_TOKEN,
    RIGHT_CURLY_TOKEN
};

/* TODO, hardcode by now, but we can define the list of keywords based on file extension name */
#define C_KEYWORDS_COUNT 32
char *editor_c_keywords_list[]= {
    "static", "void",     "char",     "short",
    "long",   "auto",     "break",    "case",
    "const",  "continue", "default",  "do",
    "double", "else",     "enum",     "extern",
    "float",  "for",      "goto",     "if",
    "int",    "register", "return",   "signed",
    "sizeof", "struct",   "switch",   "typedef",
    "union",  "unsigned", "volatile", "while"
};

/* autocomplete */
static autocomplete_buffer parser_autocomplete_buffer_create(size_t capacity);

static void parser_autocomplete_buffer_destroy(autocomplete_buffer str);

static inline size_t parser_autocomplete_buffer_capacity(autocomplete_buffer str);

static inline size_t parser_autocomplete_buffer_length(autocomplete_buffer str);

static autocomplete_buffer paser_autocomplete_buffer_append_token(autocomplete_buffer str, parser_token token);

static inline void parser_autocomplete_buffer_set_length(autocomplete_buffer str, size_t length);

/* parser */
static parser_token editor_parser_get_token(zen_tb_string line, token_type last_line_token_type);

static token_type editor_parser_get_token_type(parser_token token, token_type last_line_token);

static void editor_parse_text_buffer(editor_text_buffer *text_buffer, token_list *list_of_tokens);

static token_type  editor_parse_line(zen_tb_string line, token_type last_line_token_type, token_list *list_of_tokens);

/* helpers */
static bool editor_parser_isdelimiter(char c);

static bool inline editor_parser_isalphabetic(char c);

static inline bool editor_parser_isnumeric(char c);

static inline bool editor_parser_isalphanumeric(char c);

static inline bool editor_parser_is_hexadecimal_digit(char c);

static inline bool editor_parser_is_token_delimiter(char c);

static bool editor_string_compare(char *str1, size_t str1_size, char *str2, size_t str2_size);

#ifdef EDITOR_PARSER_IMPLEMENTATION

static parser_token
editor_parser_get_token(zen_tb_string line, token_type last_line_token_type)
{
    parser_token token = {0};
    token.start = line;
    
    char *c = line;
    
    if (last_line_token_type == MULTILINE_STRING_CONSTANT_TOKEN)
    {
        goto MULTILINE_STRING;
    }
    
    if (last_line_token_type == MULTI_COMMENT_TOKEN)
    {
        goto MULTI_COMMENT;
    }
    
    for (; *c; c++)
    {
        char c0 = *c;
        char c1 = *(c + 1);
        
        if (editor_parser_is_token_delimiter(c0) )
        {
            // reached a token delimiter so we return the token
            if (token.length != 0) break;
            
            if (c0 == '\'')
            {
                token.start = c;
                token.length++;
                for (char *cc = c + 1; *cc; cc++)
                {
                    if ((*cc == '\'') && ((*(cc - 1)) != '\\'))
                    {
                        token.length++;
                        break;
                    }
                    else token.length++;
                }
                break;
            }
            
            // headless float or double  constant
            else if ((c0 == '.') && (editor_parser_isnumeric(c1)))
            {
                token.start = c;
                token.length += 2;
                
                for (char *f = c + 2; *f; f++)
                {
                    if (*f == 'f')
                    {
                        token.length++;
                        break;
                    }
                    else if (!editor_parser_isnumeric(*f))
                    {
                        break;
                    }
                    else
                    {
                        token.length++;
                    }
                }
                break;
            }
            
            // multiline comment
            else if ((c0 == '/') && (c1 == '*'))
            {
                token.start = c;
                token.length += 2;
                // multiline comment token in a single line ex /*  */
                for (char *cm = c + 2; *cm; cm++)
                {
                    if ((*cm == '*') && (*(cm + 1) == '/'))
                    {
                        token.length += 2;
                        break;
                    }
                    else token.length++;
                }
                break;
                
                // multi line comment in a span of multiple lines
                // ex:
                /*  aasd
   asdasdasd
 */
                MULTI_COMMENT:
                token.start = c;
                for (char *cm = c; *cm; cm++)
                {
                    if ((*cm == '*') && (*(cm + 1) == '/'))
                    {
                        token.length += 2;
                        break;
                    }
                    else token.length++;
                }
                break;
            }
            
            // single line comment
            else if ((c0 == '/') && (c1 == '/'))
            {
                token.start = c;
                token.length++;
                
                for (char *cm = c + 1; *cm; cm++) token.length++;
                break;
            }
            
            // string constant
            else if (c0 == '\"')
            {
                MULTILINE_STRING:
                token.start = c;
                token.length++;
                for (char *cm = c + 1; *cm; cm++)
                {
                    if ((*cm == '\"') && (*cm != '\\'))
                    {
                        token.length++;
                        break;
                    }
                    else token.length++;
                }
                break;
            }
            
            // single delimiters
            else
            {
                token.start = c;
                token.length = 1;
            }
            break;
        }
        
        /* number constant */
        else if (editor_parser_isnumeric(c0) && (token.length == 0))
        {
            // hexadecimal constant
            if ((c0 == '0') && (c1 == 'x'))
            {
                token.start = c;
                token.length += 2;
                
                for (char *n = c + 2; *n; n++)
                {
                    if ((!editor_parser_isnumeric(*n)) &&
                        (!editor_parser_is_hexadecimal_digit(*n)))
                    {
                        break;
                    }
                    else
                    {
                        token.length++;
                    }
                }
                break;
            }
            
            token.start = c;
            token.length++;
            for (char *n = c + 1; *n; n++)
            {
                if (!editor_parser_isnumeric(*n))
                {
                    // float or double constant
                    if (*n == '.')
                    {
                        token.length++;
                        for (char *f = n + 1; *f; f++)
                        {
                            if ((*f == 'f'))
                            {
                                token.length++;
                                break;
                            }
                            else if (!editor_parser_isnumeric(*f))
                            {
                                break;
                            }
                            else token.length++;
                        }
                    }
                    break;
                }
                else
                {
                    token.length++;
                }
            }
            break;
        }
        
        else
        {
            token.length++;
        }
    }
    
    return token;
}

static token_type 
editor_parser_get_token_type(parser_token token, token_type last_line_token)
{
    for (unsigned int i = 0; i < C_KEYWORDS_COUNT; i++)
    {
        char *keyword = editor_c_keywords_list[i];
        
        if (editor_string_compare(token.start, token.length, keyword, strlen(keyword)))
        {
            return KEYWORD_TOKEN;
        }
    }
    
    if (((token.start[0] == '\"') && (token.start[token.length - 1] == '\\')))
    {
        return MULTILINE_STRING_CONSTANT_TOKEN;
    }
    
    else if ((last_line_token == MULTILINE_STRING_CONSTANT_TOKEN) &&
             (token.start[token.length - 1] == '\\'))
    {
        return MULTILINE_STRING_CONSTANT_TOKEN;
    }
    
    else if ((last_line_token == MULTILINE_STRING_CONSTANT_TOKEN) &&
             (token.start[token.length - 1] == '\"'))
    {
        return STRING_CONSTANT_TOKEN;
    }
    
    else if ((token.start[0] == '\"') && (token.start[token.length - 1] == '\"'))
    {
        return STRING_CONSTANT_TOKEN;
    }
    
    else if ((token.start[0] == '/') && (token.start[1] == '*'))
    {
        if ((token.start[token.length - 1] == '/') &&
            (token.start[token.length - 2] == '*'))
        {
            return SINGLE_COMMENT_TOKEN;
        }
        
        return MULTI_COMMENT_TOKEN;
    }
    
    else if ((last_line_token == MULTI_COMMENT_TOKEN))
    {
        if ((token.length >= 2) &&
            ((token.start[token.length - 1] == '/') && 
             (token.start[token.length - 2] == '*')))
        {
            return SINGLE_COMMENT_TOKEN;
        }
        return MULTI_COMMENT_TOKEN;
    }
    
    else if ((token.start[0] == '/') && (token.start[1] == '/'))
    {
        return SINGLE_COMMENT_TOKEN;
    }
    
    else if (token.start[0] == '\'')
    {
        if ((token.length > 1) && (token.start[token.length - 1] == '\''))
        {
            return STRING_CONSTANT_TOKEN;
        }
    }
    
    else if (editor_parser_isnumeric(token.start[0]))
    {
        return NUMBER_CONSTANT_TOKEN;
    }
    
    else if ((token.start[0] == '.') && (editor_parser_isnumeric(token.start[1])))
    {
        return NUMBER_CONSTANT_TOKEN;
    }
    
    else if (editor_parser_is_token_delimiter(token.start[0]))
    {
        if (token.start[0] == '{')
        {
            return LEFT_CURLY_TOKEN;
        }
        else if (token.start[0] == '}')
        {
            return RIGHT_CURLY_TOKEN;
        }
        
        return DELIMITER_TOKEN;
    }
    
    return IDENTIFIER_TOKEN;
    
}

static void
editor_parser_append_token_to_list(token_list *list_of_tokens, parser_token token)
{
    for (u32 index = 0; index < list_of_tokens->tokens_count; index++)
    {
        if (editor_string_compare(token.start,
                                  token.length,
                                  list_of_tokens->token_position[index],
                                  strlen(list_of_tokens->token_position[index])))
        {
            return;
        }
    }
    // TODO(willian): need reallocation for the token pointers buffer
    // when we exceed maximum capacity
    
    list_of_tokens->token_position[list_of_tokens->tokens_count] = 
        &list_of_tokens->tokens_string[parser_autocomplete_buffer_length(list_of_tokens->tokens_string)];
    
    // mamimum capacity is 1024 by now we can double everytime if needed 
    list_of_tokens->tokens_count++;
    list_of_tokens->tokens_string =  paser_autocomplete_buffer_append_token(list_of_tokens->tokens_string, token);
}

static token_type
editor_parse_line(zen_tb_string line, token_type last_line_token_type, token_list *list_of_tokens)
{
    zen_tb_size line_length = zen_tb_line_length(line);
    
    unsigned char *meta_line = zen_tb_line_get_meta_string(line);
    memset(meta_line, 0, line_length);
    
    token_type type = last_line_token_type;
    
    for (u32 i = 0; i < line_length;)
    {
        parser_token token;
        token = editor_parser_get_token(&line[i], type);
        
        // classify the token
        type = editor_parser_get_token_type(token, type);
        
        // set glyphs color type in the meta string
        memset(&meta_line[i], type, token.length);
        
        //
        if (type == IDENTIFIER_TOKEN && (!zen_tb_line_been_parsed(line)))
        {
            editor_parser_append_token_to_list(list_of_tokens, token);
        }
        
        i += token.length;
    }
    
    zen_tb_line_set_parse_status(line, true);
    
    return type;
}

static void editor_parse_text_buffer(editor_text_buffer *text_buffer, token_list *list_of_tokens)
{
    token_type last_token_type = UNKNOWN_TOKEN;
    for (zen_tb_string *row = &text_buffer->rows[0];
         row < (&text_buffer->rows[text_buffer->length]);
         row++)
    {
        last_token_type = editor_parse_line(*row, last_token_type, list_of_tokens);
    }
}

static autocomplete_buffer
parser_autocomplete_buffer_create(size_t capacity)
{
    parser_autocomplete_buffer_header *header;
    
    autocomplete_buffer str;
    
    header = (parser_autocomplete_buffer_header *)calloc(capacity + 1, sizeof(*header));
    
    if (!header) return NULL;
    
    header->length = 0;
    header->capacity = capacity;
    
    str = (autocomplete_buffer)header + sizeof(*header);
    
    return str;
}

static void
parser_autocomplete_buffer_destroy(autocomplete_buffer str)
{
    if (str == ZEN_TB_NULLPTR) return;
    
    free(AUTO_BUFFER_HEADER(str));
}

static inline size_t
parser_autocomplete_buffer_capacity(autocomplete_buffer str)
{
    return (AUTO_BUFFER_HEADER(str)->capacity);
}

static inline size_t
parser_autocomplete_buffer_length(autocomplete_buffer str)
{
    return (AUTO_BUFFER_HEADER(str)->length);
}

static inline void
parser_autocomplete_buffer_set_length(autocomplete_buffer str, size_t length)
{
    (AUTO_BUFFER_HEADER(str)->length = length);
}

static autocomplete_buffer
paser_autocomplete_buffer_append_token(autocomplete_buffer str, parser_token token)
{
    char *blank_space = "\0";
    
    autocomplete_buffer new_str;
    
    size_t append_length = token.length + 1;
    
    size_t str_length = parser_autocomplete_buffer_length(str);
    
    size_t space_left = parser_autocomplete_buffer_capacity(str) - str_length;
    
    if (space_left >= append_length)
    {
        memcpy(str + str_length, token.start, token.length);
        
        parser_autocomplete_buffer_set_length(str, token.length + str_length);
        
        memcpy(str + parser_autocomplete_buffer_length(str), blank_space, 1);
        
        parser_autocomplete_buffer_set_length(str, parser_autocomplete_buffer_length(str) + 1);
        
        return str;
    }
    
    new_str = parser_autocomplete_buffer_create(str_length + append_length);
    
    // just return without appending
    if (new_str == NULL) return str;
    
    memcpy(new_str, str, str_length);
    
    memcpy(str + str_length, token.start, token.length);
    
    parser_autocomplete_buffer_set_length(str, token.length + str_length);
    
    memcpy(str + parser_autocomplete_buffer_length(str), blank_space, 1);
    
    parser_autocomplete_buffer_set_length(str, parser_autocomplete_buffer_length(str) + 1);
    
    parser_autocomplete_buffer_destroy(str);
    
    str = new_str;
    
    return str;
}

static bool
editor_string_compare(char *str1, size_t str1_size, char *str2, size_t str2_size)
{
    if (str1_size != str2_size) return false;
    
    for (size_t i = 0; i < str1_size; i++)
    {
        if (str1[i] != str2[i]) return false;
    }
    
    return true;
}

static bool inline
editor_parser_isalphabetic(char c)
{
    return (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')));
}

static inline bool
editor_parser_isnumeric(char c)
{
    return ((c >= '0') && (c <= '9'));
}

static inline bool
editor_parser_isalphanumeric(char c)
{
    return (editor_parser_isnumeric(c) || editor_parser_isalphabetic(c));
}

static inline bool
editor_parser_is_token_delimiter(char c)
{
    // NOTE(willian): '_' dont delimit a token
    return !editor_parser_isalphanumeric(c) && (c != '_');
}

static inline bool
editor_parser_is_hexadecimal_digit(char c)
{
    return ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F'));
}

static bool
editor_parser_isdelimiter(char c)
{
    return !editor_parser_isalphanumeric(c);
}

#endif /* EDITOR_PARSER_IMPLEMENTATION */

#endif /* PARSER_H */
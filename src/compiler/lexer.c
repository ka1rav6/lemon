#include "lexer.h"
#include <limits.h>
#include <stdbool.h>
#include "../utils/arena.h"
#include "../utils/dynamic_array.h"

Lexer* l;

static char* read_file(Arena* ar, const char* filename, size_t* size)
{
    FILE* f = fopen(filename, "rb");
    if (f == NULL)
        return NULL;

    char* buf = NULL;
    if (fseek(f, 0, SEEK_END) == 0){
        long len = ftell(f);
        rewind(f);
        if (len >= 0){
            buf = (char*) arena_alloc(ar, (size_t) len + 1);
            if (buf != NULL){
                *size      = fread(buf, 1, (size_t) len, f);
                buf[*size] = '\0';
            }
        }
    }
    fclose(f);
    return buf;
}

bool lexer_init(Arena* ar, const char* file)
{
    l = NULL;
    if (file == NULL)
        return false;

    size_t size = 0;
    char* data  = read_file(ar, file, &size);
    if (data == NULL)
        return false;

    Lexer* lx      = (Lexer*) arena_alloc(ar, sizeof(Lexer));
    TokenArray* ta = (TokenArray*) arena_alloc(ar, sizeof(TokenArray));
    if (lx == NULL || ta == NULL)
        return false;

    *ta           = (TokenArray){0};
    lx->tokens    = ta;
    lx->fname     = file;
    lx->data      = data;
    lx->file_size = size;
    lx->index     = 0;
    l             = lx;
    return true;
}

#define TOK_PUSH(_type, val)                          \
                {                                     \
                    TokMetadata data = {              \
                                .character = i,       \
                                .line = line_num,     \
                                .file = l->fname      \
                    };                                \
                    Token t = {                       \
                                .type = _type,        \
                                .value = val,         \
                                .data = data          \
                    };                                \
                    DA_PUSH(*l->tokens, t, NULL);     \
                }

static inline bool isnum(char ch)
{
    return (ch >= '0' && ch <='9');
}

void tokenize(const char* line, size_t line_num)
{
    size_t len = strlen(line);
    for (size_t i = 0; i < len; i++){
        char ch = line[i];
        switch (ch)
        {
            case '(':
                TOK_PUSH(TOKEN_LPAREN, INT_MIN);
                break;
            case ')':
                TOK_PUSH(TOKEN_RPAREN, INT_MIN);
                break;
            case '+':
                TOK_PUSH(TOKEN_PLUS  , INT_MIN);
                break;
            case '-':
                TOK_PUSH(TOKEN_MINUS , INT_MIN);
                break;
            case '*':
                TOK_PUSH(TOKEN_STAR  , INT_MIN);
                break;
            case '/':
                TOK_PUSH(TOKEN_SLASH , INT_MIN);
                break;
            case ' ':
            case '\t':
            case '\n':
                break;
            default: {
                if (!isnum(ch)){
                    LOG("%s:%zu:%zu: skipping unexpected character '%c'",
                        l->fname, line_num, i, ch);
                    break;
                }
                int value = 0;
                size_t end = i;
                while (end < len && isnum(line[end]))
                    value = value * 10 + (line[end++] - '0');
                TOK_PUSH(TOKEN_NUMBER, value);
                i = end - 1;  // the loop's i++ then lands past the last digit
            }
        }
    }
}

TokenArray* tokenize_file(void)
{
    size_t line_num = 1;
    tokenize(l->data, line_num);
    size_t i = l->file_size;
    TOK_PUSH(TOKEN_EOF, INT_MIN);
    return l->tokens;
}

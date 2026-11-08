#include "lexer.h"
#include <stdbool.h>
#include "../utils/arena.h"
#include "../utils/dynamic_array.h"

// the standard error message when the arena allocated is too small
static const char* const ERR_NO_SPACE = "source does not fit in the compiler arena";

// reads file and returns the buffer
static char* read_file(Arena* ar, const char* filename, size_t* size, const char** why)
{
    FILE* f = fopen(filename, "rb");
    if (f == NULL){
        *why = "cannot open file";
        return NULL;
    }
    long len = -1;
    if (fseek(f, 0, SEEK_END) == 0)
        len = ftell(f);
    rewind(f);
    char* buf = NULL;
    if (len < 0)
        *why = "cannot measure file";
    else if ((buf = (char*) arena_alloc(ar, (size_t) len + 1)) == NULL)
        // the only way arena_alloc fails is a full block
        *why = ERR_NO_SPACE;
    else {
        *size      = fread(buf, 1, (size_t) len, f);
        buf[*size] = '\0';
    }
    fclose(f);
    return buf;
}

// initializes the lexer. Returns NULL on an array, 
// with a reason inside `why`
Lexer* lexer_init(Arena* ar, const char* file, const char** why)
{
    const char* unread = NULL;
    if (why == NULL)
        why = &unread;  // so every failure path can write one without checking

    if (file == NULL){
        *why = "no file given";
        return NULL;
    }

    size_t size = 0;
    char* data  = read_file(ar, file, &size, why);
    if (data == NULL)
        return NULL;

    Lexer* l       = (Lexer*) arena_alloc(ar, sizeof(Lexer));
    TokenArray* ta = (TokenArray*) arena_alloc(ar, sizeof(TokenArray));
    if (l == NULL || ta == NULL){
        *why = ERR_NO_SPACE;
        return NULL;
    }

    *ta = (TokenArray){0};
    *l  = (Lexer){
        .tokens     = ta,
        .fname      = file,
        .data       = data,
        .file_size  = size,
        .index      = 0,
        .line       = 1,
        .line_start = 0
    };
    return l;
}

// we get the exact column (starting from 1) of an index, by subtracting the line_start
static inline u64 column_at(const Lexer* l, size_t index)
{
    return (u64)(index - l->line_start) + 1;
}

// Creates and pushes the token into the token array owned by the lexer
// used to be a macro
static void push_token(Lexer* l, TokenType type, u64 number, size_t start)
{
    Token t = {
        .type   = type,
        .number = number,
        .loc    = {
            .file   = l->fname,
            .line   = l->line,
            .column = column_at(l, start)
        }
    };
    DA_PUSH(*l->tokens, t, NULL);
}

static inline bool isnum(char ch)
{
    return (ch >= '0' && ch <='9');
}

static size_t lex_number(Lexer* l, size_t start)
{
    u64 value     = 0;
    bool overflow = false;
    size_t end    = start;

    while (end < l->file_size && isnum(l->data[end])){
        u64 d = (u64)(l->data[end++] - '0');
        if (value > (UINT64_MAX - d) / 10)
            overflow = true;  
        else
            value = value * 10 + d;
    }

    if (overflow){
        LOG("%s:%" PRIu64 ":%" PRIu64 ": integer literal does not fit in 64 bits",
            l->fname, l->line, column_at(l, start));
        value = 0;
    }
    push_token(l, TOKEN_NUMBER, value, start);
    return end;
}

// the public function to tokenize the entire file
// returns TokenArray (the one owned by the lexer created)
TokenArray* tokenize_file(Lexer* l)
{
    for (; l->index < l->file_size; l->index++){
        size_t i = l->index;
        char ch  = l->data[i];
        switch (ch)
        {
            case '(':
                push_token(l, TOKEN_LPAREN, 0, i);
                break;
            case ')':
                push_token(l, TOKEN_RPAREN, 0, i);
                break;
            case '+':
                push_token(l, TOKEN_PLUS  , 0, i);
                break;
            case '-':
                push_token(l, TOKEN_MINUS , 0, i);
                break;
            case '*':
                push_token(l, TOKEN_STAR  , 0, i);
                break;
            case '/':
                push_token(l, TOKEN_SLASH , 0, i);
                break;
            case '\n':
                l->line++;
                l->line_start = i + 1;
                break;
            case ' ':
            case '\t':
            case '\r':
                break;
            default:
                if (!isnum(ch)){
                    LOG("%s:%" PRIu64 ":%" PRIu64 ": skipping unexpected character '%c'",
                        l->fname, l->line, column_at(l, i), ch);
                    break;
                }
                l->index = lex_number(l, i) - 1;
        }
    }
    push_token(l, TOKEN_EOF, 0, l->index);
    return l->tokens;
}

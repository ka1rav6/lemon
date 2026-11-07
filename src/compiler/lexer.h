#ifndef LEMON_LEXER_H
#define LEMON_LEXER_H

#include <stdbool.h>

#include "../utils/common.h"
#include "../utils/arena.h"

// the one list of token names: the enum below and ENUM_NAMES both read it
#define TOKEN_TYPE_LIST(X) \
    X(TOKEN_NUMBER)        \
    X(TOKEN_PLUS  )        \
    X(TOKEN_MINUS )        \
    X(TOKEN_STAR  )        \
    X(TOKEN_SLASH )        \
    X(TOKEN_LPAREN)        \
    X(TOKEN_RPAREN)        \
    X(TOKEN_EOF   )

#define X(name) name,
typedef enum _TokenType{
    TOKEN_TYPE_LIST(X)
} TokenType;
#undef X

typedef struct _TokMetadata{
    u64 line     ;
    u64 character;
    const char* file;
}TokMetadata;


typedef struct _Token{
    TokenType type  ;
    int value       ;
    TokMetadata data;
} Token;


typedef struct _TokenArray{
    Token* items   ;
    size_t count   ;
    size_t capacity;
}TokenArray;

typedef struct _Lexer{
    TokenArray* tokens;
    const char* fname ;
    const char* data  ;
    size_t file_size  ;
    size_t index      ;
}Lexer;

bool lexer_init(Arena* ar, const char* file);
void tokenize(const char* line, size_t line_num);
TokenArray* tokenize_file(void);

#endif // LEMON_LEXER_ H

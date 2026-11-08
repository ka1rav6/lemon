#ifndef LEMON_LEXER_H
#define LEMON_LEXER_H

#include "../utils/common.h"
#include "../utils/arena.h"

/* --------------- Some Macros --------------- */

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

/* -------------------- Token related declarations ------------------ */

typedef enum _TokenType{
    TOKEN_TYPE_LIST(X)
} TokenType;
#undef X

typedef struct _Token{
    TokenType type  ;
    u64       number;
    SrcLoc    loc   ;
} Token;


typedef struct _TokenArray{
    Token* items   ;
    size_t count   ;
    size_t capacity;
}TokenArray;

/* -------------------- Lexer Declaration ------------------ */

typedef struct _Lexer{
    TokenArray* tokens;
    const char* fname ;
    const char* data  ;
    size_t file_size  ;
    size_t index      ;
    u64    line       ;
    size_t line_start ;
}Lexer;

Lexer*      lexer_init(Arena* ar, const char* file, const char** why);
TokenArray* tokenize_file(Lexer* l);

#endif // LEMON_LEXER_H

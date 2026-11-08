#ifndef LEMON_STRBUF_H
#define LEMON_STRBUF_H

#include "common.h"
#include "dynamic_array.h"
#include "arena.h"

#include <stdbool.h>

typedef struct _Strbuf{
    char*  items   ;
    size_t count   ;
    size_t capacity;
    Arena* arena   ;  // NULL = heap
}Strbuf;

/* ------------   Constructors and Destructors     ------------- */
Strbuf* strbuf_create(Arena*); // if None => allocate on the heap
void strbuf_destroy(Strbuf*) ;

/* -------------   Different functions to Append   ------------- */
bool appendc(Strbuf*, char);
bool append (Strbuf*, const char*);

// printf into the buffer. The attribute is what makes the compiler check the
// format string against the arguments at every call site
#if defined(__GNUC__) // as format attribute exists only if __GNUC__ is defined
bool appendf(Strbuf*, const char*, ...) __attribute__((format(printf, 2, 3)));
#else
bool appendf(Strbuf*, const char*, ...);
#endif

/* -------------   Reading the buffer back out  ------------- */
const char* cstr(Strbuf*);

/* ------------- Helper Macros ------------- */

#define STR_APPEND(str, data)                                                  \
        _Generic((data),                                                       \
                 char        : appendc,                                        \
                 int         : appendc,                                        \
                 char*       : append ,                                        \
                 const char* : append                                          \
                 )((str), (data))

#endif // LEMON_STRBUF_H

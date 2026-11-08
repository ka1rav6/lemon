#include "strbuf.h"

#include <stdarg.h>

/* ------------   Constructors and Destructors     ------------- */

Strbuf* strbuf_create(Arena* ar)
{
    Strbuf* temp = (ar == NULL) ? (Strbuf*) malloc(sizeof(Strbuf))
                                : (Strbuf*) arena_alloc(ar, sizeof(Strbuf));
    if (temp == NULL)
        return NULL;

    temp->items    = NULL;  // the first push allocates
    temp->count    = 0;
    temp->capacity = 0;
    temp->arena    = ar;
    return temp;
}

void strbuf_destroy(Strbuf* str)
{
    if (str == NULL || str->arena != NULL)
        return;  // an arena buffer goes when its arena does
    free(str->items);
    free(str);
}


static void reserve(Strbuf* str, size_t n)
{
    size_t need = str->count + n + 1;
    if (need <= str->capacity)
        return;
    size_t cap = str->capacity;
    while (cap < need)
        cap = GROW_CAPACITY(cap);
    str->items    = reallocate(str->items, sizeof(char) * str->capacity,
                               sizeof(char) * cap, str->arena);
    str->capacity = cap;
}


/* -------------   Different functions to Append   ------------- */

bool appendc(Strbuf* str, char c)
{
    if (str == NULL){
        LOG("TRIED TO APPEND TO A NULL STRING");
        return false;
    }
    DA_PUSH(*str, c, str->arena);
    return true;
}

bool append (Strbuf* str, const char* str2)
{
    if (str == NULL || str2 == NULL){
        LOG("TRIED TO APPEND TO A NULL STRING");
        return false;
    }
    size_t len = strlen(str2);
    reserve(str, len);
    memcpy(str->items + str->count, str2, len);
    str->count += len;
    return true;
}

bool appendf(Strbuf* str, const char* fmt, ...)
{
    if (str == NULL || fmt == NULL){
        LOG("TRIED TO APPEND TO A NULL STRING");
        return false;
    }

    va_list ap, measure;
    va_start(ap, fmt);
    va_copy(measure, ap);  
    int len = vsnprintf(NULL, 0, fmt, measure);
    va_end(measure);
    if (len < 0){  // the format string itself is broken
        va_end(ap);
        LOG("vsnprintf failed");
        return false;
    }

    reserve(str, (size_t)len);
    // + 1 because vsnprintf always terminates; the NUL lands past `count`
    vsnprintf(str->items + str->count, (size_t)len + 1, fmt, ap);
    va_end(ap);
    str->count += (size_t)len;
    return true;
}

/* -------------   Reading the buffer back out      ------------- */
const char* cstr(Strbuf* str)
{
    if (str == NULL)
        return NULL;
    DA_PUSH(*str, '\0', str->arena);  
    str->count--; 
    return str->items;
}

/**
 * Copyright 2026, Airton Ishimori
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the “Software”), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

typedef enum {
    MJTOK_IDENTIFIER,               /* aA-zZ */
    MJTOK_SEP_BRACE_OPEN,           /* { */
    MJTOK_SEP_BRACE_CLOSE,          /* } */
    MJTOK_SEP_BRACKET_OPEN,         /* [ */
    MJTOK_SEP_BRACKET_CLOSE,        /* ] */
    MJTOK_SEP_COLON,                /* : */
    MJTOK_SEP_COMMA,                /* , */
    MJTOK_LIT_STRING,               /* aA-zZ*/
    MJTOK_LIT_INTEGER,              /* 0-9 */
    MJTOK_LIT_FRACTION,             /* 0.0-9.0*/
    MJTOK_LIT_EXPONENT,             /* ex: 1.5e4, 6.022e-23 */
    //TODO:
} mjtok_type_t;

typedef struct {
    mjtok_type_t    type;
    const void      *value;
    const char      *start;
    size_t          len;
} mjtok_t;

typedef struct {
    mjtok_t *tokens;
    size_t  count;
    size_t  cap;
} mjarr_t;

#define MJTOK_ARRAY_INIT_CAP        100
#define MJTOK_ARRAY_GROWTH_FACTOR   2

static void init_tok_arr(mjarr_t *arr) 
{
    assert(arr != NULL && "Array cannot be null");
    arr->count = 0;
    arr->cap = MJTOK_ARRAY_INIT_CAP;
    arr->tokens = malloc(arr->cap * sizeof(*arr->tokens));
}

static void append_tok(mjarr_t *arr, mjtok_t token)
{
    if (arr->count >= arr->cap) {
        size_t new_cap = arr->cap * MJTOK_ARRAY_GROWTH_FACTOR;
        mjtok_t *tmp = realloc(arr->tokens, new_cap * sizeof(*arr->tokens));
        if (tmp == NULL) {
            fprintf(stderr, "Out of memory failure");
            free(arr->tokens);
            exit(1);
        }
    }

    arr->tokens[arr->count++] = token;
}

void test() {
    mjarr_t array;
    init_tok_arr(&array);
    mjtok_t token = {
        .type = MJTOK_IDENTIFIER,
        .value = "a",
        .start = 0,
        .len = 1
    };
    append_tok(&array, token);
}

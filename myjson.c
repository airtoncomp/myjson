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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>

#define MJ_RET_ON_NULL(x, fmt, ...) do {                    \
            if (x == NULL) {                                \
                fprintf(stderr, fmt"\n", ##__VA_ARGS__);    \
                return -1;                                  \
            }                                               \
        } while (0)

#define MJ_RET_ON_ERR(x, fmt, ...) do {                     \
            if (x < 0) {                                    \
                fprintf(stderr, fmt"\n", ##__VA_ARGS__);    \
                return -1;                                  \
            }                                               \
        } while (0)

#define MJ_RET_ON_ERR2(x) do {                              \
            if (x < 0) {                                    \
                return -1;                                  \
            }                                               \
        } while (0)

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

#define is_brace_open(x)            (x == '{')
#define is_brace_close(x)           (x == '}')
#define is_bracket_open(x)          (x == '[')
#define is_bracket_close(x)         (x == ']')
#define is_colon(x)                 (x == ':')

static void init_tok_arr(mjarr_t *arr) 
{
    assert(arr != NULL && "Array cannot be null");

    arr->count = 0;
    arr->cap = MJTOK_ARRAY_INIT_CAP;
    arr->tokens = malloc(arr->cap * sizeof(*arr->tokens));
}

static int append_tok(mjarr_t *arr, mjtok_t token)
{
    assert(arr != NULL && "Array cannot be null");

    if (arr->count >= arr->cap) {
        size_t new_cap = arr->cap * MJTOK_ARRAY_GROWTH_FACTOR;
        mjtok_t *tmp = realloc(arr->tokens, new_cap * sizeof(*arr->tokens));
        MJ_RET_ON_NULL(tmp, "Out of memory failure");
        arr->tokens = tmp;
        arr->cap = new_cap;
    }

    arr->tokens[arr->count++] = token;

    return 0;
}

static inline mjtok_t new_tok(mjtok_type_t t, const void *val, const char *pos, size_t val_len)
{
    mjtok_t token = {
            .type = t,
            .value = val,
            .start = pos, 
            .len = val_len
        };

    return token;
}

static int tokenize_json (mjarr_t *arr, char *const json)
{
    char *cptr = json; 

    while (cptr && *cptr != '\0') {
        printf("char: %c\n", *cptr);
        if (isspace((unsigned char) *cptr) || *cptr == '\t') {
            goto advance;
        }
        if (is_brace_open(*cptr)) {
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_SEP_BRACE_OPEN, cptr, cptr, 1)));
            goto advance;
        }
        if (is_brace_close(*cptr)) {
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_SEP_BRACE_CLOSE, cptr, cptr, 1)));
            goto advance;
        }
        if (is_colon(*cptr)) {
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_SEP_COLON, cptr, cptr, 1)));
            goto advance;
        }
        if (is_bracket_open(*cptr)) {
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_SEP_BRACE_OPEN, cptr, cptr, 1)));
            goto advance;
        }
        if (is_bracket_close(*cptr)) {
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_SEP_BRACE_CLOSE, cptr, cptr, 1)));
            goto advance;
        }
        //TODO
advance:
        cptr++;
    }

    return 0;
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

    mjtok_t token2 = {
        .type = MJTOK_IDENTIFIER,
        .value = "b",
        .start = 0,
        .len = 1
    };
    append_tok(&array, token2);

    mjarr_t arr2; 
    init_tok_arr(&arr2);
    char buf[] = "{\"a\":\"b\", \"c\":[]}";
    char *str = buf;
    tokenize_json(&arr2, str);

    for (int i = 0; i < arr2.count; i++)
        printf("token: type=%d, value=%c, start=%p, len=%zu\n",
                arr2.tokens[i].type, *(char *) arr2.tokens[i].value, arr2.tokens[i].start, arr2.tokens[i].len);
}

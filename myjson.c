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

#define MJ_RET_ON_TRUE(x, fmt, ...) do {                   \
            if (x) {                                        \
                fprintf(stderr, fmt"\n", ##__VA_ARGS__);    \
                return -1;                                  \
            }                                               \
        } while (0)

typedef enum {
    MJTOK_IDENTIFIER,               /* aA-zZ */
    MJTOK_BRACE_OPEN,               /* { */
    MJTOK_BRACE_CLOSE,              /* } */
    MJTOK_BRACKET_OPEN,             /* [ */
    MJTOK_BRACKET_CLOSE,            /* ] */
    MJTOK_COLON,                    /* : */
    MJTOK_COMMA,                    /* , */
    MJTOK_STRING,                   /* aA-zZ*/
    MJTOK_NUMBER,                   /* 0-9, 0.0-9.9*/
    MJTOK_INTEGER,                  /* 0-9 */
    MJTOK_FRACTION,                 /* 0.0-9.0*/
    MJTOK_EXPONENT,                 /* ex: 1.5e4, 6.022e-23 */
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
#define is_comma(x)                 (x == ',')
#define is_quote_double(x)          (x == '"')
#define is_minus(x)                 (x == '-')
#define is_plus(x)                  (x == '+')
#define is_dot(x)                   (x == '.')
#define is_exponent(x)              (x == 'e' || x == 'E')

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

static int scan_str(mjarr_t *arr, char **cptr, mjtok_t *out)
{
    assert(arr && *cptr && out && "Cannot be null");

    char *val = *cptr;
    char *start = *cptr;
    size_t val_len = 0;
    int quote_count = 0;

    /* Caller should call scan_str() if first quote
       is found, but here we make sure that this is true.
       If not, return an error. 
       */
    MJ_RET_ON_TRUE(!is_quote_double(**cptr), "Invalid string");

    for ( ; *cptr && **cptr != '\0' && quote_count < 2; (*cptr)++, val_len++) {
        if (is_quote_double(**cptr)) {
            quote_count++;
            continue;
        }
        MJ_RET_ON_TRUE(!isalpha((unsigned char) **cptr), "Invalid string");
    } 

    out->type = MJTOK_STRING;
    out->value = val;
    out->start = start;
    out->len = ++val_len;
    
    char buf[256] = {0};
    snprintf(buf, val_len, "%s", val);
    buf[val_len] = '\0';
    printf("string: %s\n", buf);

    return 0;
}

static int scan_num(mjarr_t *arr, char **cptr, mjtok_t *out)
{
    assert(arr && *cptr && out && "Cannot be null");

    char *val = *cptr;
    char *start = *cptr;
    size_t val_len = 0;

    /* Caller should have checked that first character might be 
       valid digit ranging from 0-9 or starting with '-' symbol.
       This is just a check for safety. Return -1 if not true.
       */
    MJ_RET_ON_TRUE(!(isdigit(**cptr) || is_minus(**cptr)), "Invalid number");

    if (is_minus(**cptr)) {
        (*cptr)++; val_len++;
        MJ_RET_ON_TRUE(*cptr && !isdigit(**cptr), "Invalid number");
    }

    if (isdigit(**cptr)) {
        for ( ;*cptr && **cptr != '\0'; (*cptr)++, val_len++) {
            if (!isdigit(**cptr)) break;
        }
        if (is_dot(**cptr)) {
            (*cptr)++; val_len++;
            MJ_RET_ON_TRUE(is_exponent(**cptr), "Invalid digit after dot");
            for ( ;*cptr && **cptr != '\0'; (*cptr)++, val_len++) {
                if (!isdigit(**cptr)) break;
            }
            if (is_exponent(**cptr)) {
                (*cptr)++; val_len++;
                if (is_minus(**cptr) || is_plus(**cptr)) {
                    (*cptr)++; val_len++;
                }
                for ( ;*cptr && **cptr != '\0'; (*cptr)++, val_len++) {
                    if (!isdigit(**cptr)) break;
                }
            }
        }
        if (is_exponent(**cptr)) {
            (*cptr)++; val_len++;
            if (is_minus(**cptr) || is_plus(**cptr)) {
                (*cptr)++; val_len++;
            }
            for ( ;*cptr && **cptr != '\0'; (*cptr)++, val_len++) {
                if (!isdigit(**cptr)) break;
            }
        }
    }

    out->type = MJTOK_NUMBER;
    out->value = val;
    out->start = start;
    out->len = ++val_len;

    char buf[256] = {0};
    snprintf(buf, val_len, "%s", val);
    buf[val_len] = '\0';
    printf("string: %s\n", buf);

    return 0;
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
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_BRACE_OPEN, cptr, cptr, 1)));
            goto advance;
        }
        if (is_brace_close(*cptr)) {
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_BRACE_CLOSE, cptr, cptr, 1)));
            goto advance;
        }
        if (is_colon(*cptr)) {
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_COLON, cptr, cptr, 1)));
            goto advance;
        }
        if (is_bracket_open(*cptr)) {
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_BRACE_OPEN, cptr, cptr, 1)));
            goto advance;
        }
        if (is_bracket_close(*cptr)) {
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_BRACE_CLOSE, cptr, cptr, 1)));
            goto advance;
        }
        if (is_comma(*cptr)) {
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_COMMA, cptr, cptr, 1)));
            goto advance;
        }
        if (is_quote_double(*cptr)) {
            mjtok_t tok;
            MJ_RET_ON_ERR2(scan_str(arr, &cptr, &tok));
            MJ_RET_ON_ERR2(append_tok(arr, tok));
            goto advance;
        }
        if (isdigit(*cptr) || is_minus(*cptr)) {
            mjtok_t tok;
            MJ_RET_ON_ERR2(scan_num(arr, &cptr, &tok));
            MJ_RET_ON_ERR2(append_tok(arr, tok));
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
    //char *str = R"({"a":"b", "c":[], "d":1, "e":2.4, "f":0.0e10})";
    //char *str = "{\"a\":\"b\", \"c\":[]}";
    char *str = R"({"a":0.0e12, "b":0.1E-56, "c":2.5e+4, "d":6e7})";
    printf("%s\n", str);
    tokenize_json(&arr2, str);

    for (size_t i = 0; i < arr2.count; i++)
        printf("token: type=%d, value=%c, start=%p, len=%zu\n",
                arr2.tokens[i].type, *(char *) arr2.tokens[i].value, arr2.tokens[i].start, arr2.tokens[i].len);
}

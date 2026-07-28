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

#include "myjson.h"

#ifdef DEBUG

#define MJ_LOG(...) do {                                                \
            fprintf(stdout, "[DEBUG] %s:%d: ", __FILE__, __LINE__);     \
            fprintf(stdout, __VA_ARGS__);                               \
        } while(0)

#define MJ_LOGE(...) do {                                               \
            fprintf(stderr, "[DEBUG] %s:%d: ", __FILE__, __LINE__);     \
            fprintf(stderr, __VA_ARGS__);                               \
        } while(0)

#define MJ_LOG_STR(fmt, start, len, ...) do {                           \
            char buf[1024] = {0};                                       \
            snprintf(buf, len, "%s", start);                            \
            buf[val_len] = '\0';                                        \
            MJ_LOG(fmt, buf, ##__VA_ARGS__);                            \
        } while(0)

#else

#define MJ_LOG(stdout, ...)                 ((void)0)
#define MJ_LOGE(stderr, ...)                ((void)0)
#define MJ_LOG_STR(fmt, start, len, ...)    ((void)0)

#endif

#define MJ_RET_ERR_ON_NULL(x, fmt, ...) do {                \
            if (x == NULL) {                                \
                fprintf(stderr, fmt"\n", ##__VA_ARGS__);    \
                return -1;                                  \
            }                                               \
        } while(0)

#define MJ_RET_ERR_ON_TRUE(x, fmt, ...) do {                \
            if (x) {                                        \
                fprintf(stderr, fmt"\n", ##__VA_ARGS__);    \
                return -1;                                  \
            }                                               \
        } while(0)

#define MJ_RET_ON_ERR(x, fmt, ...) do {                     \
            if (x < 0) {                                    \
                fprintf(stderr, fmt"\n", ##__VA_ARGS__);    \
                return -1;                                  \
            }                                               \
        } while(0)

#define MJ_RET_ON_ERR2(x) do {                              \
            if (x < 0) {                                    \
                return -1;                                  \
            }                                               \
        } while(0)


/**
 * MyJSON Lexer
 *
 * Reads a JSON string and identifies its tokens. Tokens are stored on the heap
 * as a flat dynamic array.
 *
 * The MyJSON lexer does not copy C strings. Each token stores a pointer to the
 * beginning of the corresponding string value, along with its size in bytes.
 */

typedef enum {
    MJTOK_BRACE_OPEN,               /* { */
    MJTOK_BRACE_CLOSE,              /* } */
    MJTOK_BRACKET_OPEN,             /* [ */
    MJTOK_BRACKET_CLOSE,            /* ] */
    MJTOK_COLON,                    /* : */
    MJTOK_COMMA,                    /* , */
    MJTOK_STRING,                   /* aA-zZ*/
    MJTOK_NUMBER,                   /* ex: 0-9, 0.0-9.9, 0e50 */
    MJTOK_BOOL,                     /* true or false */
    MJTOK_NULL                      /* null */
} mjtok_type_t;

typedef struct {
    const void      *value;
    size_t          len;
    mjtok_type_t    type;
} mjtok_t;

typedef struct {
    mjtok_t *tokens;
    size_t  count;
    size_t  cap;
} mjarr_t;

#define MJTOK_ARRAY_INIT_CAP        1000
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
#define is_bslash(x)                (x == '\\')
#define is_fslash(x)                (x == '/')
#define is_backspc_b(x)             (x == 'b')
#define is_formfeed_f(x)            (x == 'f')
#define is_linefeed_n(x)            (x == 'n')
#define is_carret_r(x)              (x == 'r')
#define is_tab_t(x)                 (x == 't')
#define is_hex_u(x)                 (x == 'u')
#define is_exponent(x)              (x == 'e' || x == 'E')

static const char _bool_true[4] = "true";
static const char _bool_false[5] = "false";
static const char _value_null[4] = "null";

#define CHAR_OF_TRUE(x)     _bool_true[x]
#define CHAR_OF_FALSE(x)    _bool_false[x]
#define CHAR_OF_NULL(x)     _value_null[x]

static void init_tok_arr(mjarr_t *arr) 
{
    assert(arr != NULL && "Array cannot be null");

    arr->count = 0;
    arr->cap = MJTOK_ARRAY_INIT_CAP;
    arr->tokens = malloc(arr->cap * sizeof(*arr->tokens));
}

static void free_tok_arr(mjarr_t *arr) 
{
    assert(arr != NULL && "Array cannot be null");
    free(arr->tokens);
}

static int append_tok(mjarr_t *arr, mjtok_t token)
{
    assert(arr != NULL && "Array cannot be null");

    if (arr->count >= arr->cap) {
        size_t new_cap = arr->cap * MJTOK_ARRAY_GROWTH_FACTOR;
        mjtok_t *tmp = realloc(arr->tokens, new_cap * sizeof(*arr->tokens));
        MJ_RET_ERR_ON_NULL(tmp, "Out of memory failure");
        arr->tokens = tmp;
        arr->cap = new_cap;
    }

    arr->tokens[arr->count++] = token;

    return 0;
}

static inline mjtok_t new_tok(mjtok_type_t t, const void *val, size_t val_len)
{
    mjtok_t token = {
            .type = t,
            .value = val,
            .len = val_len
        };

    return token;
}

static int scan_str(mjarr_t *arr, const char **cptr, mjtok_t *out)
{
    assert(arr && *cptr && out && "Cannot be null");

    const char *val = *cptr;
    size_t val_len = 0;
    int quote_count = 0;

    /* Caller should call scan_str() if first quote
       is found, but here we make sure that this is true.
       If not, return an error. 
       */
    MJ_RET_ERR_ON_TRUE(!is_quote_double(**cptr), "Invalid string");

    for ( ; *cptr && **cptr != '\0' && quote_count < 2; (*cptr)++, val_len++) {
        if (is_quote_double(**cptr)) {
            quote_count++;
            continue;
        }
        if (is_bslash(**cptr)) {
            (*cptr)++; val_len++;
            MJ_RET_ERR_ON_TRUE(!(*cptr && **cptr != '\0'), "Invalid string");
            if (is_quote_double(**cptr) 
                || is_bslash(**cptr)
                || is_fslash(**cptr)
                || is_backspc_b(**cptr)
                || is_formfeed_f(**cptr)
                || is_linefeed_n(**cptr)
                || is_carret_r(**cptr)
                || is_tab_t(**cptr)) {
                (*cptr)++; val_len++;
                continue;
            }
            if (is_hex_u(**cptr)) {
                (*cptr)++; val_len++;
                MJ_RET_ERR_ON_TRUE(!(*cptr && **cptr != '\0'), "Invalid string");
                int hex_count = 0;
                for ( ; *cptr && **cptr != '\0' && hex_count < 4; (*cptr)++, val_len++) {
                    MJ_RET_ERR_ON_TRUE(!isxdigit((unsigned char) **cptr), "Invalid string");
                    hex_count++;
                }
                continue;
            }
        }
        MJ_RET_ERR_ON_TRUE(!isalpha((unsigned char) **cptr), "Invalid string");
    } 

    out->type = MJTOK_STRING;
    out->value = val;
    out->len = val_len;

    /* The loop finishes by advancing to the next character,
       but there is the outer loop that still advances (where
       the scan_str() is called. We should keep the pointer
       exactly where the character '"' is.*/
    (*cptr)--;
    
    MJ_LOG_STR("string: %s\n", val, val_len+1);

    return 0;
}

static int scan_num(mjarr_t *arr, const char **cptr, mjtok_t *out)
{
    assert(arr && *cptr && out && "Cannot be null");

    const char *val = *cptr;
    size_t val_len = 0;

    /* Caller should check that first character might be 
       valid digit ranging from 0-9 or starting with '-' symbol.
       This is just a check for safety. Return -1 if not true.
       */
    MJ_RET_ERR_ON_TRUE(!(isdigit(**cptr) || is_minus(**cptr)), "Invalid number");

    if (is_minus(**cptr)) {
        (*cptr)++; val_len++;
        MJ_RET_ERR_ON_TRUE(*cptr && !isdigit(**cptr), "Invalid number");
    }

    if (isdigit(**cptr)) {
        for ( ;*cptr && **cptr != '\0'; (*cptr)++, val_len++) {
            if (!isdigit(**cptr)) break;
        }
        if (is_dot(**cptr)) {
            (*cptr)++; val_len++;
            MJ_RET_ERR_ON_TRUE(is_exponent(**cptr), "Invalid digit after dot");
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
    out->len = val_len;

    /* The loop finishes by advancing to the next character,
       but there is the outer loop that still advances (where
       the scan_str() is called. We should keep the pointer
       exactly where the last digit is.*/
    (*cptr)--;

    MJ_LOG_STR("string: %s\n", val, val_len+1);

    return 0;
}

static int scan_bool_true(mjarr_t *arr, const char **cptr, mjtok_t *out)
{
    assert(arr && *cptr && out && "Cannot be null");

    const char *val = *cptr;
    size_t val_len = 0;

    /* Caller should check first if the character is 't' before
       calling this function. This is just a check for safety.
       Return -1 if not true.
       */
    MJ_RET_ERR_ON_TRUE(!(**cptr == CHAR_OF_TRUE(0)), "Invalid boolean");

    int next_idx = 0;
    for ( ; *cptr && **cptr != '\0' && next_idx < 4; (*cptr)++, val_len++) {
        MJ_RET_ERR_ON_TRUE(!(**cptr == CHAR_OF_TRUE(next_idx++)), "Invalid boolean");
    }

    out->type = MJTOK_BOOL;
    out->value = val;
    out->len = val_len;

    /* The loop finishes by advancing to the next character,
       but there is the outer loop that still advances (where
       the scan_str() is called. We should keep the pointer
       exactly where the character 'e' is.*/
    (*cptr)--;

    MJ_LOG_STR("string: %s\n", val, val_len+1);

    return 0;
}

static int scan_bool_false(mjarr_t *arr, const char **cptr, mjtok_t *out)
{
    assert(arr && *cptr && out && "Cannot be null");

    const char *val = *cptr;
    size_t val_len = 0;

    /* Caller should check first if the character is 'f' before
       calling this function. This is just a check for safety.
       Return -1 if not true.
       */
    MJ_RET_ERR_ON_TRUE(!(**cptr == CHAR_OF_FALSE(0)), "Invalid boolean");

    int next_idx = 0;
    for ( ; *cptr && **cptr != '\0' && next_idx < 5; (*cptr)++, val_len++) {
        MJ_RET_ERR_ON_TRUE(!(**cptr == CHAR_OF_FALSE(next_idx++)), "Invalid boolean");
    }

    out->type = MJTOK_BOOL;
    out->value = val;
    out->len = val_len;

    /* The loop finishes by advancing to the next character,
       but there is the outer loop that still advances (where
       the scan_str() is called. We should keep the pointer
       exactly where the character 'e' is.*/
    (*cptr)--;

    MJ_LOG_STR("string: %s\n", val, val_len+1);

    return 0;
}

static int scan_value_null(mjarr_t *arr, const char **cptr, mjtok_t *out)
{
    assert(arr && *cptr && out && "Cannot be null");

    const char *val = *cptr;
    size_t val_len = 0;

    /* Caller should check first if the character is 'n' before
       calling this function. This is just a check for safety.
       Return -1 if not true.
       */
    MJ_RET_ERR_ON_TRUE(!(**cptr == CHAR_OF_NULL(0)), "Invalid null");

    int next_idx = 0;
    for ( ; *cptr && **cptr != '\0' && next_idx < 4; (*cptr)++, val_len++) {
        MJ_RET_ERR_ON_TRUE(!(**cptr == CHAR_OF_NULL(next_idx++)), "Invalid null");
    }

    out->type = MJTOK_NULL;
    out->value = val;
    out->len = val_len;

    /* The loop finishes by advancing to the next character,
       but there is the outer loop that still advances (where
       the scan_str() is called. We should keep the pointer
       exactly where the character 'l' is.*/
    (*cptr)--;

    MJ_LOG_STR("string: %s\n", val, val_len+1);

    return 0;
}

static int tokenize_json(mjarr_t *arr, const char *json)
{
    const char *cptr = json; 

    while (cptr && *cptr != '\0') {
        MJ_LOG("char: %c\n", *cptr);
        if (isspace((unsigned char) *cptr) || *cptr == '\t' || *cptr == '\n' || *cptr == '\r') {
            goto advance;
        }
        if (is_brace_open(*cptr)) {
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_BRACE_OPEN, cptr, 1)));
            goto advance;
        }
        if (is_brace_close(*cptr)) {
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_BRACE_CLOSE, cptr, 1)));
            goto advance;
        }
        if (is_colon(*cptr)) {
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_COLON, cptr, 1)));
            goto advance;
        }
        if (is_bracket_open(*cptr)) {
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_BRACKET_OPEN, cptr, 1)));
            goto advance;
        }
        if (is_bracket_close(*cptr)) {
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_BRACKET_CLOSE, cptr, 1)));
            goto advance;
        }
        if (is_comma(*cptr)) {
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_COMMA, cptr, 1)));
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
        if (*cptr == CHAR_OF_TRUE(0)) {
            mjtok_t tok;
            MJ_RET_ON_ERR2(scan_bool_true(arr, &cptr, &tok));
            MJ_RET_ON_ERR2(append_tok(arr, tok));
            goto advance;
        }
        if (*cptr == CHAR_OF_FALSE(0)) {
            mjtok_t tok;
            MJ_RET_ON_ERR2(scan_bool_false(arr, &cptr, &tok));
            MJ_RET_ON_ERR2(append_tok(arr, tok));
            goto advance;
        }
        if (*cptr == CHAR_OF_NULL(0)) {
            mjtok_t tok;
            MJ_RET_ON_ERR2(scan_value_null(arr, &cptr, &tok));
            MJ_RET_ON_ERR2(append_tok(arr, tok));
            goto advance;
        }
advance:
        cptr++;
    }

    return 0;
}

/**
 * MyJSON Parser
 *
 * Reads array of tokens generated by the lexer and starts to build
 * AST (Abstract Syntax Tree).
 *
 */

typedef enum {
    MJ_NODE_OBJECT,
    MJ_NODE_PAIR,
    MJ_NODE_ARRAY,
    MJ_NODE_STRING,
    MJ_NODE_NUMBER,
    MJ_NODE_BOOL,
    MJ_NODE_NULL
} mjnode_type_t;

/**
 * The field 'node' is of type mj_node_t, but such node is generic.
 * Casting depends on 'type'.
 */
typedef struct {
    void            *node;
    mjnode_type_t   type;
} mj_node_t;

#define MJ_ARRAY_NODE_INIT_CAP        100
#define MJ_NODE_ARRAY_GROWTH_FACTOR   2

/**
 * Dynamic array of 'mj_node_t *'
 */
typedef struct {
    mj_node_t       **arr;
    size_t          count;
    size_t          cap;
    int             allow_growth;
} mj_arr_node_t;

typedef struct {
    const char      *value;
    size_t          len;
} mj_str_node_t;

typedef struct {
    const char      *value;
    size_t          len;
} mj_num_node_t;

typedef struct {
    const char      *value;
    size_t          len;
} mj_bool_node_t;

typedef struct {
    const char      *value;
    size_t          len;
} mj_null_node_t;

typedef struct {
    const char      *key;
    size_t          keylen;
    void            *value;
} mj_pair_node_t;

typedef struct {
    mj_arr_node_t   members;
} mj_obj_node_t;


/**
 * This is the type that is exposed to the user of library.
 * However, this is typedefed to myjson_t by myjson.h file.
 */
struct myjson {
    mj_node_t *root;
};

#define mj_frame_type(x)        x->frame_type
#define mj_frame_state(x)       x->frame_state
#define mj_frame_stack_top(x)   x->frames[x->count - 1]

typedef struct {
    enum {
        OBJ_FRAME,
        ARR_FRAME
    } frame_type;

    enum {
        OBJ_EXPECT_FIRST_KEY_OR_END,
        OBJ_EXPECT_KEY,
        OBJ_EXPECT_COLON,
        OBJ_EXPECT_VAL,
        OBJ_EXPECT_COMMA_OR_END,
        ARR_EXPECT_FIRST_VAL_OR_END,
        ARR_EXPECT_VAL,
        ARR_EXPECT_COMMA_OR_END
    } frame_state;
   
    mj_node_t   *node;
    const char  *pending_key;
    size_t      pending_keylen;

} mj_frame_t;

typedef struct {
    mj_frame_t  *frames;
    size_t      count;
    size_t      cap;
    int         allow_growth; //TODO
} mj_frame_stack_t;

#define MJ_FRAME_STACK_INIT_CAP         1000
#define MJ_FRAME_STACK_GROWTH_FACTOR    2
#define MJ_FRAME_STACK_ALLOW_GROWTH     1

/**
 * MyJSON free memory
 */

static void mj_free_obj_node(mj_obj_node_t *node);
static void mj_free_arr_node(mj_arr_node_t *node);
static void mj_free_pair_node(mj_pair_node_t *node);
static void mj_free_str_node(mj_str_node_t *node);
static void mj_free_bool_node(mj_bool_node_t *node);
static void mj_free_null_node(mj_null_node_t *node);
static void mj_free_node(mj_node_t *node);

static void mj_free_obj_node(mj_obj_node_t *node)
{
    for (size_t i = 0; i < node->members.cap; i++) {
        if (node->members.arr[i]) {
            if (node->members.arr[i]->node) {
                mj_pair_node_t *pair_node = node->members.arr[i]->node;
                mj_free_pair_node(pair_node);
            }
            free(node->members.arr[i]);
        }
    }
    free(node->members.arr);
    node->members.arr = NULL;
    node->members.count = 0;
    free(node);
}

static void mj_free_arr_node(mj_arr_node_t *node)
{
    for (size_t i = 0; i < node->cap; i++) {
        if (node->arr[i]) {
            if (node->arr[i]->node) {
                if (node->arr[i]->type == MJ_NODE_PAIR) {
                    mj_pair_node_t *pair_node = node->arr[i]->node;
                    mj_free_pair_node(pair_node);
                    continue;
                }
            }
            free(node->arr[i]);
        }
    }
    free(node->arr);
    node->arr = NULL;
    node->count = 0;
    free(node);
}

static void mj_free_pair_node(mj_pair_node_t *node)
{
    node->key = NULL;
    node->keylen = 0;
    mj_free_node(node->value);
    free(node);
}

static void mj_free_str_node(mj_str_node_t *node)
{
    if (node) {
        node->value = NULL;
        node->len = 0;
        free(node);
    }
}

static void mj_free_bool_node(mj_bool_node_t *node)
{
    if (node) {
        node->value = NULL;
        node->len = 0;
        free(node);
    }
}

static void mj_free_null_node(mj_null_node_t *node)
{
    if (node) {
        node->value = NULL;
        node->len = 0;
        free(node);
    }
}

static void mj_free_node(mj_node_t *node)
{
    switch(node->type) {
    case MJ_NODE_OBJECT:
        mj_free_obj_node(node->node);
        free(node);
        break;
    case MJ_NODE_ARRAY:
        mj_free_arr_node(node->node);
        break;
    case MJ_NODE_STRING: {
        mj_str_node_t *str_node = node->node;
        mj_free_str_node(str_node);
        free(node);
        break;
    }
    case MJ_NODE_NUMBER:
        break;
    case MJ_NODE_BOOL:
        mj_free_bool_node(node->node);
        free(node);
        break;
    case MJ_NODE_NULL:
        mj_free_null_node(node->node);
        free(node);
        break;
    default:
        fprintf(stderr, "Unknown json type");
    }
}

void myjson_free(myjson_t *mj)
{
    if (!mj || !mj->root) {
        fprintf(stderr, "Cannot free null json root");
        return;
    }
    mj_free_node(mj->root);
    mj->root = NULL;
    free(mj);
}

/**
 * MyJSON node allocator
 */

static void init_frame_stack(mj_frame_stack_t *stack) 
{
    assert(stack != NULL && "Statck cannot be null");

    stack->count = 0;
    stack->cap = MJ_FRAME_STACK_INIT_CAP;
    stack->frames = malloc(stack->cap * sizeof(*stack->frames));
    stack->allow_growth = MJ_FRAME_STACK_ALLOW_GROWTH;
}

static int push_frame(mj_frame_stack_t *stack, mj_frame_t frame)
{
    assert(stack != NULL && "Statck cannot be null");

    if (stack->count >= stack->cap && stack->allow_growth) {
        size_t new_cap = stack->cap * MJ_FRAME_STACK_GROWTH_FACTOR;
        mj_frame_t *tmp = realloc(stack->frames, new_cap * sizeof(*stack->frames));
        MJ_RET_ERR_ON_NULL(tmp, "Out of memory failure");
        stack->frames = tmp;
        stack->cap = new_cap;
    }

    stack->frames[stack->count++] = frame;

    return 0;
}

static void pop_frame(mj_frame_stack_t *stack)
{
    if (stack->count > 0) {
        size_t curr = stack->count - 1;
        stack->frames[curr].node = NULL;
        stack->frames[curr].pending_key = NULL;
        stack->frames[curr].pending_keylen = 0;
        stack->count--;
    }
}

static inline int is_frame_stack_empty(const mj_frame_stack_t *stack)
{
    assert(stack != NULL && "Null pointer");
    return stack->count == 0;
}

static void init_mj_arr_node(mj_arr_node_t *arr, size_t cap) 
{
    assert(arr != NULL && "Array cannot be null");

    arr->arr = calloc(cap, sizeof(arr->arr));
    arr->count = 0;
    arr->cap = cap;
}

static int append_mj_node(mj_arr_node_t *arr, mj_node_t *node)
{
    assert(arr != NULL && "Array cannot be null");

    if (arr->count >= arr->cap) {
        size_t new_cap = arr->cap * MJ_NODE_ARRAY_GROWTH_FACTOR;
        mj_node_t **tmp = realloc(arr->arr, new_cap * sizeof(arr->arr));
        MJ_RET_ERR_ON_NULL(tmp, "Out of memory failure");
        arr->arr = tmp;
        arr->cap = new_cap;
    }

    arr->arr[arr->count++] = node;

    return 0;
}

static mj_str_node_t *alloc_str_node(const char *val, size_t val_len)
{
    mj_str_node_t *str_node = malloc(sizeof(*str_node));
    str_node->value = val;
    str_node->len = val_len;
    return str_node;
}

static mj_bool_node_t *alloc_bool_node(const char *val, size_t val_len)
{
    mj_bool_node_t *bool_node = malloc(sizeof(*bool_node));
    bool_node->value = val;
    bool_node->len = val_len;
    return bool_node;
}

static mj_null_node_t *alloc_null_node(const char *val, size_t val_len)
{
    mj_null_node_t *null_node = malloc(sizeof(*null_node));
    null_node->value = val;
    null_node->len = val_len;
    return null_node;
}

static mj_obj_node_t *alloc_obj_node(size_t cap)
{
    mj_obj_node_t *obj_node = malloc(sizeof(*obj_node));
    init_mj_arr_node(&obj_node->members, cap);
    return obj_node;
}

static mj_arr_node_t *alloc_arr_node(size_t cap)
{
    mj_arr_node_t *arr_node = malloc(sizeof(*arr_node));
    init_mj_arr_node(arr_node, cap);
    return arr_node;
}

static mj_pair_node_t *alloc_pair_node(const char *key, size_t keylen, mj_node_t *val)
{
    mj_pair_node_t *pair_node = malloc(sizeof(*pair_node));
    pair_node->key = key;
    pair_node->keylen = keylen;
    pair_node->value = val;
    return pair_node;
}

static mj_node_t *alloc_node(void *child, mjnode_type_t type)
{
    mj_node_t *node = malloc(sizeof(*node));
    node->node = child;
    node->type = type;
    return node;
}

inline myjson_t *myjson_create()
{
    return malloc(sizeof(myjson_t));
}

/**
 * MyJSON parser into nodes
 */

static int attach_node(mj_frame_stack_t *stack, mj_node_t *node, mj_node_t **root)
{
    if (is_frame_stack_empty(stack)) {
        if (*root) {
            fprintf(stderr, "Unexpected extra json value after root\n");
            return -1;
        }
        *root = node;
        return 0;
    }

    mj_frame_t *frame = &mj_frame_stack_top(stack);

    if (mj_frame_type(frame) == ARR_FRAME) {
        if (mj_frame_state(frame) != ARR_EXPECT_FIRST_VAL_OR_END &&
            mj_frame_state(frame) != ARR_EXPECT_VAL) {
            fprintf(stderr, "Unexpected value in array\n");
            return -1;
        }

        mj_arr_node_t *arr_node = frame->node->node;
        arr_node->arr[arr_node->count] = node;
        arr_node->count++;

        frame->frame_state = ARR_EXPECT_COMMA_OR_END;

        return 0;
    }

    if (mj_frame_type(frame) == OBJ_FRAME) {
        if (mj_frame_state(frame) != OBJ_EXPECT_VAL) {
            fprintf(stderr, "Unexpected value in object\n");
            return -1;
        }

        mj_pair_node_t *pair_node = alloc_pair_node(frame->pending_key, frame->pending_keylen, node);
        mj_node_t *new_node = alloc_node(pair_node, MJ_NODE_PAIR);

        mj_obj_node_t *obj_node = frame->node->node;
        append_mj_node(&obj_node->members, new_node);

        frame->pending_key = NULL;
        frame->pending_keylen = 0;
        frame->frame_state = OBJ_EXPECT_COMMA_OR_END;
    }

    return 0;
}

static void mj_parse_obj_start(mj_frame_stack_t *stack, mj_node_t **root)
{
    mj_obj_node_t *obj_node = alloc_obj_node(MJ_ARRAY_NODE_INIT_CAP);
    mj_node_t *node = alloc_node(obj_node, MJ_NODE_OBJECT);

    if (attach_node(stack, node, root) < 0) {
        mj_free_obj_node(obj_node);
        free(node);
        return;
    }

    mj_frame_t frame;
    frame.node = node;
    frame.frame_type = OBJ_FRAME;
    frame.frame_state = OBJ_EXPECT_FIRST_KEY_OR_END;
    frame.pending_key = NULL;
    frame.pending_keylen = 0;

    if (push_frame(stack, frame) < 0) {
        mj_free_obj_node(obj_node);
        free(node);
    }
}

static void mj_parse_obj_end(mj_frame_stack_t *stack)
{
    if (is_frame_stack_empty(stack)) {
        fprintf(stderr, "Unexpected brace '}'\n");
        return;
    }
    
    mj_frame_t *frame = &mj_frame_stack_top(stack);

    if (mj_frame_type(frame) != OBJ_FRAME) {
        fprintf(stderr, "Unexpected brace '}' while parssing array\n");
        return;
    }

    if (mj_frame_state(frame) == OBJ_EXPECT_FIRST_KEY_OR_END) {
        pop_frame(stack);
        return;
    }

    if (mj_frame_state(frame) == OBJ_EXPECT_COMMA_OR_END) {
        pop_frame(stack);
        return;
    }

    if (mj_frame_state(frame) == OBJ_EXPECT_KEY) {
        fprintf(stderr, "Expected object key after ','\n");
        return;
    }

    if (mj_frame_state(frame) == OBJ_EXPECT_COLON) {
        fprintf(stderr, "Expected ':' after object key\n");
        return;
    }

    if (mj_frame_state(frame) == OBJ_EXPECT_VAL)
        fprintf(stderr, "Expected value after ':'\n");
}

static void mj_parse_str(mj_frame_stack_t *stack, mj_node_t **root, const char *val, size_t val_len)
{
    if (is_frame_stack_empty(stack)) {
        mj_str_node_t *str_node = alloc_str_node(val, val_len);
        mj_node_t *node = alloc_node(str_node, MJ_NODE_STRING);
        attach_node(stack, node, root);
        return;
    }

    mj_frame_t *frame = &mj_frame_stack_top(stack);
    
    if (mj_frame_type(frame) == OBJ_FRAME) {
        if (mj_frame_state(frame) == OBJ_EXPECT_FIRST_KEY_OR_END) {
            frame->pending_key = val;
            frame->pending_keylen = val_len;
            frame->frame_state = OBJ_EXPECT_COLON;
            return;
        }
        if (mj_frame_state(frame) == OBJ_EXPECT_KEY) {
            frame->pending_key = val;
            frame->pending_keylen = val_len;
            frame->frame_state = OBJ_EXPECT_COLON;
            return;
        }
        if (mj_frame_state(frame) == OBJ_EXPECT_VAL) {
            mj_str_node_t *str_node = alloc_str_node(val, val_len);
            mj_node_t *node = alloc_node(str_node, MJ_NODE_STRING);
            attach_node(stack, node, root);
            return;
        }
        fprintf(stderr, "Unexpected string in object\n");
        return;
    }

    if (mj_frame_type(frame) == ARR_FRAME) {
        mj_str_node_t *str_node = alloc_str_node(val, val_len);
        mj_node_t *node = alloc_node(str_node, MJ_NODE_STRING);
        attach_node(stack, node, root);
    }
}

static void mj_parse_colon(mj_frame_stack_t *stack)
{
    if (is_frame_stack_empty(stack)) {
        fprintf(stderr, "Unexpected ':' outside object\n");
        return;
    }

    mj_frame_t *frame = &mj_frame_stack_top(stack);

    if (mj_frame_type(frame) != OBJ_FRAME) {
        fprintf(stderr, "Unexpected ':' inside array\n");
        return;
    }

    if (mj_frame_state(frame) != OBJ_EXPECT_COLON) {
        fprintf(stderr, "Unexpected ':' in object\n");
        return;
    }

    frame->frame_state = OBJ_EXPECT_VAL;
}

static void mj_parse_bool(mj_frame_stack_t *stack, mj_node_t **root, const char *val, size_t val_len)
{
    mj_bool_node_t *bool_node = alloc_bool_node(val, val_len);
    mj_node_t *node = alloc_node(bool_node, MJ_NODE_BOOL);
    attach_node(stack, node, root);
}

static void mj_parse_null(mj_frame_stack_t *stack, mj_node_t **root, const char *val, size_t val_len)
{
    mj_null_node_t *null_node = alloc_null_node(val, val_len);
    mj_node_t *node = alloc_node(null_node, MJ_NODE_NULL);
    attach_node(stack, node, root);
}

static void mj_parse_comma(mj_frame_stack_t *stack)
{
    if (is_frame_stack_empty(stack)) {
        fprintf(stderr, "Unexpected ',' outside container");
        return;
    }

    mj_frame_t *frame = &mj_frame_stack_top(stack);

    if (mj_frame_type(frame) == OBJ_FRAME) {
        if (mj_frame_state(frame) != OBJ_EXPECT_COMMA_OR_END) {
            fprintf(stderr, "Unexpected ',' in object\n");
            return;
        }
        frame->frame_state = OBJ_EXPECT_KEY;
        return;
    }

    if (mj_frame_type(frame) == ARR_FRAME) {
        if (mj_frame_state(frame) != ARR_EXPECT_COMMA_OR_END) {
            fprintf(stderr, "Unexpected ',' in array\n");
            return;
        }
        frame->frame_state = ARR_EXPECT_VAL;
        return;
    }
}

static void mj_parse_arr_start(mj_frame_stack_t *stack, mj_node_t **root)
{
    mj_arr_node_t *arr_node = alloc_arr_node(MJ_ARRAY_NODE_INIT_CAP);
    mj_node_t *node = alloc_node(arr_node, MJ_NODE_ARRAY);

    if (attach_node(stack, node, root) < 0) {
        mj_free_arr_node(arr_node);
        free(node);
        return;
    }

    mj_frame_t frame;
    frame.node = node;
    frame.frame_type = ARR_FRAME;
    frame.frame_state = ARR_EXPECT_FIRST_VAL_OR_END;

    if (push_frame(stack, frame) < 0) {
        mj_free_arr_node(arr_node);
        free(node);
    }
}

static void mj_parse_arr_end(mj_frame_stack_t *stack)
{
    if (is_frame_stack_empty(stack)) {
        fprintf(stderr, "Unexpected bracket ']'\n");
        return;
    }
    
    mj_frame_t *frame = &mj_frame_stack_top(stack);

    if (mj_frame_type(frame) != ARR_FRAME) {
        fprintf(stderr, "Unexpected bracket ']' while parsing object\n");
        return;
    }

    if (mj_frame_state(frame) == ARR_EXPECT_FIRST_VAL_OR_END) {
        pop_frame(stack);
        return;
    }

    if (mj_frame_state(frame) == ARR_EXPECT_COMMA_OR_END) {
        pop_frame(stack);
        return;
    }

    if (mj_frame_state(frame) == ARR_EXPECT_VAL)
        fprintf(stderr, "Expected array value after ','\n");
}

int myjson_parse(myjson_t *mj, const char *json)
{
    MJ_RET_ERR_ON_TRUE(!mj || !json, "Null pointer");

    /* Root node of json tree */
    mj->root = NULL;

    mj_frame_stack_t stack;
    init_frame_stack(&stack);

    mjarr_t arr;
    init_tok_arr(&arr);
    MJ_RET_ON_ERR(tokenize_json(&arr, json), "Invalid json data");

    for (size_t curr = 0; curr < arr.count; curr++) {
        mjtok_t tok = arr.tokens[curr];
        switch (tok.type) {
        case MJTOK_BRACE_OPEN:
            mj_parse_obj_start(&stack, &mj->root);
            break;
        case MJTOK_BRACE_CLOSE:
            mj_parse_obj_end(&stack);
            break;
        case MJTOK_BRACKET_OPEN:
            mj_parse_arr_start(&stack, &mj->root);
            break;
        case MJTOK_BRACKET_CLOSE:
            mj_parse_arr_end(&stack);
            break;
        case MJTOK_STRING:
            mj_parse_str(&stack, &mj->root, tok.value, tok.len);
            break;
        case MJTOK_NUMBER:
            //TODO:
            break;
        case MJTOK_BOOL:
            mj_parse_bool(&stack, &mj->root, tok.value, tok.len);
            break;
        case MJTOK_NULL:
            mj_parse_null(&stack, &mj->root, tok.value, tok.len);
            break;
        case MJTOK_COLON:
            mj_parse_colon(&stack);
            break;
        case MJTOK_COMMA:
            mj_parse_comma(&stack);
            break;
        default:
            fprintf(stderr, "Unexpected token\n");
            return -1;
        }
    }

    free_tok_arr(&arr);

    if (!is_frame_stack_empty(&stack)) {
        fprintf(stderr, "Unclosed json structure\n");
        free(stack.frames);
        return -1;
    }

    free(stack.frames);

    MJ_RET_ERR_ON_NULL(mj->root, "Expected json value");

    return 0;
}

/**
 * MyJSON Printer
 */

static void mj_print_bool_node(mj_bool_node_t *node);
static void mj_print_null_node(mj_null_node_t *node);
static void mj_print_str_node(const char *s, size_t slen);
static void mj_print_obj_node(const mj_obj_node_t *node);
static void mj_print_arr_node(const mj_arr_node_t *node);
static void mj_print_pair_node(const mj_pair_node_t *node);
static void mj_print_node(const mj_node_t *node);

static void mj_print_null_node(mj_null_node_t *node)
{
    if (strncmp(node->value, "null", node->len) == 0)
        printf("null");
}

static void mj_print_bool_node(mj_bool_node_t *node)
{
    if (strncmp(node->value, "true", node->len) == 0)
        printf("true");
    else
        printf("false");
}

static void mj_print_str_node(const char *s, size_t slen)
{
    printf("\"");

    for (size_t i = 0; i < slen; i++) {
        if (is_quote_double(s[i])) {
            printf("\\\"");
            continue;
        }
        if (is_bslash(s[i])) {
            printf("\\\\");
            continue;
        }
        if (is_linefeed_n(s[i])) {
            printf("\\n");
            continue;
        }
        if (is_carret_r(s[i])) {
            printf("\\r");
            continue;
        }
        if (is_tab_t(s[i])) {
            printf("\\t");
            continue;
        }
        if (is_backspc_b(s[i])) {
            printf("\\b");
            continue;
        }
        if (is_formfeed_f(s[i])) {
            printf("\\f");
            continue;
        }
        //TODO: print unicode escape
        printf("%c", s[i]);
    }

    printf("\"");
}

static void mj_print_obj_node(const mj_obj_node_t *node)
{
    printf("{");

    for (size_t i = 0; i < node->members.count; i++) {
        const mj_pair_node_t *pair_node = node->members.arr[i]->node;
        mj_print_pair_node(pair_node);

        if (i + 1 < node->members.count)
            printf(",");
    }

    printf("}");
}

static void mj_print_arr_node(const mj_arr_node_t *node)
{
    printf("[");

    for (size_t i = 0; i < node->count; i++) {
        mj_print_node(node->arr[i]);

        if (i + 1 < node->count)
            printf(",");
    }

    printf("]");
}

static void mj_print_pair_node(const mj_pair_node_t *node)
{
    mj_print_str_node(node->key, node->keylen);

    printf(":");

    mj_print_node(node->value);
}

static void mj_print_node(const mj_node_t *node)
{
    switch(node->type) {
    case MJ_NODE_OBJECT:
        mj_print_obj_node(node->node);
        break;
    case MJ_NODE_ARRAY:
        mj_print_arr_node(node->node);
        break;
    case MJ_NODE_STRING: {
        mj_str_node_t *str_node = node->node;
        mj_print_str_node(str_node->value, str_node->len);
        break;
    }
    case MJ_NODE_NUMBER:
        break;
    case MJ_NODE_BOOL:
        mj_print_bool_node(node->node);
        break;
    case MJ_NODE_NULL:
        mj_print_null_node(node->node);
        break;
    default:
        fprintf(stderr, "Unknown json type");
    }
}

void myjson_print(const myjson_t *mj)
{
    if (!mj || !mj->root) {
        fprintf(stderr, "Cannot print null json root");
        return;
    }
    mj_print_node(mj->root);
}


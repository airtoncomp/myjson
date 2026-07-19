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
    out->len = ++val_len;
    
    MJ_LOG_STR("string: %s\n", val, val_len);

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
    out->len = ++val_len;

    MJ_LOG_STR("string: %s\n", val, val_len);

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
    out->len = ++val_len;

    MJ_LOG_STR("string: %s\n", val, val_len);

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
    out->len = ++val_len;

    MJ_LOG_STR("string: %s\n", val, val_len);

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
    out->len = ++val_len;

    MJ_LOG_STR("string: %s\n", val, val_len);

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
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_BRACE_OPEN, cptr, 1)));
            goto advance;
        }
        if (is_bracket_close(*cptr)) {
            MJ_RET_ON_ERR2(append_tok(arr, new_tok(MJTOK_BRACE_CLOSE, cptr, 1)));
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

typedef struct {
    const void      *value;
    size_t          len;
} mj_str_node_t;

typedef struct {
    const void      *value;
    size_t          len;
} mj_num_node_t;

typedef struct {
    const void      *value;
    size_t          len;
} mj_bool_node_t;

typedef struct {
    mj_node_t       *elems;
    size_t          count;
} mj_arr_node_t;

typedef struct {
    const void      *key;
    const void      *value;
} mj_pair_node_t;

typedef struct {
    mj_node_t       *members;
    size_t          count;
} mj_obj_node_t;


/**
 * This is the type that is exposed to the user of library.
 * However, this is typedefed to myjson_t by myjson.h file.
 */
struct myjson {
    mj_node_t root;
};

#define mj_frame_type(x)        x.frame_type
#define mj_frame_state(x)       x.frame_state
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
    mj_node_t   *pending_key;

} mj_frame_t;

typedef struct {
    mj_frame_t  *frames;
    size_t      count;
    size_t      cap;
    int         allow_growth;
} mj_frame_stack_t;

#define MJ_FRAME_STACK_INIT_CAP         1000
#define MJ_FRAME_STACK_GROWTH_FACTOR    2
#define MJ_FRAME_STACK_ALLOW_GROWTH     1
#define MJ_FRAME_STACK_DENY_GROWTH      1

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
        free(stack->frames[curr].node);
        stack->frames[curr].node = NULL;
        free(stack->frames[curr].pending_key);
        stack->frames[curr].pending_key = NULL;
        stack->count--;
    }
}

static inline int is_frame_stack_empty(const mj_frame_stack_t *stack)
{
    assert(stack != NULL && "Null pointer");
    return stack->count == 0;
}

static void attach_node(mj_frame_stack_t *stack, mj_node_t *node, mj_node_t *root)
{
    if (is_frame_stack_empty(stack)) {
        if (root->node) {
            fprintf(stderr, "Unexpected extra json value after root\n");
            return;
        }
        root->node = node;
        root->type = MJ_NODE_OBJECT; 
        return;
    }

    mj_frame_t frame = mj_frame_stack_top(stack);

    if (mj_frame_type(frame) == ARR_FRAME) {
        if (mj_frame_state(frame) != ARR_EXPECT_FIRST_VAL_OR_END &&
            mj_frame_state(frame) != ARR_EXPECT_VAL) {
            fprintf(stderr, "Unexpected value in array\n");
        }

        mj_arr_node_t *arr_node = frame.node->node;
        arr_node->elems[arr_node->count].node = node;
        arr_node->count++;

        frame.frame_state = ARR_EXPECT_FIRST_VAL_OR_END;

        return;
    }

    if (mj_frame_type(frame) == OBJ_FRAME) {
        if (mj_frame_state(frame) != OBJ_EXPECT_VAL) {
            fprintf(stderr, "Unexpected value in object\n");
        }

        mj_pair_node_t *pair_node = malloc(sizeof(*pair_node));
        pair_node->key = frame.pending_key;
        pair_node->value = node;
        
        mj_obj_node_t *obj_node = frame.node->node;
        obj_node->members[obj_node->count].node = pair_node;
        obj_node->count++;

        frame.pending_key = NULL;
        frame.frame_state = OBJ_EXPECT_COMMA_OR_END;

        return;
    }
}

static void mj_parse_obj_start(mj_frame_stack_t *stack, mj_node_t *root)
{
    mj_node_t *node = malloc(sizeof(*node));
    node->type = MJ_NODE_OBJECT;

    mj_obj_node_t *obj = malloc(sizeof(*obj));
    obj->members = NULL;
    obj->count = 0;

    node->node = obj;

    attach_node(stack, node, root);

    mj_frame_t frame;
    frame.node = node;
    frame.frame_type = OBJ_FRAME;
    frame.frame_state = OBJ_EXPECT_FIRST_KEY_OR_END;
    frame.pending_key = NULL;

    push_frame(stack, frame);
}

static void mj_parse_obj_end(mj_frame_stack_t *stack)
{
    if (is_frame_stack_empty(stack)) 
        fprintf(stderr, "Unexpected brace '}'\n");
    
    mj_frame_t frame = mj_frame_stack_top(stack);

    if (mj_frame_type(frame) != OBJ_FRAME)
        fprintf(stderr, "Unexpected brace '}' while parssing array\n");

    if (mj_frame_state(frame) == OBJ_EXPECT_FIRST_KEY_OR_END) {
        pop_frame(stack);
        return;
    }

    if (mj_frame_state(frame) == OBJ_EXPECT_COMMA_OR_END) {
        pop_frame(stack);
        return;
    }

    if (mj_frame_state(frame) == OBJ_EXPECT_KEY)
        fprintf(stderr, "Expected object key after ','\n");

    if (mj_frame_state(frame) == OBJ_EXPECT_COLON)
        fprintf(stderr, "Expected ':' after object key\n");

    if (mj_frame_state(frame) == OBJ_EXPECT_VAL)
        fprintf(stderr, "Expected value after ':'\n");
}

int myjson_parse(myjson_t *mj, const char *json)
{
    MJ_RET_ERR_ON_TRUE(!mj || !json, "Null pointer");

    mj->root.node = NULL;

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
            break;
        case MJTOK_BRACKET_CLOSE:
            break;
        case MJTOK_STRING:
            break;
        case MJTOK_NUMBER:
            break;
        case MJTOK_BOOL:
            break;
        case MJTOK_NULL:
            break;
        case MJTOK_COLON:
            break;
        case MJTOK_COMMA:
            break;
        default:
            MJ_LOGE("Unexpected token");
            return -1;
        }
    }

    //TODO: if stack is not empty, return error: unclosed json object
    //TODO: if root is null, return error: expected json value

    return 0;
}

void test() {
    mjarr_t array;
    init_tok_arr(&array);
    mjtok_t token = {
        .type = MJTOK_STRING,
        .value = "a",
        .len = 1
    };
    append_tok(&array, token);

    mjtok_t token2 = {
        .type = MJTOK_STRING,
        .value = "b",
        .len = 1
    };
    append_tok(&array, token2);

    mjarr_t arr2; 
    init_tok_arr(&arr2);
    //char *str = R"({"a":"b", "c":[], "d":1, "e":2.4, "f":0.0e10})";
    //char *str = "{\"a\":\"b\", \"c\":[]}";
    //char *str = R"({"a":0.0e12, "b":0.1E-56, "c":2.5e+4, "d":6e7})";
    char *str = R"({"a":true, "b":false, "c":null})";

    printf("%s\n", str);

    /*tokenize_json(&arr2, str);
    for (size_t i = 0; i < arr2.count; i++)
        printf("token: type=%d, value=%c, len=%zu\n",
                arr2.tokens[i].type, *(char *) arr2.tokens[i].value, arr2.tokens[i].len);*/

    myjson_t mj;
    myjson_parse(&mj, str);
}

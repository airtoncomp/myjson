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

#ifndef _MYJSON_H_
#define _MYJSON_H_

#include <string.h>

#define MJ_STR(x) #x

/**
 * Public JSON value type exposed by the library.
 * Users should refer to this type through the myjson_t
 * typedef when parsing and working with JSON data.
 */
typedef struct myjson myjson_t;

/**
 * Public.
 * User should use 'myjson_t' as main variable to represent 
 * json data and make call to functions that use this type.
 * The 'create' functions make memory allocation while
 * 'free' ones destroy allocated memories.
 */

myjson_t *myjson_create();
myjson_t *myjson_create_root();
myjson_t *myjson_create_obj(size_t cap);
myjson_t *myjson_create_pair_str(const char *key, char *val);
//myjson_t *myjson_create_pair_int(const char *key, int val); //TODO
//myjson_t *myjson_create_pair_double(const char *key, double val); //TODO
//myjson_t *myjson_create_pair_true(const char *key); //TODO
//myjson_t *myjson_create_pair_false(const char *key); //TODO
myjson_t *myjson_create_arr(size_t cap);

void myjson_add_obj_to_root(myjson_t **root, myjson_t *obj);
void myjson_add_pair_to_obj(myjson_t *obj, myjson_t *pair);

int myjson_parse(myjson_t *mj, const char *json);
void myjson_print(const myjson_t *mj);
void myjson_free_deep(myjson_t *mj);
void myjson_free(myjson_t *mj);

#endif

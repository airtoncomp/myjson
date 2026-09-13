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
myjson_t *myjson_create_pair_int(const char *key, int val);
myjson_t *myjson_create_pair_double(const char *key, double val);
myjson_t *myjson_create_pair_true(const char *key);
myjson_t *myjson_create_pair_false(const char *key);
myjson_t *myjson_create_pair_null(const char *key);
myjson_t *myjson_create_pair_arr(const char *key, myjson_t *arr);
myjson_t *myjson_create_arr(size_t cap);

void myjson_add_obj_to_root(myjson_t *root, myjson_t *obj);
void myjson_add_arr_to_root(myjson_t *root, myjson_t *arr);
void myjson_add_pair_to_obj(myjson_t *obj, myjson_t *pair);

void myjson_append_str_to_arr(myjson_t *arr, char *val);
void myjson_append_int_to_arr(myjson_t *arr, int val);
void myjson_append_double_to_arr(myjson_t *arr, double val);
void myjson_append_true_to_arr(myjson_t *arr);
void myjson_append_false_to_arr(myjson_t *arr);
void myjson_append_null_to_arr(myjson_t *arr);
void myjson_append_obj_to_arr(myjson_t *arr, myjson_t *obj);

void myjson_replace_arr_elem_w_int(myjson_t *arr, size_t idx, int elem);
void myjson_replace_arr_elem_w_double(myjson_t *arr, size_t idx, double elem);
void myjson_replace_arr_elem_w_true(myjson_t *arr, size_t idx);
void myjson_replace_arr_elem_w_false(myjson_t *arr, size_t idx);
void myjson_replace_arr_elem_w_null(myjson_t *arr, size_t idx);
void myjson_replace_arr_elem_w_str(myjson_t *arr, size_t idx, char *elem);

void myjson_del_pair_from_obj(myjson_t *obj, const char *key);
void myjson_del_elem_from_arr(myjson_t *arr, size_t idx);

void myjson_update_pair_in_obj(myjson_t *obj, const char *key, const myjson_t *mj);
void myjson_update_str_pair_in_obj(myjson_t *obj, const char *key, const char *val);
void myjson_update_int_pair_in_obj(myjson_t *obj, const char *key, int val);
void myjson_update_double_pair_in_obj(myjson_t *obj, const char *key, double val);
void myjson_update_bool_pair_in_obj(myjson_t *obj, const char *key, int zero_or_one);

const myjson_t *myjson_get_item(const myjson_t *root, const char *key);

int myjson_parse(myjson_t *mj, const char *json);
void myjson_print(const myjson_t *mj);
void myjson_free_root(myjson_t *mj);

#endif

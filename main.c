/**
 * 2026, My personal json parser named MyJSON
 * Author: Airton Ishimori
 */

#include <stdio.h>
#include <stdlib.h>

#include "myjson.h"

void test() 
{
    printf("\n--- BEGIN: TEST TOKENIZER AND PARSER ---\n");

    //char *str = R"({"a":"b", "c":[], "d":1, "e":2.4, "f":0.0e10})";
    //char *str = "{\"a\":\"b\", \"c\":[]}";
    //char *str = R"({"a":0.0e12, "b":0.1E-56, "c":2.5e+4, "d":6e7})";
    //char *str = R"({"a":true, "b":false, "c":null})";
    //char *str = R"({"a":true, "b":{"mykey":"myval"}, "c":null})";
    //char *str = R"({"a":true, "b":{"mykey":"myval"}, "c":null})";
    //char *str = R"({"a":true})";
    //char *str = R"({"a":[true]})";
    //char *str = R"({"a":[{"b":"c"},{"d":true}, "elem"]})";
    //char *str = R"(["a",true])";
    //char *str = R"({"a":"c"})";
    char *str = R"({"a":2})";

    printf("%s\n", str);

    myjson_t *mj = myjson_create();
    myjson_parse(mj, str);
    myjson_print(mj);
    myjson_free_root(mj);

    printf("\n--- END: TEST TOKENIZER AND PARSER ---\n");
}

void test_object()
{
    printf("\n--- BEGIN: TEST OBJECT CREATION ---\n");

    myjson_t *mj = myjson_create_root();
    myjson_t *obj = myjson_create_obj(6);
    myjson_t *str_pair = myjson_create_pair_str(MJ_STR("a"), MJ_STR("\r"));
    myjson_t *int_pair = myjson_create_pair_int(MJ_STR("b"), 45);
    myjson_t *double_pair = myjson_create_pair_double(MJ_STR("c"), 32.5e-4);
    myjson_t *true_pair = myjson_create_pair_true(MJ_STR("d"));
    myjson_t *false_pair = myjson_create_pair_false(MJ_STR("f"));
    myjson_t *null_pair = myjson_create_pair_null(MJ_STR("g"));

    myjson_add_pair_to_obj(obj, str_pair);
    myjson_add_pair_to_obj(obj, int_pair);
    myjson_add_pair_to_obj(obj, double_pair);
    myjson_add_pair_to_obj(obj, true_pair);
    myjson_add_pair_to_obj(obj, false_pair);
    myjson_add_pair_to_obj(obj, null_pair);

    myjson_del_pair_from_obj(obj, MJ_STR("c"));
    myjson_update_str_pair_in_obj(obj, MJ_STR("a"), MJ_STR("test"));
    myjson_update_int_pair_in_obj(obj, MJ_STR("b"), 90);
    myjson_update_double_pair_in_obj(obj, MJ_STR("c"), -12.9);
    myjson_update_bool_pair_in_obj(obj, MJ_STR("f"), 1);

    myjson_add_obj_to_root(&mj, obj);

    myjson_print(mj);

    myjson_free(obj);
    myjson_free(str_pair);
    myjson_free(int_pair);
    myjson_free(double_pair);
    myjson_free(true_pair);
    myjson_free(false_pair);
    myjson_free(null_pair);
    myjson_free_root(mj);

    printf("\n--- END: TEST OBJECT CREATION ---\n");
}

void test_object2()
{
    printf("\n--- BEGIN: TEST OBJECT CREATION ---\n");

    myjson_t *mj = myjson_create_root();
    myjson_t *obj = myjson_create_obj(1);

    myjson_t *int_pair = myjson_create_pair_int(MJ_STR("b"), 45);

    myjson_add_pair_to_obj(obj, int_pair);

    myjson_add_obj_to_root(&mj, obj);

    myjson_t *new_int_pair = myjson_create_pair_int(MJ_STR("b"), 234);
    myjson_update_pair_in_obj(obj, MJ_STR("b"), new_int_pair);

    myjson_print(mj);

    myjson_free(obj);
    myjson_free(int_pair);
    myjson_free(new_int_pair);
    myjson_free_root(mj);

    printf("\n--- END: TEST OBJECT CREATION ---\n");
}

void test_array()
{
    printf("\n--- BEGIN: TEST ARRAY CREATION ---\n");

    myjson_t *mj = myjson_create_root();
    myjson_t *arr = myjson_create_arr(7);
    
    myjson_append_str_to_arr(arr, MJ_STR("name"));
    myjson_append_int_to_arr(arr, 23);
    myjson_append_double_to_arr(arr, -4.5e-3);
    myjson_append_true_to_arr(arr);
    myjson_append_false_to_arr(arr);
    myjson_append_null_to_arr(arr);

    myjson_t *obj = myjson_create_obj(1);
    myjson_t *str_pair = myjson_create_pair_str(MJ_STR("a"), MJ_STR("\r"));
    myjson_add_pair_to_obj(obj, str_pair);

    myjson_replace_arr_elem_w_int(arr, 0, 678);
    myjson_replace_arr_elem_w_double(arr, 1, 33.2e+2);
    myjson_replace_arr_elem_w_true(arr, 1);
    myjson_replace_arr_elem_w_false(arr, 1);
    myjson_replace_arr_elem_w_null(arr, 1);
    myjson_replace_arr_elem_w_str(arr, 0, MJ_STR("testing"));

    myjson_del_elem_from_arr(arr, 1);

    myjson_append_obj_to_arr(arr, obj);

    myjson_add_arr_to_root(&mj, arr);

    myjson_print(mj);

    myjson_free(str_pair);
    myjson_free(obj);
    myjson_free(arr);
    myjson_free_root(mj);

    printf("\n--- END: TEST ARRAY CREATION ---\n");
}

int main () 
{
    test();
    printf("\n");

    test_object();
    printf("\n");

    test_object2();
    printf("\n");

    test_array();
    printf("\n");

    printf("myjson lib\n");

    return 0;
}

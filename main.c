/**
 * 2026, My personal json parser named MyJSON
 * Author: Airton Ishimori
 */

#include <stdio.h>
#include <stdlib.h>

#include "myjson.h"

void test() 
{
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
}

void test_object()
{
    printf("\n--- BEGIN: TEST OBJECT CREATION ---\n");

    myjson_t *mj = myjson_create_root();
    myjson_t *obj = myjson_create_obj(3);
    myjson_t *str_pair = myjson_create_pair_str(MJ_STR("a"), MJ_STR("\r"));
    myjson_t *int_pair = myjson_create_pair_int(MJ_STR("b"), 45);
    myjson_t *double_pair = myjson_create_pair_double(MJ_STR("c"), 32.5e-4);

    myjson_add_pair_to_obj(obj, str_pair);
    myjson_add_pair_to_obj(obj, int_pair);
    myjson_add_pair_to_obj(obj, double_pair);
    myjson_add_obj_to_root(&mj, obj);
    
    myjson_print(mj);
    myjson_free(obj);
    myjson_free(str_pair);
    myjson_free(int_pair);
    myjson_free(double_pair);
    myjson_free_root(mj);

    printf("\n--- END: TEST OBJECT CREATION ---\n");
}

int main () 
{
    test();
    printf("\n");

    test_object();
    printf("\n");

    printf("myjson lib\n");

    return 0;
}

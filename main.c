/**
 * 2026, My personal json parser named MyJSON
 * Author: Airton Ishimori
 */

#include <stdio.h>
#include <stdlib.h>

#include "myjson.h"

void test() {
    //char *str = R"({"a":"b", "c":[], "d":1, "e":2.4, "f":0.0e10})";
    //char *str = "{\"a\":\"b\", \"c\":[]}";
    //char *str = R"({"a":0.0e12, "b":0.1E-56, "c":2.5e+4, "d":6e7})";
    //char *str = R"({"a":true, "b":false, "c":null})";
    //char *str = R"({"a":true, "b":{"mykey":"myval"}, "c":null})";
    char *str = R"({"a":true, "b":{"mykey":"myval"}, "c":null})";
    //char *str = R"({"a":true})";

    printf("%s\n", str);

    myjson_t *mj = myjson_create();
    myjson_parse(mj, str);
    myjson_print(mj);
    myjson_free(mj);
}

int main () 
{
    test();
    printf("\nmyjson lib\n");
    return 0;
}

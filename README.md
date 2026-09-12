# MyJSON

MyJSON is a lightweight JSON parsing library created as a personal project.
It can be added directly to your project and used wherever its features meet your requirements.

The library has not been extensively tested, so it may not be suitable for production-critical applications.

```
$ cc -DDEBUG -std=c17 -Wall -Wpedantic -Wextra -Werror -g -o myjson main.c myjson.c && ./myjson
$ valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -s ./myjson
```

## Usage examples

MyJSON requires a root node, which must be created with `myjson_create()`.

To parse a JSON string, pass the root node and the input string to `myjson_parse()`.
The function tokenizes and parses the JSON data into MyJSON's internal data structures.

When the JSON tree is no longer needed, release all associated memory by calling `myjson_free_root(root)`.

```
const char *str = "{\"a\":[{\"b\":\"c\"},{\"d\":true},\"elem\"]}";

myjson_t *root = myjson_create();

myjson_parse(root, str);
myjson_print(root);

myjson_free_root(root);
```

To build a JSON document programmatically, create the required nodes with the `myjson_create_*` functions,
attach child nodes to their parent nodes, and finally attach the top-level object to the root node.

```
myjson_t *root = myjson_create_root();

myjson_t *arr = myjson_create_arr(2);
myjson_append_str_to_arr(arr, MJ_STR("letter_a"));
myjson_append_str_to_arr(arr, MJ_STR("letter_b"));

myjson_t *obj = myjson_create_obj(2);
myjson_t *int_pair = myjson_create_pair_int(MJ_STR("number"), 654);
myjson_t *arr_pair = myjson_create_pair_arr(MJ_STR("array"), arr);

myjson_add_pair_to_obj(obj, int_pair);
myjson_add_pair_to_obj(obj, arr_pair);

myjson_add_obj_to_root(root, obj);

myjson_print(root);
myjson_free_root(root);
```

Each node is attached to a parent node until the complete JSON tree is connected to the root.
Once the tree is no longer needed, call `myjson_free_root()` to release all memory owned by it.


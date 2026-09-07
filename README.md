# MyJSON

MyJSON is a lightweight JSON parsing library created as a personal project.
It can be added directly to your project and used wherever its features meet your requirements.

The library has not been extensively tested, so it may not be suitable for production-critical applications.

```
cc -DDEBUG -std=gnu11 -Wall -Wpedantic -Wextra -Werror -g -o myjson main.c myjson.c && ./myjson
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -s ./myjson
```

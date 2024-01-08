// memory-alloctor.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once

#include <mutex>
using namespace std;


#include <string.h>

/* Only for the debug printf */
#include <stdio.h>
void my_free(void* block);
void* my_realloc(void* block, size_t size);
void print_mem_list();
void* my_calloc(size_t num, size_t nsize);
void* my_malloc(size_t size);
// TODO: 在此处引用程序需要的其他标头。

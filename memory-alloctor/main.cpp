#include"memory-alloctor.h"
void hello(char* a) {
	memcpy(a, "hello", 6);
}
int main() {
	char* a = (char*)my_malloc(10);
	print_mem_list();
	char* b = (char*)my_malloc(10);
	print_mem_list();
	my_free(a);
	print_mem_list();
	void*c = my_malloc(100);
	print_mem_list();
	void* d = my_malloc(20);
	print_mem_list();
	my_free(b);
	print_mem_list();
	a = (char*)my_malloc(20);
	print_mem_list();
}
// memory-alloctor.cpp: 定义应用程序的入口点。
//

#include "memory-alloctor.h"


typedef char ALIGN[16];
char* heap = nullptr;
size_t heap_top = 0;

union header {
	struct {
		size_t size;
		unsigned is_free;
		union header* next;
	} s;
	/* 使用union的特性使header保持在16字节 */
	ALIGN stub;
};
typedef union header header_t;

header_t* head = NULL, * tail = NULL;
std::recursive_mutex global_malloc_lock;
/*
模拟linux的sbrk
*/
void* sbrk(rsize_t length) {
	/*模拟假设内存只有10000Bytes*/

	if (heap == nullptr) {
		heap = new char[10000];
	}
	void* p = (void*)(heap + heap_top);
	if (heap_top + length < 0) {
		return 0;
	}
	if (heap_top + length > 10000) {
		return 0;
	}
	heap_top = heap_top + length;
	return p;

}

header_t* get_free_block(size_t size)
{
	if (head == nullptr) {
		return NULL;
	}
	header_t* curr = head;
	while (curr) {
		/* 如果找到满足的内存块就返回 */
		if (curr->s.is_free && curr->s.size >= size)
			return curr;
		curr = curr->s.next;
	}
	return NULL;
}

void my_free(void* block)
{
	header_t* header, * tmp;
	/* program break is the end of the process's data segment */
	void* programbreak;

	if (!block)
		return;
	global_malloc_lock.lock();
	header = (header_t*)block - 1;
	/* sbrk(0) 返回现在堆顶的位置 */
	programbreak = sbrk(0);

	/*
	   检查这个内存块是不是最后一块如果是我们可以将其还给os如果不是
	   我们只能标记其空闲
	 */
	if ((char*)block + header->s.size == programbreak) {
		if (head == tail) {
			head = tail = NULL;
		}
		else {
			tmp = head;
			while (tmp) {
				if (tmp->s.next == tail) {
					tmp->s.next = NULL;
					tail = tmp;
				}
				tmp = tmp->s.next;
			}
		}
		/*
		  当 sbrk() 函数的参数为负数时，它会减小程序的程序断点（program break）
		  。因此，内存会被程序释放给操作系统。
		*/
		sbrk(0 - header->s.size - sizeof(header_t));
		
		/*
		注意 这个互斥并不能保证线程安全 因为在linux中sbrk不是线程安全的 
		假设在我们找到堆顶并将其减小之后，另一个线程调用了 sbrk() 来增加堆空间的大小。
		这种情况下，我们将释放了由另一个线程通过 sbrk() 获得的内存，从而导致错误的释放内存。
		*/
		global_malloc_lock.unlock();
		return;
	}
	header->s.is_free = 1;
	global_malloc_lock.unlock();
}

void* my_malloc(size_t size)
{
	size_t total_size;
	void* block;
	header_t* header;
	if (!size)
		return NULL;
	//lock_guard<recursive_mutex> lock(global_malloc_lock);
	global_malloc_lock.lock();
	header = get_free_block(size);
	if (header) {
		/* 找到空闲块 */
		header->s.is_free = 0;
		global_malloc_lock.unlock();
		/*因为header是信息不能被覆盖*/
		return (void*)(header + 1);
	}
	/* 没有找到我们就需要从操作系统申请块*/
	total_size = sizeof(header_t) + size;
	block = sbrk(total_size);
	/*linux分配失败返回-1 但是我的返回0*/
	if (block == (void*)-1||block==nullptr) {
		global_malloc_lock.unlock();
		return NULL;
	}
	header = (header_t*)block;
	header->s.size = size;
	header->s.is_free = 0;
	header->s.next = NULL;
	if (!head)
		head = header;
	if (tail)
		tail->s.next = header;
	tail = header;
	global_malloc_lock.unlock();
	return (void*)(header + 1);
}

void* my_calloc(size_t num, size_t nsize)
{
	size_t size;
	void* block;
	if (!num || !nsize)
		return NULL;
	size = num * nsize;
	/* 检查会不溢出 */
	if (nsize != size / num)
		return NULL;
	block = my_malloc(size);
	if (!block)
		return NULL;
	memset(block, 0, size);
	return block;
}

void* my_realloc(void* block, size_t size)
{
	header_t* header;
	void* ret;
	if (!block || !size)
		return my_malloc(size);
	header = (header_t*)block - 1;
	if (header->s.size >= size)
		return block;
	ret = my_malloc(size);
	if (ret) {
		/* 移到一个更大的新块 */
		memcpy(ret, block, header->s.size);
		/* 释放旧块 */
		my_free(block);
	}
	return ret;
}

/* 测试时用的打印整个链表 */
void print_mem_list()
{
	header_t* curr = head;

	printf("heap= %p, heap_top= %d \n", heap, heap_top);
	printf("head = %p, tail = %p, breakpoint= %p\n", (void*)head, (void*)tail,(void*)sbrk(0));

	while (curr) {
		printf("addr = %p, size = %zu, is_free=%u, next=%p\n",
			(void*)curr, curr->s.size, curr->s.is_free, (void*)curr->s.next);
		curr = curr->s.next;
	}
}

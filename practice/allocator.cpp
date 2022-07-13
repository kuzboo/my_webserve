#include<cstdlib>
#include<stdio.h>
#include<iostream>

class allocator
{
public:
    void*allocate(size_t size);//size_t long unsigned int
    void deallocate(void *node, size_t size);
private:
    struct obj
    {
        struct obj *next;
    };
    obj *freeStore = NULL;
    static int CHUNK;
};
int allocator::CHUNK = 5;
void *allocator::allocate(size_t size)
{
    obj *p;
    if(freeStore==NULL)
    {
        size_t chunk = CHUNK * size;
        freeStore = p = (obj *)malloc(chunk);
        for (int i = 0; i < CHUNK - 1;++i)
        {
            p->next = (obj *)((char *)p + size);
            p = p->next;
        }
        p->next = NULL;
    }
    p = freeStore;
    freeStore = freeStore->next;//在第二个节点
    return p;
}

void allocator::deallocate(void*p,size_t size)
{
    //将p回收到自由链表最前端
    ((obj *)p)->next = freeStore;
    freeStore = (obj *)p;
    free(p);
}

class Foo
{
public:
    Foo(int i) : m_val(i){};
    ~Foo(){};
    char m_c;
    int m_val;
    //double m_val2;

public:
    static allocator myalloc;
    static void* operator new(size_t size)
    {
        return myalloc.allocate(size);
    }
    static void operator delete(void*head,size_t size)
    {
        return myalloc.deallocate(head, size);
    }
};
allocator Foo::myalloc;

int main()
{
    Foo* foo[100];
    std::cout << "Foo:" << sizeof(Foo) << std::endl;
    for (int i = 0; i < 16; ++i)
    {
        foo[i] = new Foo(i);
        std::cout << foo[i] << " " << foo[i]->m_val << std::endl;
    }
    return 0;
}

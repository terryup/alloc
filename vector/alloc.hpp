#ifndef ALLOC_H
#define ALLOC_H

#include <new>
#include <stdlib.h>
#include <mutex>
#include <cstring>

//   定义一个名为 HandlerFunc 的函数指针类型
//   该函数指针指向一个无回返值(void)、无参数列表的函数
typedef void(*HandlerFunc)();

//  一级配置器
class __malloc_alloc_template
{
private:
    //  分配失败时调用的函数，尝试释放一部分内存
    static void *oom_malloc(size_t);
    //  重新分配内存失败时调用的函数，尝试释放一部分
    static void *oom_realloc(void *, size_t);
    //  在程序运行时，当内存申请失败时所调用的回调函数，用于处理内存不足的情况
    static HandlerFunc _handler;

public:

    //  设置内存不足时的处理函数，返回原来的处理函数
    static HandlerFunc set_malloc_handler(HandlerFunc f)
    {
        HandlerFunc old = _handler;
        _handler = f;
        return (old);
    }

    //  申请内存的函数
    static void * allocate(size_t size)
    {
        //  直接申请内存，如果申请失败则调用 oom_malloc 函数
        void *ret = malloc(size);
        if (ret == 0)
        {
            ret = oom_malloc(size);
        }
        return ret;
    }

    //  释放内存的函数
    static void deallocate(void *p)
    {
        //  直接释放内存，对free的封装
        free(p);
    }

    //  重新分配内存的函数
    static void * reallocate(void *p, size_t size_sz)
    {
        //  对realloc的简单封装
        void *ret = realloc(p, size_sz);
        if (ret == 0)
        {
            ret = oom_realloc(p, size_sz);
        }
        return ret;
    }

};

//  分配失败时调用的函数，不断尝试申请内存并释放一部分已有内存，直到申请成功或者失败
void* __malloc_alloc_template::oom_malloc(size_t size)
{
    while(1)
    {
        //  如果没有设置处理函数，则抛出 bad_alloc 异常
        if (_handler == nullptr)
        {
            throw std::bad_alloc();
        }
        //  调用处理函数释放一部分内存
        _handler();

        //  申请内存
        void *ret = malloc(size);
        //  如果申请到了返回内存地址
        if (ret)
        {
            return ret;
        }
    }
}

//  重新分配内存失败时调用的函数，不断尝试申请内存并释放一部分已有内存，直到申请成功或者失败
void *__malloc_alloc_template::oom_realloc(void *p, size_t n)
{
    if (_handler == nullptr)
    {
        throw std::bad_alloc();
    }
    _handler();

    //  重新分配内存
    void *ret = realloc(p, n);
    //  如果申请到了，返回申请到的内存地址
    if (ret)
    {
        return (ret);
    }
}

HandlerFunc __malloc_alloc_template::_handler = nullptr;


class __default_alloc_template
{
private:
    //  自由链表是从8字节开始，以8字节为对齐方式，一直扩充到128
    enum { __ALIGN = 8 };
    //  自由链表的最大结点
    enum { __MAX_BYTES = 128 };
    //  自由链表的个数 = __MAX_BYTES / __ALIGN
    enum { __NFREELISTS = 16 };

    //  自由链表的节点类型
    union _Obj
    {
        //  存储下一个节点的地址
        union  _Obj* _M_free_list_link;
        //  存放用户的数据
        char _M_client_data[1];
    };

    /*
        volatile它告诉编译器不要对该变量进行优化，每次使用该变量都应该从内存中读取，
        而不是使用寄存器中的值。
        由于多个线程可能同时访问这个数组中的指针，
        因此使用volatile可以确保多线程程序中对这些指针的读写操作是同步的。
    */
    //   _free_list表示存储自由链表数组的起始地址
    static _Obj* volatile _free_list[__NFREELISTS];

    //  狭义内存池的开始和结束标志
    static char* _start_free;
    static char* _end_free;
    //  内存池大小
    static size_t _heap_size;

    //  内存池基于freelist实现，需要考虑线程安全，加互斥锁
    static std::mutex _mtx;

public:

    //  默认构造函数，使用noexcept说明不会抛出异常。
    constexpr __default_alloc_template() noexcept {}
    //  拷贝构造函数
    constexpr __default_alloc_template(const __default_alloc_template&) noexcept = default;

private:
    //  将bytes向上调整至8的倍数
    static size_t _round_up(size_t __bytes)
    {
        return (((__bytes)+(size_t)__ALIGN - 1) & ~((size_t)__ALIGN - 1));
    }

    //  获取对应节点的下标
    static size_t _freelist_index(size_t __bytes)
    {
        //  内存大小对齐至 8 的倍数，然后除以 8，得到对应的自由链表下标
        return (((__bytes)+(size_t)__ALIGN - 1) / (size_t)__ALIGN - 1);
    }

    //  将分配好的结点进行连接
    static void* _refill(size_t __n)
    {
        //  每次填充 20 个对象
        int __nobjs = 20;
        //  从内存池中获取一块大内存
        char *__chunk = _chunk_alloc(__n, __nobjs);
        _Obj* volatile* __my_free_list;
        _Obj* __result;
        //  free_list上的指针，指向内存块
        _Obj* __current_obj;
        _Obj* __next_obj;
        int __i;

        //  如果只分配了一个内存块，直接返回该内存块
        if (__nobjs == 1)
        {
            return (__chunk);
        }
        //  获取内存池_free_list中与n对应的空闲链表，确定结点的位置
        __my_free_list = _free_list + _freelist_index(__n);

        //  将第一个内存块作为返回值
        __result = (_Obj*)__chunk;
        //  将剩余的内存块挂到空闲链表上
        *__my_free_list = __next_obj = (_Obj*)(__chunk + __n);
        for(__i = 1;; __i++)
        {
            __current_obj = __next_obj;
            __next_obj = (_Obj*)((char*)__next_obj + __n);
            //  如果是最后一个内存块，将其_M_free_list_link设为0
            if (__nobjs - 1 == __i)
            {
                __current_obj->_M_free_list_link = 0;
                break;
            }
                //  否则将其_M_free_list_link设为下一个内存块的地址
            else
            {
                __current_obj->_M_free_list_link = __next_obj;
            }
        }
        //  返回第一个内存块的地址
        return (__result);
    }

    //  在堆中分配一块大小为 n 的内存，返回起始地址
    //  为了提高效率，每次分配的内存大小为 nobjs * n
    static char* _chunk_alloc(size_t __size, int& __nobjs)
    {
        //  用于保存返回值
        char* __result;
        //  请求分配的总字节数
        size_t __total_bytes = __size * __nobjs;
        //  内存池中剩余的字节数
        size_t __bytes_left = _end_free - _start_free;

        //  内存池中剩余空间足够，直接从内存池中分配
        if (__bytes_left >= __total_bytes)
        {
            //  返回内存池中可用空间的起始地址
            __result = _start_free;
            //  更新内存池起始地址
            _start_free += __total_bytes;
            return(__result);
        }
            //  内存池中剩余空间不足以满足请求，但可以分配至少一个对象
        else if (__bytes_left >= __size)
        {
            //  计算可以分配的对象数
            __nobjs = (int)(__bytes_left / __size);
            //  重新计算分配的总字节数
            __total_bytes = __size * __nobjs;
            //  返回内存池中可用空间的起始地址
            __result = _start_free;
            //  更新内存池起始地址
            _start_free += __total_bytes;
            return(__result);
        }
            //  内存池中剩余空间不足以分配一个对象，需要向系统申请内存
        else
        {
            //  计算需要向系统申请多少字节的内存
            size_t __bytes_to_get = 2 * __total_bytes + _round_up(_heap_size >> 4);
            //  将内存池中剩余空间加入对应的 free list 中，这是对剩余小块内存重新利用
            if (__bytes_left > 0)
            {
                _Obj* volatile* __my_free_list = _free_list + _freelist_index(__bytes_left);

                //  当前自由链表节点指向真正的内存位置
                ((_Obj*)_start_free)->_M_free_list_link = *__my_free_list;
                *__my_free_list = (_Obj*)_start_free;
            }
            //  使用malloc一级空间配置器再次分配内存
            _start_free = (char*)malloc(__bytes_to_get);
            //  系统内存不足，需要重新调整 free list 并重试
            if (_start_free == nullptr)
            {
                size_t __i;
                _Obj* volatile* __my_free_list;
                _Obj* __p;
                for (__i = __size; __i <= (size_t)__MAX_BYTES; __i += (size_t)__ALIGN)
                {
                    __my_free_list = _free_list + _freelist_index(__i);
                    __p = *__my_free_list;
                    //  找到可用的 free list
                    if (__p != 0)
                    {
                        *__my_free_list = __p->_M_free_list_link;
                        _start_free = (char*)__p;
                        _end_free = _start_free + __i;
                        //  重新尝试分配
                        return(_chunk_alloc(__size, __nobjs));
                    }
                }
                //  所有的 free list 中都没有可用内存块，只能使用一级分配器
                _end_free = 0;
                _start_free = (char*)__malloc_alloc_template::allocate(__bytes_to_get);
            }
            _heap_size += __bytes_to_get;
            _end_free = _start_free + __bytes_to_get;
            return(_chunk_alloc(__size, __nobjs));
        }
    }

public:
    //  开辟内存的函数，申请大小为__n的内存空间，返回指向申请内存的指针
    static void* allocate(size_t __n)
    {
        void* __ret = 0;

        //  如果申请的内存空间超过了__MAX_BYTES（128B），使用第一级配置器
        if ((size_t)__MAX_BYTES < __n)
        {
            __ret = __malloc_alloc_template::allocate(__n);
        }
            //  如果申请的内存空间小于等于_MAX_BYTES（128B），使用第二级配置器
        else
        {
            //  找到当前申请内存大小的内存块放置的位置（free_list中的位置）
            //  使用volatile确保每次读取__my_free_list都是从它的原地址中读取，而不是编译器优化后的位置。
            _Obj* volatile* __my_free_list = _free_list + _freelist_index(__n);
            //  使用互斥锁进行保护，防止多个线程同时访问导致的冲突
            std::lock_guard<std::mutex> guard (_mtx);

            _Obj* __result = *__my_free_list;
            //  如果当前位置没有挂载过内存块，调用_S_refill函数重新填充free_list
            if (__result == 0)
            {
                __ret = _refill(_round_up(__n));
                return __ret;
            }
            //  如果当前位置已经挂载了内存块，直接取出内存块，并将free_list上移一位
            else {
                *__my_free_list = __result->_M_free_list_link;
                __ret = __result;
            }
        }
        //  返回指向申请内存的指针
        return __ret;
    }

    //  释放内存
    static void deallocate(void* __p, size_t __n)
    {
        //  判断内存块大小是否大于阈值_MAX_BYTES
        if ((size_t)__MAX_BYTES < __n)
        {
            //  大于阈值，调用一级配置器的deallocate函数释放内存
            __malloc_alloc_template::deallocate(__p);
            return;
        }
            //  小于等于阈值
        else
        {
            //  找到内存块对应的free_list的位置
            //  使用volatile确保每次读取__my_free_list都是从它的原地址中读取，而不是编译器优化后的位置。
            _Obj* volatile* __my_free_list = _free_list + _freelist_index(__n);
            _Obj* __q = (_Obj*)__p;

            //  进入临界区
            std::lock_guard<std::mutex> guard(_mtx);

            //  将释放的内存块链接到对应的free_list上
            __q->_M_free_list_link = *__my_free_list;
            *__my_free_list = __q;
        }
    }

    //  内容扩充&缩容
    //  重新分配内存，将旧内存中的数据拷贝到新的内存中，同时释放旧内存
    static void *reallocate(void* __p, size_t __old_sz, size_t __new_sz)
    {
        void* __result;
        size_t __copy_sz;

        //  如果旧内存和新内存的大小都大于_MAX_BYTES（128），则直接调用reallocate函数进行内存重分配
        if (__old_sz > (size_t)__MAX_BYTES && __new_sz > (size_t)__MAX_BYTES)
        {
            return (__malloc_alloc_template::reallocate(__p, __new_sz));
        }

        //  如果新旧内存的大小相同，则不需要进行内存操作，直接返回旧内存指针
        if (_round_up(__old_sz) == _round_up(__new_sz))
        {
            return (__p);
        }

        //  如果需要进行内存操作，则先调用allocate函数分配新内存，然后将旧内存中的数据拷贝到新内存中
        //  分配新内存
        __result = allocate(__new_sz);
        //  计算拷贝大小，取较小值
        __copy_sz = __new_sz > __old_sz ? __old_sz : __new_sz;
        //  将旧内存中的数据拷贝到新内存中
        std::memcpy(__result, __p, __copy_sz);

        //  释放旧内存
        deallocate(__p, __old_sz);
        //  返回新内存首地址
        return(__result);

    }

};

char* __default_alloc_template::_start_free = nullptr;

char* __default_alloc_template::_end_free = nullptr;

size_t __default_alloc_template::_heap_size = 0;

typename  __default_alloc_template::_Obj* volatile __default_alloc_template::_free_list[__NFREELISTS]{
        nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr
};

std::mutex __default_alloc_template::_mtx;

// 定义符合STL规格的配置器接口, 不管是一级配置器还是二级配置器都是使用这个接口进行分配的
template<class T, class Alloc>
class simple_alloc
{
public:
    static T *allocate(size_t n)
    {
        return 0 == n ? 0 : (T*) Alloc::allocate(n * sizeof (T));
    }

    static T *allocate(void)
    { 
     return (T*) Alloc::allocate(sizeof (T)); 
    }

    static void deallocate(T *p, size_t n)
    { 
        if (0 != n) Alloc::deallocate(p, n * sizeof (T)); 
    }

    static void deallocate(T *p)
    { 
        Alloc::deallocate(p, sizeof (T)); 
    }
    
};

#endif
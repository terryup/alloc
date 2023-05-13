#ifndef ITERATOR_H
#define ITERATOR_H

#include <cstring>          //  memmove
#include <algorithm>        //  copy
#include <stdexcept>        //  throw

#include <utility>          //  pair


//  迭代器所指的内容不能被修改, 只读且只能执行一次读操作    (1~4):p++, ++p, p->
struct input_iterator_tag {};
//  只写并且一次只能执行一次写操作
struct output_iterator_tag {};
//  支持读写操作且支持多次读写操作
struct forward_iterator_tag : public input_iterator_tag {};
//  支持双向的移动且支持多次读写操作
struct bidirectional_iterator_tag : public forward_iterator_tag {};
//  支持双向移动且支持多次读写操作. p+n, p-n等
struct random_access_iterator_tag : public bidirectional_iterator_tag {};

//  这个函数是计算两个迭代器之间的距离的，传给__Diatance去处理,
//  而这里的iterator_category()其实就是能直接萃取出来他是什么类型的迭代器
template <class InputIterator, class Distance>
inline void destory(InputIterator first, InputIterator last, Distance& n)
{
    __destroy(first, last, n, iterator_category(first));
}

//  如果是input_iterator_tag类型的迭代器，也就是只能++的，那就要用while循环一个一个算
template <class InputIterator, class Distance>
inline void __Distance(InputIterator first, InputIterator last, Distance& n, input_iterator_tag)
{
    while(first != last)
    {
        ++first;
        ++n;
    }
}

//  如果是random_access_interator类型的迭代器，那就方便多了，直接后(last) - 前(first)就可以了
template <class RandomAccessIterator, class Distance>
inline void __Distance(RandomAccessIterator first, RandomAccessIterator last, Distance& n, random_access_iterator_tag)
{
    n += last - first;
}

template <class T, class Distance>
struct input_iterator
{
    typedef input_iterator_tag iterator_category;
    typedef T                  value_type;
    typedef Distance           difference_type;
    typedef T*                 pointer;
    typedef T&                 reference;
};

template <class T, class Distance>
struct output_iterator
{
    typedef output_iterator_tag iterator_category;
    typedef T                   value_type;
    typedef Distance            difference_type;
    typedef T*                  pointer;
    typedef T&                  reference;
};

template <class T, class Distance>
struct forward_iterator
{
    typedef forward_iterator_tag iterator_category;
    typedef T                    value_type;
    typedef Distance             difference_type;
    typedef T*                   pointer;
    typedef T&                   reference;
};

template <class T, class Distance>
struct bidirectional_iterator
{
    typedef bidirectional_iterator_tag iterator_category;
    typedef T                          value_type;
    typedef Distance                   difference_type;
    typedef T*                         pointer;
    typedef T&                         reference;
};

template <class T, class Distance>
struct random_access_iterator
{
    typedef random_access_iterator_tag iterator_category;
    typedef T                          value_type;
    typedef Distance                   difference_type;
    typedef T*                         pointer;
    typedef T&                         reference;
};

//  这个地方因为我们上面5个类型的迭代器他的参数类型的明明都是一样的
//  所以这里可以用typename来实现萃取技术，其实简单点理解就是
//  把一个不知名的迭代器当进来通过提取得到他们的类型
template <class Iterator>
struct iterator_traits
{
    typedef typename Iterator::iterator_category iterator_category;
    typedef typename Iterator::value_type        value_type;
    typedef typename Iterator::difference_type   difference_type;
    typedef typename Iterator::pointer           pointer;
    typedef typename Iterator::reference         reference;
};

//  这里分别为原生指针 T* 和 const T* 生成不同的 traits 实现
//  都指定了 iterator_category 为 random_access_iterator_tag，
//  因为原生指针的特性就是可以随机访问内存，因此它们也被视为随机访问迭代器
template <class T>
struct iterator_traits<T*>
{
    typedef random_access_iterator_tag iterator_category;
    typedef T                          value_type;
    typedef ptrdiff_t                  difference_type;
    typedef T*                         pointer;
    typedef T&                         reference;
};
template <class T>
struct iterator_traits<const T*>
{
    typedef random_access_iterator_tag iterator_category;   //  迭代器类型
    typedef T                          value_type;          //  迭代器所指对象的类型
    typedef ptrdiff_t                  difference_type;     //  两个迭代器之间的距离
    typedef const T*                   pointer;             //  迭代器所指对象的类型指针
    typedef const T&                   reference;           //  迭代器所指对象的类型引用
};

//  这个函数就是distance()调用的拿来得到迭代器类型的函数
template <class Iterator>
inline typename iterator_traits<Iterator>::iterator_category iterator_category(const Iterator&)
{
    typedef typename iterator_traits<Iterator>::iterator_category category;
    return category();
}

template <class Iterator>
inline typename iterator_traits<Iterator>::difference_type* difference_type(const Iterator&)
{
    return static_cast<typename iterator_traits<Iterator>::difference_type*>(0);
}

template <class Iterator>
inline typename iterator_traits<Iterator>::value_type* value_type(const Iterator&)
{
    return static_cast<typename iterator_traits<Iterator>::value_type*>(0);
}

////////////////////////////////////////////////////////////////
//  第二个版本的, 接受两个迭代器, 并设法找出元素的类型. 通过__type_trais<> 找出最佳措施
//  这个函数其实就是销毁first到last之间的对象，这里first和last是一个迭代器（可以理解为是一个指针）
//  要摧毁他们两个指针之间所指向的对象，也就是first到last-1的这个距离，不包括last
template <class ForwardIterator>
inline void destroy(ForwardIterator first, ForwardIterator last) 
{
  __destroy(first, last, value_type(first));
}
////////////////////////////////////////////////////////////////

//  这个函数就是上面这个函数调用的，他其实就用到了萃取技术，
//  分析这个迭代器所指向的对象他是POD还是non-trivial
//  分别用__true_type和__false_type
//  POD：可以理解为系统默认的类型，也就是int，char：这种类型不需要我们自己调用析构
//  non-trivial：可以理解为我们自己class出来的类型：需要调用析构
template <class ForwardIterator, class T>
inline void __destroy(ForwardIterator first, ForwardIterator last, T*)
{
    typedef typename __type_traits<T>::has_trivial_destructor trivial_destructor;
    __destroy_aux(first, last, trivial_destructor());
}

//  这里就是识别出来了这个迭代器所指向的类型是non-trivial的类型
//  我们这里就需要一个一个的循环的去调用这些对象的析构函数
template <class ForwardIterator>
inline void __destroy_aux(ForwardIterator first, ForwardIterator last, __false_type) 
{
    for ( ; first < last; ++first)
    {
        /*
            这个位置因为first它就是一个迭代器，其实也可以理解为它就是一个指针，
            而我们在从first到*first的过程就是解引用的过程，而这里因为是获取了对象的本身
            也就是迭代器所指对象的本身，所以解引用操作会返回这个对象，从而触发其析构函数的调用
            当我们通过获取新的指针（即通过&*first获取原指针所指向的对象的指针）来访问对象时，
            编译器会根据对象的类型，决定是否需要调用析构函数。如果对象的类型有析构函数，
            那么获取新的指针时就会调用对象的析构函数，以保证对象的正确销毁。
        */
        destroy(&*first);
    }
}

//  这个函数就是判断出来时POD类型，那就不需要调用析构函数
template <class ForwardIterator> 
inline void __destroy_aux(ForwardIterator, ForwardIterator, __true_type) {}

struct __true_type {};
struct __false_type {};

//  这里将所有的内嵌型别都定义为false_type, 这是对所有的定义最保守的值
//  其实可以加上POD类型，但有点多，没必要都写上来
template <class type>
struct __type_traits { 
   typedef __true_type     this_dummy_member_must_be_first;
   typedef __false_type    has_trivial_default_constructor;
   typedef __false_type    has_trivial_copy_constructor;
   typedef __false_type    has_trivial_assignment_operator;
   typedef __false_type    has_trivial_destructor;
   typedef __false_type    is_POD_type;
};

//  从first到last范围内的元素复制到从 result地址开始的内存
template <class InputIterator, class ForwardIterator>
inline ForwardIterator uninitialized_copy(InputIterator first, InputIterator last, ForwardIterator result)
{
    return __uninitialized_copy(first, last, result, value_type(result));
} 

//  针对const char*和const wchar*单独做了特例化
//  直接调用c++的memmove操作, 毕竟这样的效率更加的高效
inline char* uninitialized_copy(const char* first, const char* last, char* result)
{
    memmove(result, first, last - first);
    return result + (last - first);
}
inline wchar_t* uninitialized_copy(const wchar_t* first, const wchar_t* last, wchar_t* result)
{
    memmove(result, first, sizeof(wchar_t) * (last - first));
    return result + (last - first);
}

//  这个地方如果是POD类型的话那就直接调用std::copy，这样可以优化拷贝速度 
template <class InputIterator, class ForwardIterator>
inline ForwardIterator __uninitialized_copy(InputIterator first, InputIterator last, ForwardIterator result, __true_type)
{
    return std::copy(first, last, result);
}

template <class T1, class T2>
inline void construct(T1* __p, const T2& val)
{
    new (__p) T(val);
}

//  这里如果不是POD类型就比较麻烦了，需要一个一个拷贝过来，由于我们这里把所有类型都设置为了非POD
//  所以都会走到这
template <class InputIterator, class ForwardIterator>
inline ForwardIterator __uninitialized_copy(InputIterator first, InputIterator last, ForwardIterator result, __false_type)
{
    ForwardIterator cur = result;
    try
    {
        //  这里只能循环一个一个拷贝过去范围是[first, last)
        for (; first != last; ++first, ++cur)
        {
            //  在 construct 函数调用的过程中，
            //  对象的内存被合适地初始化，而不是简单地复制原对象的值到新对象
            construct(&*cur, *first);
        }
        //  它的值是输出迭代器范围 [result, result + (last - first)) 的尾后迭代器
        return cur;
    }
    catch(...)
    {
        /*
            这个地方如果在执行构造的过程中发生了异常
            该操作会在异常处理机制下，析构构造成功的对象
            因为 destroy 函数会依次析构从 result 到 cur 这个范围内的对象，
            所以只有构造成功的对象才会被析构，而不会对尚未构造成功的对象执行析构
            这种技巧可以确保已经构造成功的对象不会泄漏，同时也能正确处理构造过程中的异常
        */
        destroy(result, cur);
        throw;
    }
}

//  将指定数量的元素从源区间复制到目标区间。它的参数包括源区间的起始位置 
//  first，需要复制的元素数量 n，以及目标区间的起始位置 result
//  和上一个函数差不多，只不过实现方式不同
template <class InputIterator, class Size, class ForwardIterator>
inline std::pair<InputIterator, ForwardIterator> uninitialized_copy(InputIterator first, Size size, ForwardIterator result)
{
    //  根据iterator_category选择最优函数
    return __uninitialized_copy_n(first, size, result, iterator_category(first));
}

template <class InputIterator, class Size, class ForwardIterator>
inline std::pair<InputIterator, ForwardIterator> __uninitialized_copy_n(InputIterator first, Size size, ForwardIterator result, input_iterator_tag)
{
    ForwardIterator cur = result;
    try
    {
        for (; size > 0; --size, ++first, ++cur)
        {
            construct(&*cur, *first);
        }
        return std::pair<InputIterator, ForwardIterator>(first, cur);
    }
    catch(...)
    {
        destroy(result, cur);
        throw;
    }
}
template <class RandomAccessIterator, class Size, class ForwardIterator>
inline std::pair<RandomAccessIterator, ForwardIterator> __uninitialized_copy_n(RandomAccessIterator first, Size size, ForwardIterator result, random_access_iterator_tag)
{
    RandomAccessIterator last = first + size;
    return std::make_pair(last, uninitialized_copy(first, last, result));
}

//  这个是吧[first,last)的值替换为X
template <class ForwardIterator, class T>
inline void uninitialized_fill(ForwardIterator first, ForwardIterator last, const T& X)
{
    __uninitialized_fill(first, last, X, value_type(first));
}

template <class ForwardIterator, class T1, class T2>
inline void __uninitialized_fill(ForwardIterator first, ForwardIterator last,  const T1& X, T2*)
{
  typedef typename __type_traits<T2>::is_POD_type is_POD;
  __uninitialized_fill_aux(first, last, X, is_POD());      
}

template <class ForwardIterator, class T>
inline void __uninitialized_fill_aux(ForwardIterator first, ForwardIterator last,  const T& X, __true_type)
{
    std::fill(first, last, X);
}
template <class ForwardIterator, class T>
inline void __uninitialized_fill_aux(ForwardIterator first, ForwardIterator last,  const T& X, __false_type)
{
    ForwardIterator cur = first;
    try
    {
        for (; cur != last; ++cur)
        {
            construct(&*cur, X);
        }
    }
    catch(...)
    {
        destroy(result, cur);
        throw;
    }
}

//  从first开始n 个元素填充成 x 值
template <class ForwardIterator, class Size, class T>
inline ForwardIterator uninitialized_fill_n(ForwardIterator first, Size size, const T& X)
{
    return __uninitialized_fill_n(first, size, X, value_type(first));
}

template <class ForwardIterator, class T1, class T2>
inline ForwardIterator __uninitialized_fill_n(ForwardIterator first, ForwardIterator last,  const T1& X, T2*)
{
    typedef typename __type_traits<T2>::is_POD_type is_POD;
    return __uninitialized_fill_n_aux(first, n, X, is_POD())
}

template <class ForwardIterator, class Size, class T>
inline ForwardIterator __uninitialized_fill_n_aux(ForwardIterator first, Size n, const T& X, __true_type)
{
    return std::fill_n(first, n, X);
}
template <class ForwardIterator, class Size, class T>
inline ForwardIterator __uninitialized_fill_n_aux(ForwardIterator first, Size n, const T& X, __false_type)
{
    ForwardIterator cur = first;
    try
    {
        for (; n > 0; --n, ++cur)
        {
            construct(&*cur, x);
        }
        return cur;
    }
    catch(...)
    {
        destroy(result, cur);
        throw;
    }
    
}



#endif
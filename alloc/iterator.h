#ifndef ITERATOR_H
#define ITERATOR_H

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


#endif
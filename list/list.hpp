#ifndef LIST_HPP
#define LIST_HPP

#include "iterator.hpp"
#include "alloc.hpp"

//  __list_node用来实现节点, 数据结构中就储存前后指针和属性
template <class T>
struct __list_node
{
    //  前后指针
    typedef void* void_pointer;
    void_pointer next;
    void_pointer prev;
    //  属性
    T data;
};


template <class T, class Ref, class Ptr>
struct __list_iterator
{
    typedef __list_iterator<T, T&, T*>                iterator;
    typedef __list_iterator<T, const T&, const T*>    const_iterator;
    typedef __list_iterator<T, Ref, Ptr>              self;

    //  迭代器是bidirectional_iterator_tag类型
    typedef bidirectional_iterator_tag iterator_category;
    typedef T                          value_type;
    typedef Ptr                        pointer;
    typedef Ref                        reference;
    typedef size_t                     size_type;
    typedef ptrdiff_t                  difference_type;

    //  定义节点指针
    typedef __list_node<T>*  link_type;
    link_type _node;

    //  构造函数
    __list_iterator(link_type x) : _node(x) {}
    __list_iterator() {}
    __list_iterator(const iterator& x) : _node(x._node) {}

    //  定义重载
    bool operator==(const self& x) const
    {
        return _node == x._node;
    }

    bool operator!=(const self& x) const
    {
        return _node != x._node;
    }

    reference operator*() const
    {
        return (*_node).data;
    }

    pointer operator->() const
    {
        return &(operator*());
    }

    //  ++和--是直接操作的指针指向next还是prev, 因为list是一个双向链表
    self& operator++()
    {
        _node = (link_type)((*_node).next);
        return *this;
    }
    self& operator++(int)
    {
        self tmp = *this;
        ++*this;
        return tmp;
    }

    self& operator--()
    {
        _node = (link_type)((*_node).prev);
        return *this;
    }
    self& operator--(int)
    {
        self tmp = *this;
        --*this;
        return tmp;
    }

};


template<class T, class Alloc = __default_alloc_template>
class list
{
protected:
    //  list在定义node节点时, 定义的不是一个指针
    typedef void*                  void_pointer;
    typedef __list_node<T>         list_node;   //  节点
    typedef simple_alloc<list_node, Alloc> list_node_allocator; //  空间配置器

public:
    //  定义嵌套类型
    typedef T                   value_type;
    typedef value_type*         pointer;
    typedef const value_type*   const_pointer;
    typedef value_type&         referece;
    typedef const value_type&   const_referece;
    typedef list_node*          link_type;
    typedef size_t              size_type;
    typedef ptrdiff_t           difference_type;

protected:
    //  定义一个节点, 这里节点并不是一个指针.
    link_type node;

public:
    //  定义迭代器
    typedef __list_iterator<T, T&, T*>                iterator;
    typedef __list_iterator<T, const T&, const T*>    const_iterator;

protected:
    //  构造函数前期准备
    //  分配一个元素大小的空间, 返回分配的地址
    link_type get_node()
    {
        return list_node_allocator::allocate();
    }

    //  释放一个元素大小的内存
    void put_node(link_type p)
    {
        list_node_allocator::deallocate(p);
    }

    //  分配一个元素大小的空间并调用构造初始化内存
    link_type create_node(const T& x)
    {
        link_type p = get_node();
        try
        {
            construct(&p->data, x);
        }
        catch(...)
        {
            put_node(p);
        }
        return p;
    }

    //  调用析构并释放一个元素大小的空间
    void destroy_node(link_type p)
    {
        destroy(&p->data);
        put_node(p);
    }

    //  对节点初始化
    void empty_initialize()
    {
        node = get_node();
        node->next = node;
        node->prev = node;
    }

    //  list在初始化的时候都会构造一个空的node节点, 然后对元素进行insert插入操作
    void fill_initialize(size_type n, const T& value)
    {
        empty_initialize();
        try
        {
            insert(begin(), n, value);
        }
        catch(...)
        {
            clear();
            put_node(node);
        }
    }

    //  接受一个迭代器范围 [first, last)，并将范围内的元素逐个添加到 list 中
    void range_initialize(iterator first, iterator last)
    {
        empty_initialize();
        while (first != last)
        {
            //  后面会实现
            push_back(*first);
            ++first;
        }
    }

    void range_initialize(const_iterator first, const_iterator last)
    {
        empty_initialize();
        while (first != last)
        {
            //  后面会实现
            push_back(*first);
            ++first;
        }
    }


public:
    //  默认构造函数, 分配一个空的node节点
    list()
    {
        empty_initialize();
    }

    //  都调用同一个函数进行初始化
    list(size_type n, const T& value)
    {
        fill_initialize(n, value);
    }
    list(int n, const T& value)
    {
        fill_initialize(n, value);
    }
    list(long n, const T& value)
    {
        fill_initialize(n, value);
    }

    //  分配n个节点
    explicit list(size_type n)
    {
        fill_initialize(n, T());
    }

    //  接受两个迭代器进行范围的初始化
    list(iterator first, iterator last)
    {
        range_initialize(first, last);
    }
    list(const list<T, Alloc>& x)
    {
        range_initialize(x.begin(), x.end());
    }


    //  释放所有的节点空间. 包括最初的空节点
    ~list()
    {
        clear();
        put_node(node);
    }

public:
    //  基本属性获取
    iterator begin()
    {
        return (link_type)((*node).next);
    }

    const_iterator begin() const
    {
        return (link_type)((*node).next);
    }

    iterator end()
    {
        return node;
    }
    const_iterator end() const
    {
        return node;
    }

    bool empty() const
    {
        return node->next == node;
    }

    size_type size() const
    {
        size_type result = 0;
        distance(begin(), end(), result);
        return result;
    }

    size_type max_size() const
    {
        return size_type(-1);
    }

    referece front()
    {
        return *begin();
    }

    const_referece front() const
    {
        return *begin();
    }

    referece back()
    {
        return *(--end());
    }

    const_referece back() const
    {
        return *(--end());
    }

    void swap(list<T, Alloc>& x)
    {
        std::swap(node, x.node);
    }

public:
    //  push和pop还有erase以及clear
    void push_front(const T& x)
    {
        insert(begin(), x);
    }
    void push_back(const T& x)
    {
        insert(end(), x);
    }

    void pop_front()
    {
        erase(begin());
    }
    void pop_back()
    {
        iterator tmp = end();
        erase(--tmp);
    }

    iterator erase(iterator position)
    {
        link_type next_node = link_type(position._node->next);
        link_type prev_node = link_type(position._node->prev);

        prev_node->next = next_node;
        next_node->prev = prev_node;

        destroy_node(position._node);
        return iterator(next_node);
    }

    iterator erase(iterator first, iterator last)
    {
        while(first != last)
        {
            erase(first++);
        }
        return last;
    }

    void remove(const T& value)
    {
        iterator first = begin();
        iterator last = end();

        while(first != last)
        {
            iterator next = first;
            ++next;
            if(*first == value)
            {
                erase(first);
            }
            first = next;
        }
    }

    void clear()
    {
        link_type cur = (link_type) node->next;
        //  除空节点都删除
        while(cur != node)
        {
            link_type tmp = cur;
            cur = (link_type)cur->next;
            destroy_node(tmp);
        }
        node->next = node;
        node->prev = node;
    }

public:
    list<T, Alloc> operator=(const list<T, Alloc>& x)
    {
        if (this != &x)
        {
            iterator first1 = begin();
            iterator last1 = end();
            const_iterator first2 = x.begin();
            const_iterator last2 = x.end();
            while (first1 != last1 && first2 != last2)
            {
                *first1++ = *first2++;
            }
            if (first2 == last2)
            {
                erase(first1, last1);
            }
            else
            {
                insert(last1, first2, last2);
            }
        }
        return *this;
    }

public:
    //  resize重新修改list的大小
    void resize(size_type new_size)
    {
        resize(new_size, T());
    }
    void resize(size_type new_size, const T& x)
    {
        iterator i = begin();
        size_type len = 0;
        for (; i != end() && len < new_size; ++i, ++len)
            ;
        if (len == new_size)
        {
            erase(i, end());
        }
        else
        {
            insert(end(), new_size - len, x);
        }
    }

    //  unique函数是将数值相同且连续的元素删除, 只保留一个副本
    template <class BinaryPredicate>
    void unique(BinaryPredicate binary_pred)
    {
        iterator first = begin();
        iterator last = end();

        if (first == last)
        {
            return;
        }
        iterator next = first;
        while(++next != last)
        {
            if(binary_pred(*first, *next))
            {
                erase(next);
            }
            else
            {
                first = next;
            }
            next = first;
        }
    }

    iterator insert(iterator position, const T& x)
    {
        //  将元素插入指定位置的前一个地址
        link_type tmp = create_node(x);
        tmp->next = position._node;
        tmp->prev = position._node->prev;
        (link_type(position._node->prev))->next = tmp;
        position._node->prev = tmp;
        return tmp;
    }

    //  以下重载函数都是调用iterator insert(iterator position, const T& x)函数
    iterator insert(iterator position)
    {
        insert(position, T());
    }
    template <class InputIterator>
    void insert(iterator position, InputIterator first, InputIterator last)
    {
        for(; first != last; ++first)
        {
            insert(position, *first);
        }
    }
    void insert(iterator position, const_iterator first, const_iterator last)
    {
        for(; first != last; ++first)
        {
            insert(position, *first);
        }
    }
    void insert(iterator position, size_type n, const T& x)
    {
        for(; n > 0; --n)
        {
            insert(position, x);
        }
    }

public:
    //  transfer函数功能是将一段链表插入到我们指定的位置之前
    //  transfer函数接受3个迭代器. 第一个迭代器表示链表要插入的位置, first到last最闭右开区间插入到position之前
    void transfer(iterator position, iterator first, iterator last)
    {
        if (position != last)
        {
            (*(link_type)((*last._node).prev)).next = position._node;
            (*position._node).prev = (*last._node).prev;
            link_type tmp = link_type((*position._node).prev);
            (*first._node).prev = tmp;
            (*(link_type)((*position._node).prev)).next = first._node;
            (*(link_type)((*first._node).prev)).next = last._node;
            (*last._node).prev = (*first._node).prev; 
        }
    }

    //  splice 将两个链表进行合并
    void splic(iterator position, list& x)
    {
        if (!x.empty())
        {
            transfer(position, x.begin(), x.end());
        }
    }
    void splic(iterator position, list&, iterator i)
    {
        iterator j = i;
        ++j;
        if (position == i || position == j)
        {
            return;
        }
        transfer(position, i, j);
    }
    void splic(iterator position, list&, iterator first, iterator last)
    {
        if (first != last)
        {
            transfer(position, first, last);
        }
    }

    //  将传入的list链表x与原链表按从小到大合并到原链表中(前提是两个链表都是已经从小到大排序了)(sort)
    void merge(list& x)
    {
        iterator first1 = begin();
        iterator last1 = end();
        iterator first2 = x.begin();
        iterator last2 = x.end();

        while (first1 != last1 && first2 && last2)
        {
            if (*first2 < *first1)
            {
                iterator next = first2;
                //  将first2到first+1的左闭右开区间插入到first1的前面
                //  这就是将first2合并到first1链表中
                transfer(first1, first2, ++next);
                first2 = next;
            }
            else
            {
                ++first1;
            }
        }
        if (first2 != last2)
        {
            //  如果链表x还有元素则全部插入到first1链表的尾部
            transfer(last1, first2, last2);
        }
        
    }

    //  reverse函数是实现将链表翻转的功能
    void reverse()
    {
        if (node->next == node || link_type(node->next)->next == node)
        {
            return;
        }
        iterator first = begin();
        ++first;
        while (first != end())
        {
            iterator old = first;
            ++first;
            //  将元素插入到begin()之前
            transfer(begin(), old, first);
        }
    }

};

//  这个函数是给下面operator<调用的，这只是自己写的一种实现方法
//  用于比较两个范围的元素，它按字典顺序逐个比较元素，并返回比较结果
template <class InputIterator1, class InputIterator2>
inline bool lexicographical_compare(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2)
{
    while (first1 != last1)
    {
        if (first2 == last2 || *first2 < *first1)
        {
            return false;
        }
        if (*first1 < *first2)
        {
            return true;
        }
        ++first1;
        ++first2;
    }
    return (first2 != last2);
}

//  这里是重载运算符（==，< , =）
template <class T, class Alloc>
inline bool operator==(const list<T,Alloc>& x, const list<T,Alloc>& y)
{
    typedef typename list<T,Alloc>::link_type link_type;
    link_type e1 = x.node;
    link_type e2 = y.node;
    link_type n1 = (link_type) e1->next;
    link_type n2 = (link_type) e2->next;
    //  将两个链表执行一一的对比来分析是否相等
    //  这里不把元素个数进行一次比较, 主要获取个数时也要遍历整个数组, 所以就不将个数纳入比较
    for (; n1 != e1 && n2 != e2; n1 = (link_type) n1->next, n2 = (link_type) n2->next)
    {
        if (n1->data != n2->data)
        {
            return false;
        }
    }
    return n1 == e1 && n2 == e2;
}

//  这里的比较是假设已经排好序  并且长度相等的两个链表
template <class T, class Alloc>
inline bool operator<(const list<T, Alloc>& x, const list<T, Alloc>& y)
{
    return lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
}


#endif
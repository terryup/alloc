#ifndef VECTOR_H
#define VECTOR_H


#include "alloc.hpp"
#include "iterator.hpp"

#include <utility>


template <class T, class Alloc = __default_alloc_template>
class vector{
public:
    //  定义vector自身的嵌套级别
    typedef T                  value_type;
    typedef value_type*        pointer;
    typedef const value_type*  const_pointer;
    //  定义迭代器, 这里就只是一个普通的指针
    typedef value_type*        iterator;
    typedef const value_type*  const_iterator;
    typedef value_type&        reference;
    typedef size_t             size_type;
    typedef ptrdiff_t          difference_type;
    typedef const value_type&  const_reference;

protected:
     typedef simple_alloc<T, Alloc> data_allocator; //  设置其空间配置器
    iterator start;                                         //  使用空间的头
    iterator finish;                                        //  使用空间的尾
    iterator end_of_storage;                                //  可用空间的尾

public:
    //  初始化vector的使用空间头和空间的尾
    //  这两个函数主要是为了下面的构造函数初始化准备的
    void fill_initialize(size_type n, const T& value)
    {
        start = allocate_and_fill(n, value);
        finish = start + n;
        end_of_storage = finish;
    }
    //  调用默认的第二配置器分配内存, 分配失败就释放所分配的内存
    iterator allocate_and_fill(size_type n, const T& X)
    {
        //  申请n个元素的线性空间.
        iterator result = data_allocator::allocate(n);
        //  对整个线性空间进行初始化, 如果有一个失败则删除全部空间并抛出异常
        try
        {
            uninitialized_fill_n(result, n, X);
            return result;
        }
        catch(...)
        {
            data_allocator::deallocate(result, n);
        }
    }

    //  vector有多个构造函数, 为了满足多种初始化
    //  默认构造函数
    //  类似平时使用时：vector<int> vec;
    vector() : start(0), finish(0), end_of_storage(0) {}
    //  必须显示的调用这个构造函数, 接受一个值
    //  类似平时使用时：vector<int> vec(5);  -> 构造一个大小为5默认值为0的vector
    explicit vector(size_type n) { fill_initialize(n, T()); }
    //  接受一个大小和初始化值. int和long都执行相同的函数初始化
    //  类似平时使用时：vector<int> vec(3, 7); -> 构造一个大小为3值都为7的 vector
    vector(size_type n, const T& value) { fill_initialize(n, value); }
    vector(int n, const T& value) { fill_initialize(n, value); }
    vector(long n, const T& value) { fill_initialize(n, value); }
    //  接受一个vector参数的构造函数
    //  类似平时使用时：
    //  vector<int> vec1(5, 7); -> 创建一个包含 5 个值为 7 的元素的 vector
    //  vector<int> vec2(vec1); -> 通过拷贝 vec1 来初始化 vec2，两个 vector 的元素完全相同
    //  这里调用的是uninitialized_copy执行初始化
    vector(const vector<T, Alloc>& x)
    {
        start = allocate_and_copy(x.end() - x.begin(), x.begin(), x.end());
        finish = start + (x.end() - x.begin());	// 初始化头和尾迭代器位置
        end_of_storage = finish;
    }
    //  同样进行初始化
    template <class ForwardIterator> 
    iterator allocate_and_copy(size_type n, ForwardIterator first, ForwardIterator last)
    {
        iterator result = data_allocator::allocate(n);
        try
        {
            uninitialized_copy(first, last, result);
            return result;
        }
        catch(...)
        {
            data_allocator::deallocate(result, n);
        } 
    } 
    //  支持两个迭代器表示范围的复制
    iterator allocate_and_copy(size_type n, const_iterator first, const_iterator last) 
    {
        iterator result = data_allocator::allocate(n);
        try
        {
            uninitialized_copy(first, last, result);
            return result;
        }
        catch(...)
        {
            data_allocator::deallocate(result, n);
        }
    }

    //  析构函数就是直接调用deallocate 空间配置器, 从头释放到数据尾部, 最后将内存还给空间配置器
    //  vector因为是类, 所以我们并不需要手动的释放内存, 
    //  生命周期结束后就自动调用析构从而释放调用空间, 当然我们也可以直接调用析构函数释放内存
    void deallocate()
    {
        if (start)
        {
            data_allocator::deallocate(start, end_of_storage - start);
        }
    }
    ~vector()
    {
        destroy(start, finish);
        deallocate();
    }

public:
    //  这里就是位置信息的获取，还有一些属性信息的获取
    //  获取数据的开始以及结束位置的指针. 记住这里返回的是迭代器, 也就是vector迭代器就是该类型的指针
    iterator begin() { return start; }
    iterator end() { return finish; }

    //  获取值
    reference front() { return *begin(); }
    reference back() { return *(end() - 1); }

    //  获取右值
    const_iterator begin() const { return start; }
    const_iterator end() const { return finish; }
    const_reference front() const {return *begin(); }
    const_reference back() const { return *(end() - 1); }

    //  获取基本数组信息
    //  数组元素的个数
    size_type size() const { return size_type(end() - begin()); }
    //  最大能存储的元素个数
    size_type max_size() const { return size_type(-1) / sizeof(T); }
    //  数组的实际大小
    size_type capacity() const { return size_type(end_of_storage - begin()); }

    //  判断vector是否为空, 并不是比较元素为0, 是直接比较头尾指针
    bool empty() const { return begin() == end(); }

public:
    //  vector的实现
    //  push_back从尾部插入数据. 当数组还有备用空间的时候就直接插入尾部就行了, 
    //  当没有备用空间后就重新寻找更大的空间再将数据全部复制过去
    void push_back(const T& x)
    {
        //  数组的备用空间还足够那就直接插入尾部就行了
        if (finish != end_of_storage)
        {
            construct(finish, x);
            ++finish;
        }
        //  数组被填充满, 调用insert_aux必须重新寻找新的更大的连续空间, 再进行插入
        else
        {
            insert_aux(end(), x);
        }
    }

    //  pop_back从尾部进行删除
    void pop_back()
    {
        --finish;
        destroy(finish);
    }

    //  清除指定位置的元素. 实际就是将指定位置后面的所有元素向前移动, 最后析构掉最后一个元素
    iterator erase(iterator position)
    {
        if (position + 1 != end())
        {
            std::copy(position + 1, finish, position);
        }
        --finish;
        destroy(finish);
        return position;
    }

    //  清除一个指定范围的元素, 同样将指定范围后面的所有元素向前移动, 最后析构掉整个范围的元素
    //  清除的是左闭右开的区间 [ )
    iterator erase(iterator first, iterator last)
    {
        iterator i = std::copy(last, finish, first);
        destroy(i, finish);
        finish = finish - (last - finish);
        return first;
    }

    //  clear清除所有数据
    void clear()
    {
        erase(begin(), end());
    }

public:
    //  vector的重载运算符
    reference operator[](size_type n)
    {
        return *(begin() + n);
    }

    const_reference operator[](size_type n) const
    {
        return *(begin() + n);
    }

    vector<T, Alloc>& operator=(const vector<T, Alloc>& x)
    {
        if (&x != this)
        {
            //  判断x的数据大小跟赋值的数组大小
            //  数组大小过小
            if (x.size() > capacity())
            {
                //  进行范围的复制, 并销毁掉原始的数据.
                iterator tmp = allocate_and_copy(x.end() - begin(), x.begin(), x.end());
                destroy(start, finish);
                deallocate();
                //  修改偏移
                start = tmp;
                end_of_storage = start + (x.end() - x.begin());
            }
            //  数组的元素大小够大, 直接将赋值的数据内容拷贝到新数组中. 并将后面的元素析构掉
            else if (size() >= x.size())
            {
                iterator i = std::copy(x.begin(), x.end(), begin());
                destroy(i, finish);
            }
            //  数组的元素大小不够, 装不完x的数据, 但是数组本身的大小够大
            else
            {
                //  先将x的元素填满原数据大小
                std::copy(x.begin(), x.begin() + size(), start);
                //  再将x后面的数据全部填充到后面
                uninitialized_copy(x.begin() + size(), x.end(), finish);
            }
            finish = start + x.size();
        }
        return *this;
    }

public:
    // 修改容器的实际的大小
    void reserve(size_type n)
    {
        //  修改的容器大小要大于原始数组大小才行
        if (capacity() < n)
        {
            const size_type old_size = size();
            //  重新拷贝数据, 并将原来的空间释放掉
            iterator tmp = allocate_and_copy(n, start, finish);
            destory(start, finish);
            deallocate();
            //  重新修改3个迭代器位置
            start = tmp;
            finish = tmp + old_size;
            end_of_storage = start + n;
        }
    }

    //  resize重新修改数组元素的容量. 这里是修改容纳元素的大小, 不是数组的大小.
    void resize(size_type new_size)
    {
        resize(new_size, T());
    }
    void resize(size_type new_size, const T& x)
    {
        //  元素大小大于了要修改的大小, 则释放掉超过的元素
        if (new_size < size())
        {
            erase(begin() + new_size, end());
        }
        //  元素不够, 就从end开始到要求的大小为止都初始化x
        else
        {
            insert(end(), new_size - size(), x);
        }
    }

    //  vector实现swap就只是将3个迭代器进行交换即可, 并不用将整个数组进行交换
    void swap(vector<T, Alloc>& x)
    {
        //  交换指针，即交换 start, finish, end_of_storage
        std::swap(start, x.start);
        std::swap(finish, x.finish);
        std::swap(end_of_storage, x.end_of_storage);
    }
    
public:
    //  下面是插入的实现
    iterator insert(iterator position)
    {
        return insert(position, T());
    }
    iterator insert(iterator position, const T& x)
    {
        size_type n = position - begin();
        //  如果数组还有备用空间, 并且插入的是finish位置, 直接插入即可, 最后调整finish就行了
        if (finish != end_of_storage && position == end())
        {
            construct(finish, x);
            ++finish;
        }
        //  以上条件不成立, 调用另一个函数执行插入操作
        else{
            insert_aux(position, x);
        }
        return begin() + n;
    }

    /*
        1、如果数组还有备用空间, 就直接移动元素, 再将元素插入过去, 最后调整finish就行了
        2、没有备用空间, 重新申请空间原始空间的两倍+1的空间后, 再将元素拷贝过去同时执行插入操作
        3、析构调用原始空间元素以及释放空间, 最后修改3个迭代器的指向
    */
    void insert_aux(iterator position, const T& x)
    {
        //  如果数组还有备用空间, 就直接移动元素, 再将元素插入过去, 最后调整finish就行了
        if (finish != end_of_storage)
        {
            //  调用构造, 并将最后一个元素复制过去, 调整finish
            construct(finish, *(finish - 1));
            ++finish;
            T x_copy = x;
            //  将插入元素位置的后面所有元素往后移动, 最后元素插入到位置上
            std::copy_backward(position, finish - 2, finish - 1);
            *position = x_copy;
        }
        //  没有备用空间, 重新申请空间再将元素拷贝过去同时执行插入操作
        else
        {
            const size_type old_size = size();
            //  重新申请空间原始空间的两倍+1的空间
            const size_type len = old_size != 0 ? 2 * old_size : 1;
            
            iterator new_start = data_allocator::allocate(len);
            iterator new_finish = new_start;
            try
            {
                //  进行分段将原始元素拷贝新的空间中, 这样也就实现了插入操作
                new_finish = uninitialized_copy(start, position, new_start);
                construct(new_finish, x);
                ++new_finish;
                new_finish = uninitialized_copy(position, finish, new_finish);
            }
            catch(...)
            {
                destroy(new_start, new_finish);
                data_allocator::deallocate(new_start, len);
                throw;
            }
            //  释放掉原来的空间, 调整新的3个迭代器的位置
            destroy(begin(), end());
            deallocate();
            start = new_start;
            finish = new_finish;
            end_of_storage = new_start + len;
        }
    }

    //  传入一个迭代器, 插入的个数, 和插入的值
    void insert(iterator pos, int n, const T& x)
    {
        insert(pos, (size_type)n, x);
    }
    void insert(iterator pos, long n, const T& x)
    {
        insert(pos, (size_type)n, x);
    }

    /*
    如果备用空间足够大, 先保存插入位置到end的距离:
        从插入的位置到end的距离大于要插入的个数n:
            先构造出finish-n个大小的空间, 再移动finish - n个元素的数据；
            在将从插入位置后的n个元素移动；
            最后元素从插入位置开始进行填充；
        从插入的位置到end的距离小于了要插入的个数:
            先构造出n - elems_after个大小的空间, 再从finish位置初始化n - elems_after为x；
            从插入位置开始到原来的finish位置结束全部复制到新的结束位置后面；
            从插入位置进行填充x；
        上面分成两种情况就只是为了插入后的元素移动方便. 其实我自己考虑过为什么直接分配出n个空间, 
        然后从position+n处开始把原数据依次拷贝再将数据插入过去就行了, 没有必要分成两种情况, 可能这里面有什么我没有考虑到的因素吧.
    备用空间不足的时候执行:
        重新申请一个当前两倍的空间或者当前大小+插入的空间, 选择两者最大的方案；
        进行分段复制到新的数组中, 从而实现插入；
        将当前数组的元素进行析构, 最后释放空间；
        修改3个迭代器；
    */
    void insert(iterator position, size_type n, const T& x)
    {
        if (n != 0)
        {
            //  备用空间足够大
            if (size_type(end_of_storage - finish) >= n)
            {
                T x_copy = x;
                //  保存插入位置到end的距离
                const size_type elems_after = finish - position;
                iterator old_finish = finish;

                //  插入的位置到数据结束的距离大于了要插入的个数n
                if (elems_after > 0)
                {
                    //  先构造出finish-n个大小的空间, 再移动finish - n个元素的数据
                    uninitialized_copy(finish - n, finish, finish);
                    finish += n;
                    //  在将从插入位置后的n个元素移动
                    std::copy_backward(position, old_finish - n, old_finish);
                    //  元素从插入位置开始进行填充即可
                    fill(position, position + n, x_copy);
                }
                //  从插入的位置到end的距离小于了要插入的个数
                else
                {
                    //  先构造出n - elems_after个大小的空间, 再从finish位置初始化n - elems_after为x
                    uninitialized_fill_n(finish, n - elems_after, x_copy);
                    finish += n - elems_after;
                    //  从插入位置开始到原来的finish位置结束全部复制到新的结束位置后面
                    uninitialized_copy(position, old_finish, finish);
                    finish += elems_after;
                    //  从插入位置进行填充x
                    fill(position, old_finish, x_copy);
                }
            }
            //  空间不足处理
            else
            {
                //  重新申请一个当前两倍的空间或者当前大小+插入的空间, 选择两者最大的方案.
                const size_type old_size = size();
                const size_type len = old_size + std::max(old_size, n);
                iterator new_start = data_allocator::allocate(len);
                iterator new_finish = new_start;
                try
                {
                    //  同样进行分段复制到新的数组中, 从而实现插入
                    new_finish = uninitialized_copy(start, position, new_start);
	        	    new_finish = uninitialized_fill_n(new_finish, n, x);
	        	    new_finish = uninitialized_copy(position, finish, new_finish);
                }
                catch(...)
                {
                    destory(new_start, new_finish);
                    data_allocator::deallocate(new_start, len);
                    throw;
                }
                //  将当前数组的元素进行析构, 最后释放空间
                destroy(start, finish);
                deallocate();
                //  修改3个迭代器
                start = new_start;
      		    finish = new_finish;
      		    end_of_storage = new_start + len;
            }
        }
    }

    void insert(iterator position, const_iterator first, const_iterator last)
    {
        if (first != last)
        {
            size_type n = 0;
            distance(first, last, n);
            if (size_type(end_of_storage - finish) >= n)
            {
                const size_type elems_after = finish - position;
                iterator old_finish = finish;
                if (elems_after > n)
                {
                    uninitialized_copy(finish - n, finish, finish);
                    finish += n;
                    copy_backward(position, old_finish - n, old_finish);
                    std::copy(first, last, position);
                }
                else
                {
                    uninitialized_copy(first + elems_after, last, finish);
                    finish += n - elems_after;
                    uninitialized_copy(position, old_finish, finish);
                    finish += elems_after;
                    copy(first, first + elems_after, position);
                }
            }
            else
            {
                const size_type old_size = size();
                const size_type len = old_size + std::max(old_size, n);
                iterator new_start = data_allocator::allocate(len);
                iterator new_finish = new_start;
                try
                {
                    new_finish = uninitialized_copy(start, position, new_start);
                    new_finish = uninitialized_copy(first, last, new_finish);
                    new_finish = uninitialized_copy(position, finish, new_finish);
                }
                catch(...)
                {
                    destroy(new_start, new_finish);
                    data_allocator::deallocate(new_start, len);
                    throw;
                }
                destroy(start, finish);
                deallocate();
                start = new_start;
                finish = new_finish;
                end_of_storage = new_start + len;
            }
        }
    }

};




#endif
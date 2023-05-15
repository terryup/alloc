#include "list.hpp"

#include <iostream>

#include <list>

int main()
{
    list<int>ls;
    list<int> List;
	List.push_back(1);
	List.push_front(0);
	List.push_back(2);
	List.push_front(3);
	List.insert(++List.begin(), 5);	// list没有重载 + 运算符, 所以不能直接操作迭代器
	
	list<int> List1(List);
	list<int> List2 = List1;
	std::cout << "list size = " << List2.size() << std::endl;
	if(!List2.empty())
		for (auto i : List2)
		{
			std::cout << i << " ";
		}
	std::cout << std::endl;

    std::list<int> LS;

}

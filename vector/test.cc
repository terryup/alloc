#include "vector.hpp"

#include <iostream>

int main()
{
    vector<int> vec;
    vector<int> v1;
	vector<int> v2(4);
	vector<int> v3(4, 1);


	v1.push_back(1);
	v1.push_back(2);
	v1.push_back(3);
	v1.push_back(4);
	v1.push_back(5);
	
    //这里清除的是一个[v1.begin()+2, v1.end()-1) , 左闭右开的区间
	v1.erase(v1.begin()+2, v1.end()-1);
	v1.pop_back();
	
	v2 = v1;
	
	if(!v3.empty())	// 不为空
	{
		std::cout << *v3.begin() << " " << v3.front() << " " << *(v3.end() - 1) << " " << v3.back() <<  std::endl;
		std::cout << "size = " << v3.size() << std::endl;
        v3.~vector();
	}

	if(!v1.empty())	// 不为空
	{
		std::cout << *v1.begin() << " " << v1.front() << " " << *(v1.end() - 1) << " " << v1.back() <<  std::endl;
		std::cout << "size = " << v1.size() << std::endl;
        v1.~vector();
	}

	

}
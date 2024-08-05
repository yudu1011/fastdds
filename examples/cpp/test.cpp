#include <memory>
#include <string>
#include <iostream>
#include <cstdlib>

class MyClass {
public:
    std::string value;
    MyClass(const std::string& val) : value(val) {
      std::cout << "构造函数" << std::endl;
    }
    ~MyClass(){
      std::cout << "析构函数" << std::endl;
    }
};

int main() {
    // 使用 make_shared 创建 MyClass 对象的 shared_ptr
    auto myPtr = std::make_shared<MyClass>("Hello World");

    // 使用智能指针指向的对象
    std::cout << myPtr->value << std::endl;

    return 0;
}
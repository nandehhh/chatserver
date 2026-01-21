#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std;

//test.cpp
#include <iostream>  
#include <boost/lexical_cast.hpp>  

int main() {  
    int number = 123;  
    std::string str = boost::lexical_cast<std::string>(number);  
    std::cout << "Number as string: " << str << std::endl;  
    return 0;  
}




void func1()
{
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhangsan";
    js["to"] = "li si";
    js["msg"] = "how are you";
    std::cout<<js<<std::endl;
    std::string sendBuf = js.dump();//json转成字符串，然后转成c字符发送出去
    std::cout<<sendBuf.c_str()<<std::endl;

}
void func2()
{
    json js;
    // 添加数组
    js["id"] = {1,2,3,4,5};
    // 添加key-value
    js["name"] = "zhang san";
    // 添加对象
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["liu shuo"] = "hello china";
    // 上面等同于下面这句一次性添加数组对象
    js["msg"] = {{"zhang san", "hello world"}, {"liu shuo", "hello china"}};
    std::cout << js << std::endl;
}
void func3()
{
    json js;
    // 直接序列化一个vector容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);
    js["list"] = vec;
    // 直接序列化一个map容器
    map<int, string> m;
    m.insert({1, "黄山"});
    m.insert({2, "华山"});
    m.insert({3, "泰山"});
    js["path"] = m;
    cout<<js<<endl;
}
// int main()
// {
//     func3();
//     return 0;
// }
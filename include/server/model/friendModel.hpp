#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H
#include <vector>
#include "user.hpp"
using namespace std;
//维护好友信息的数据处理接口
class friendModel
{
public:
//添加好友
void insert(int userid,int friendid);

//返回用户的好友列表,以及好友的具体信息,所以涉及到联合查询
vector<User> query(int userid);


};


#endif
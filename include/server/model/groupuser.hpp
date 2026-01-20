#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

//群组用户，多了一个role角色，从user类直接继承，复用user的其他信息
class GroupUser:public User
{
private:
    string role;
public:
    void setRole(string role){this->role = role;}
    string getrole() {return this->role;}
};


#endif
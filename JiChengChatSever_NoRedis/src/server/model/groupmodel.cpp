#include "groupmodel.hpp"
#include "db.h"

bool GroupModel::createGroup(Group& group)
{
    //1组装sql语句
    char sql[1024] = {0};
    sprintf(sql,"insert into allgroup(groupname,groupdesc) values('%s','%s')",group.getName().c_str(),group.getDesc().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if(mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    return false;
}

//加入群组
void GroupModel::addGroup(int userid,int groupid,string role)
{
    //1组装sql语句
    char sql[1024] = {0};
    sprintf(sql,"insert into groupuser values(%d,%d,'%s')",groupid,userid,role.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
//查询用户所在群组信息
vector<Group> GroupModel::queryGroup(int userid)
{
    //根据userid在groupuser表中查询该用户所属的群组消息
    //再根据群组信息，找出属于该群组的所有用户的userid，并且和user表进行多表联合查询
    char sql[1024] = {0};
    sprintf(sql,"select a.id,a.groupname,a.groupdesc from allgroup a inner join groupuser b on b.groupid = a.id where b.userid = %d",userid);
    vector<Group> GroupVec;//存放用户所在的群组

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);

                GroupVec.push_back(group);
            }
        }
    }
    //查询群组的用户信息
    for (Group& group: GroupVec)
    {
        sprintf(sql,"select a.id, a.name, a.state,b.grouprole from  user a inner join groupuser b on b.userid = a.id where b.groupid = %d",group.getId());
        vector<GroupUser> GroupuserVec;

        MySQL mysql;
        if (mysql.connect())
        {
            MYSQL_RES* res = mysql.query(sql);
            if(res != nullptr)
            {
               //把userid用户的所有离线消息放入vec中返回
               MYSQL_ROW row;
               while((row = mysql_fetch_row(res)) != nullptr)
                {
                    GroupUser groupuser;
                    groupuser.setId(atoi(row[0]));
                    groupuser.setName(row[1]);
                    groupuser.setState(row[2]);
                    groupuser.setRole(row[3]);
                    group.getUsers().push_back(groupuser);
               }
               mysql_free_result(res);
            }
        }



    }
    return GroupVec;
    
}
//根据指定的groupid查询群组用户id列表，除userid自己，主要用户群聊业务给群组其他人员群发消息
vector<int> GroupModel::queryGroupUsers(int userid,int groupid){
char sql[1024] = {0};
    sprintf(sql,"select userid from groupuser where groupid = %d and userid != %d",groupid,userid);
    vector<int> Vec;

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                Vec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return Vec;
}
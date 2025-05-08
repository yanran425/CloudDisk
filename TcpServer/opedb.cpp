#include "opedb.h"
#include"QMessageBox"
#include"QDebug"
#include"QString"

OpeDB::OpeDB(QObject *parent) : QObject(parent)
{
    // 添加数据库
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    // 初始化数据库
    init();
}

OpeDB::~OpeDB()
{
    // 关闭数据库
    m_db.close();
}

OpeDB &OpeDB::getInstance()
{
    static OpeDB instance;
    return instance;
}

void OpeDB::init()
{
    m_db.setHostName("localhost");
    /*
     * :代表读取资源文件下的内容
     * ./ 代表读取可执行程序同目录下的内容
     * */
    m_db.setDatabaseName("./cloud.db");
    if(m_db.open())
    {
        QSqlQuery query;
        query.exec("select * from userInfo");
        while(query.next())
        {
            QString data = QString("%1,%2,%3").arg(query.value(0).toString())
                    .arg(query.value(1).toString())
                    .arg(query.value(2).toString());
            qDebug() << data;
            // 服务端重启后所有用户应该是离线状态
            handleOffine(query.value(1).toString().toStdString().c_str());
        }
    }
    else
    {
        QMessageBox::critical(NULL, "打开数据库", "打开数据库失败");
    }
}

bool OpeDB::handleRegist(const char *name, const char *pwd)
{
    // 非空校验
    if(NULL == name || NULL == pwd)
    {
        return false;
    }
    QString strSql = QString("insert into userInfo(name, pwd) values('%1','%2')").arg(name).arg(pwd);
    QSqlQuery query;
    // 返回sql插入结果
    return query.exec(strSql);
}

bool OpeDB::handleLogin(const char *name, const char *pwd)
{
    // 非空校验
    if(NULL == name || NULL == pwd)
    {
        return false;
    }
    QString strSql = QString("select * from userInfo where name = '%1' and pwd = '%2' and online = 0").arg(name).arg(pwd);
    QSqlQuery query;
    query.exec(strSql);
    // 判断是否查询到了数据
    if(query.next())
    {
        // 如果查询到了数据，则更新用户的登录状态
        strSql = QString("update userInfo set online = 1 where name = '%1'").arg(name);
        query.exec(strSql);
        return true;
    }
    else
    {
        return false;
    }
}

void OpeDB::handleOffine(const char *name)
{
    // 非空校验
    if(NULL == name)
    {
        return;
    }
    QString strSql = QString("update userInfo set online = 0 where name = '%1'").arg(name);
    QSqlQuery query;
    // 执行更新操作
    query.exec(strSql);
}

QStringList OpeDB::handleAllOnline()
{
    QString strSql = QString("select name from userInfo where online = 1");
    QSqlQuery query;
    // 执行操作
    query.exec(strSql);
    QStringList result;
    result.clear();
    while(query.next())
    {
        result.append(query.value(0).toString());
    }
    return result;
}

int OpeDB::handleSearchUser(const char *name)
{
    // 用户不存在
    if(NULL == name)
    {
        return -1;
    }
    QString strSql = QString("select online from userInfo where name = '%1'").arg(name);
    QSqlQuery query;
    // 执行操作
    query.exec(strSql);
    // 如果查询到结果了的话
    if(query.next())
    {
        // 返回用户在线结果
        return query.value(0).toInt();
    }
    // 用户不存在
    else
    {
        return -1;
    }
}

int OpeDB::handleAddfriendCheck(const char *friendName, const char *loginName)
{
    if(NULL == friendName || NULL == loginName)
    {
        // 输入内容格式错误
        return -1;
    }
    QString strSql = QString("select id from userInfo where name = '%1'").arg(friendName);
    QSqlQuery query;
    query.exec(strSql);
    if(query.next())
    {
        int friendId = query.value(0).toInt();
        strSql = QString("select id from userInfo where name = '%1'").arg(loginName);
        query.exec(strSql);
        query.next();
        int loginId = query.value(0).toInt();
        strSql = QString("select * from friend where id = '%1' and friendId = '%2'").arg(loginId).arg(friendId);
        query.exec(strSql);
        if(query.next())
        {
            // 如果已经是好友了
            return 0;
        }
        else
        {
            strSql = QString("select online from userInfo where id = '%1'").arg(friendId);
            query.exec(strSql);
            query.next();
            int online = query.value(0).toInt();
            if(online == 1)
            {
                // 对方在线中
                return 1;
            }
            else if(online == 0)
            {
                // 如果对方离线中
                return 2;
            }
        }
    }
    else
    {
        // 用户不存在
        return 3;
    }

}

void OpeDB::handleAddfriend(const char *friendName, const char *loginName)
{
    // 获取好友id
    QString strSql = QString("select id from userInfo where name = '%1'").arg(friendName);
    QSqlQuery query;
    query.exec(strSql);
    query.next();
    int friendId = query.value(0).toInt();
    // 获取登录用户id
    strSql = QString("select id from userInfo where name = '%1'").arg(loginName);
    query.exec(strSql);
    query.next();
    int loginId = query.value(0).toInt();
    // 新增好友
    strSql = QString("insert into friend (id,friendId) values ('%1','%2'), ('%2','%1')").arg(friendId).arg(loginId);
    query.exec(strSql);
}

QStringList OpeDB::handleFlushFriend(const char *name)
{
    QStringList strNameList;
    strNameList.clear();

    if(NULL == name)
    {
        return strNameList;
    }

    QString strSql = QString("select id from userInfo where name = '%1'").arg(name);
    QSqlQuery query;
    query.exec(strSql);
    query.next();
    int loginId = query.value(0).toInt();

    strSql = QString("select friendId from friend where id = '%1'").arg(loginId);
    query.exec(strSql);
    while(query.next())
    {
        int friendId = query.value(0).toInt();
        strSql = QString("select name from userInfo where id = '%1' and online = 1").arg(friendId);
        QSqlQuery nameQuery;
        nameQuery.exec(strSql);
        if(nameQuery.next())
        {
            strNameList.append(nameQuery.value(0).toString());
        }
    }
    return strNameList;
}

void OpeDB::handleDeletefriend(const char *friendName, const char *loginName)
{
    if(NULL == friendName || NULL == loginName)
    {
        return ;
    }
    QString strSql = QString("select id from userInfo where name = '%1'").arg(friendName);
    QSqlQuery query;
    query.exec(strSql);
    query.next();
    int friendId = query.value(0).toInt();
    strSql = QString("select id from userInfo where name = '%1'").arg(loginName);
    query.exec(strSql);
    query.next();
    int userId = query.value(0).toInt();
    strSql = QString("delete from friend where (id = '%1' and friendId = '%2') or (id = '%2' and friendId = '%1')").arg(userId).arg(friendId);
    query.exec(strSql);
}

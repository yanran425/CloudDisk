#include "mytcpsocket.h"
#include"QDebug"
#include"mytcpserver.h"
#include<QDir>
#include<QFileInfoList>

MyTcpSocket::MyTcpSocket()
{
    m_pTimer = new QTimer;
    m_bUpload = false;//定义最开始不是上传文件的状态
    // 定义接受到数据之后的操作
    connect(this, SIGNAL(readyRead()), this, SLOT(recvMsg()));
    // 定义客户端退出链接
    connect(this, SIGNAL(disconnected()), this, SLOT(clientOffline()));
    connect(m_pTimer, SIGNAL(timeout()),
            this, SLOT(sendFileDataToClient()));
}

QString MyTcpSocket::getStrName()
{
    return m_strName;
}

void MyTcpSocket::recvMsg()
{
    // 如果是上传文件
    if(m_bUpload)
    {
        // 上传的文件过大可能会导致客户端崩溃
        // 这里需要readAll多次，因为可能文件还没有发送完成
        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;
        // 写入文件
        QByteArray buff = readAll();
        m_file.write(buff);
        m_iRecved += buff.size();

        qDebug() << QString("第 %1 次传入文件,接受数据大小:%2").arg(m_iCount).arg(buff.size());
        m_iCount++;
        // 读取文件内容完成，向客户端发送信息上传成功
        if(m_iTotal == m_iRecved)
        {
            m_bUpload = false;
            m_file.close();
            strcpy(pdu->caData, UPLOAD_FILE_OK);
            write((char*)pdu, pdu->uiPDULen);
            free(pdu);
            pdu = NULL;
        }
        // 读取数据错误的情况下不再读取了，向客户端发送信息提示结束上传
        else if(m_iTotal < m_iRecved)
        {
            m_bUpload = false;
            m_file.close();
            strcpy(pdu->caData, UPLOAD_FILE_FAIL);
            write((char*)pdu, pdu->uiPDULen);
            free(pdu);
            pdu = NULL;
        }
        // 其他情况代表文件上传未完成，等待结束数据
        return ;
    }
    //通信对象的总大小
    uint uiPDULen = 0;
    // 先获取通信对象的总大小
    this->read((char*)&uiPDULen, sizeof(int));
    // 消息数据大小
    uint uiMsgLen = uiPDULen - sizeof(PDU);
    // 创建通信对象空间
    PDU *pdu = mkPDU(uiMsgLen);
    // 更新通信对象总大小
    pdu->uiPDULen = uiPDULen;
    // 读取剩余数据内容到通信对象中
    this->read((char*)pdu + sizeof(int), uiPDULen - sizeof(int));
    switch(pdu->uiMsgType)
    {
    case ENUM_MSG_TYPE_REGIST_REQUEST:
    {
        regist(pdu);
        break;
    }
    case ENUM_MSG_TYPE_LOGIN_REQUEST:
    {
        login(pdu);
        break;
    }
    case ENUM_MSG_TYPE_ALL_ONLINE_REQUEST:
    {
        allOnline(pdu);
        break;
    }
    case ENUM_MSG_TYPE_SEARCH_USER_REQUEST:
    {
        searchUser(pdu);
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:
    {
        addFriend(pdu);
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_AGREE:{
        addFriendAgree(pdu);
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE:{
        addFriendRefuse(pdu);
        break;
    }
    case ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST:{
        flushFriend(pdu);
        break;
    }
    case ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST:{
        deleteFriend(pdu);
        break;
    }
    case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:{
        privateChat(pdu);
        break;
    }
    case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:
    {
        groupChat(pdu);
        break;
    }
    case ENUM_MSG_TYPE_CREATE_DIR_REQUEST:
    {
        createDir(pdu);
        break;
    }
    case ENUM_MSG_TYPE_FLUSH_DIR_REQUEST:
    {
        flushDir(pdu);
        break;
    }
    case ENUM_MSG_TYPE_DEL_DIR_REQUEST:
    {
        delDir(pdu);
        break;
    }
    case ENUM_MSG_TYPE_RENAME_DIR_REQUEST:
    {
        renameDir(pdu);
        break;
    }
    case ENUM_MSG_TYPE_ENTER_DIR_REQUEST:
    {
        enterDir(pdu);
        break;
    }
    case ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST:
    {
        uploadFile(pdu);
        break;
    }
    case ENUM_MSG_TYPE_DEL_FILE_REQUEST:
    {
        delFile(pdu);
        break;
    }
    case ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST:
    {
        downloadFile(pdu);
        break;
    }
    case ENUM_MSG_TYPE_SHARE_FILE_REQUEST:
    {
        shareFile(pdu);
        break;
    }
    case ENUM_MSG_TYPE_SHARE_FILE_NOTE_REQUEST:
    {
        shareFileNote(pdu);
        break;
    }
    case ENUM_MSG_TYPE_MOVE_FILE_REQUEST:
    {
        moveFile(pdu);
        break;
    }
    default:
        break;
    }
    // 释放内存
    free(pdu);
    pdu = NULL;
}

void MyTcpSocket::clientOffline()
{
    // 更新数据库状态
    OpeDB::getInstance().handleOffine(m_strName.toStdString().c_str());
    // 发出退出链接信号
    emit offline(this);
}

void MyTcpSocket::sendFileDataToClient()
{
    char *pBuffer = new char[4096];
    qint64 ret = 0;
    while(true)
    {
        ret = m_file.read(pBuffer, 4096);
        if(ret > 0 && ret <= 4096)
        {
            write(pBuffer, ret);
        }
        // 文件内容读取完成
        else if(0 == ret)
        {
            m_file.close();
            break;
        }
        // 文件读取出错
        else
        {
            m_file.close();
            // 实际开发应该记录到日志文件中
            qDebug() << "发送文件失败";
            break;
        }
    }
    delete []pBuffer;
    pBuffer = NULL;
}

void MyTcpSocket::regist(PDU *pdu)
{
    // 获取客户端传输过来的账号密码
    char caName[32] = {'\0'};
    char caPwd[32] = {'\0'};
    // 前32位是账号，后32位是密码
    strncpy(caName, pdu->caData, 32);
    strncpy(caPwd, pdu->caData + 32, 32);
    // 插入数据库
    bool ret = OpeDB::getInstance().handleRegist(caName, caPwd);
    // 创建回复消息通信对象
    PDU *retPdu = mkPDU(0);
    // 设置本次通信对象的消息类型
    retPdu->uiMsgType = ENUM_MSG_TYPE_REGIST_RESPOND;
    // 设置回复消息内容
    if(ret)
    {
        strcpy(retPdu->caData, REGIST_OK);
        // 给新用户创建文件夹
        QDir dir;
        ret = dir.mkdir(QString("./%1").arg(caName));
    }
    else
    {
        strcpy(retPdu->caData, REGIST_FAIL);
    }
    // 发送通信对象内容给客户端
    this->write((char *)retPdu, retPdu->uiPDULen);
    // 释放内存
    free(retPdu);
    retPdu = NULL;
}

void MyTcpSocket::login(PDU *pdu)
{
    // 获取客户端传输过来的账号密码
    char caName[32] = {'\0'};
    char caPwd[32] = {'\0'};
    // 前32位是账号，后32位是密码
    strncpy(caName, pdu->caData, 32);
    strncpy(caPwd, pdu->caData + 32, 32);
    // 插入数据库
    bool ret = OpeDB::getInstance().handleLogin(caName, caPwd);
    // 创建回复消息通信对象
    PDU *retPdu = mkPDU(0);
    // 设置本次通信对象的消息类型
    retPdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_RESPOND;
    // 设置回复消息内容
    if(ret)
    {
        strcpy(retPdu->caData, LOGIN_OK);
        m_strName = caName;
    }
    else
    {
        strcpy(retPdu->caData, LOGIN_FAIL);
    }
    // 发送通信对象内容给客户端
    this->write((char *)retPdu, retPdu->uiPDULen);
    // 释放内存
    free(retPdu);
    retPdu = NULL;
}

void MyTcpSocket::allOnline(PDU *pdu)
{
    QStringList ret = OpeDB::getInstance().handleAllOnline();
    // 所有姓名的大小【32个字节】
    uint uiMsgLen = ret.size() * 32;
    PDU *retPdu = mkPDU(uiMsgLen);
    retPdu->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINE_RESPOND;
    for(int i = 0; i < ret.size(); i++)
    {
        memcpy((char *)(retPdu->caMsg) + i*32
               , ret.at(i).toStdString().c_str()
               , ret.at(i).size());
    }
    // 发送通信对象内容给客户端
    this->write((char *)retPdu, retPdu->uiPDULen);
    // 释放内存
    free(retPdu);
    retPdu = NULL;
}

void MyTcpSocket::searchUser(PDU *pdu)
{
    int ret = OpeDB::getInstance().handleSearchUser(pdu->caData);
    PDU *retPdu = mkPDU(0);
    retPdu->uiMsgType = ENUM_MSG_TYPE_SEARCH_USER_RESPOND;
    if(-1 == ret)
    {
        strcpy(retPdu->caData, SEARCH_USER_NO);
    }
    else if(1 == ret)
    {
        strcpy(retPdu->caData, SEARCH_USER_ONLINE);
    }
    else if(0 == ret)
    {
        strcpy(retPdu->caData, SEARCH_USER_OFFLINE);
    }
    // 发送通信对象内容给客户端
    this->write((char *)retPdu, retPdu->uiPDULen);
    // 释放内存
    free(retPdu);
    retPdu = NULL;
}

void MyTcpSocket::addFriend(PDU *pdu)
{
    // 获取客户端传输过来的好友名称
    char caFriendName[32] = {'\0'};
    char caLoginName[32] = {'\0'};
    // 前32位是好友名称，后32位是登录者名称
    strncpy(caFriendName, pdu->caData, 32);
    strncpy(caLoginName, pdu->caData + 32, 32);
    // 添加好友判断
    int ret = OpeDB::getInstance().handleAddfriendCheck(caFriendName, caLoginName);
    // 未添加好友且好友在线
    if(ret == 1)
    {
        MyTcpServer::getInstance().resend(caFriendName, pdu);
    }
    else
    {
        PDU *retPdu = mkPDU(0);
        retPdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
        // 好友名称为NULL
        if(ret == -1)
        {
            strcpy(retPdu->caData, UNKNOWN_ERROR);
        }
        // 好友已存在
        else if(ret == 0)
        {
            strcpy(retPdu->caData, ADD_FRIEND_EXISTS);
        }
        // 好友不存在，但该用户离线
        else if(ret == 2)
        {
            strcpy(retPdu->caData, ADD_FRIEND_OFFLINE);
        }
        // 好友名称错误
        else if(ret == 3)
        {
            strcpy(retPdu->caData, ADD_FRIEND_NOT_EXISTS);
        }
        this->write((char *)retPdu, pdu->uiPDULen);
        free(retPdu);
        retPdu = NULL;
    }
}

void MyTcpSocket::addFriendAgree(PDU *pdu)
{
    // 获取客户端传输过来的好友名称
    char caFriendName[32] = {'\0'};
    char caLoginName[32] = {'\0'};
    // 前32位是好友名称，后32位是登录者名称
    strncpy(caFriendName, pdu->caData, 32);
    strncpy(caLoginName, pdu->caData + 32, 32);
    // 添加好友
    OpeDB::getInstance().handleAddfriend(caFriendName, caLoginName);
    // 转发消息，添加好友成功
    MyTcpServer::getInstance().resend(caLoginName, pdu);
}

void MyTcpSocket::addFriendRefuse(PDU *pdu)
{
    // 获取客户端传输过来的好友名称
    char caLoginName[32] = {'\0'};
    // 前32位是好友名称，后32位是登录者名称
    strncpy(caLoginName, pdu->caData + 32, 32);
    // 转发消息，申请加好友失败
    MyTcpServer::getInstance().resend(caLoginName, pdu);
}

void MyTcpSocket::flushFriend(PDU *pdu)
{
    // 获取客户端传输过来的好友名称
    char caLoginName[32] = {'\0'};
    strncpy(caLoginName, pdu->caData, 32);
    QStringList ret = OpeDB::getInstance().handleFlushFriend(caLoginName);
    uint uiMsgLen = ret.size() * 32;
    PDU *retPdu = mkPDU(uiMsgLen);
    retPdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND;
    for(int i = 0; i < ret.size(); i++)
    {
        // 记住，这里放的应该的char*，而不是int[], 因为是要复制char*的内容过来
        memcpy((char*)(retPdu->caMsg) + i * 32
               , ret.at(i).toStdString().c_str()
               , ret.at(i).size());
    }
    write((char*)retPdu, retPdu->uiPDULen);
    free(retPdu);
    retPdu = NULL;
}

void MyTcpSocket::deleteFriend(PDU *pdu)
{
    // 获取客户端传输过来的好友名称
    char caFriendName[32] = {'\0'};
    char caLoginName[32] = {'\0'};
    // 前32位是好友名称，后32位是登录者名称
    strncpy(caFriendName, pdu->caData, 32);
    strncpy(caLoginName, pdu->caData + 32, 32);
    // 删除好友
    OpeDB::getInstance().handleDeletefriend(caFriendName, caLoginName);
    // 通知删除成功
    PDU *retPdu = mkPDU(0);
    retPdu->uiMsgType = ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND;
    strcpy(retPdu->caData, DELETE_FRIEND_OK);
    write((char*)retPdu, retPdu->uiPDULen);
    free(retPdu);
    retPdu = NULL;
    // 转发消息给被删除的好友
    MyTcpServer::getInstance().resend(caFriendName, pdu);
}

void MyTcpSocket::privateChat(PDU *pdu)
{
    // 获取客户端传输过来的好友名称
    char caLoginName[32] = {'\0'};
    strncpy(caLoginName, pdu->caData, 32);
    MyTcpServer::getInstance().resend(caLoginName, pdu);
}

void MyTcpSocket::groupChat(PDU *pdu)
{
    QStringList allUser = OpeDB::getInstance().handleAllOnline();
    for(int i = 0; i < allUser.size(); i++)
    {
        MyTcpServer::getInstance().resend(allUser.at(i).toStdString().c_str(), pdu);
    }
}

void MyTcpSocket::createDir(PDU *pdu)
{
    PDU *retPdu = mkPDU(0);
    retPdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
    QString strCurPath = QString("%1").arg((char*)pdu->caMsg);
    QDir dir;
    bool ret = dir.exists(strCurPath);
    // 如果当前文件夹不存在
    if(!ret)
    {
        strcpy(retPdu->caData, CUR_DIR_NOT_EXIST);
        write((char*)retPdu, retPdu->uiPDULen);
        free(retPdu);
        retPdu = NULL;
        return;
    }
    // 获取客户端传输过来的名称
    char caLoginName[32] = {'\0'};
    char caDirName[32] = {'\0'};
    // 前32位是登录者名称，后32位是新增文件夹名称
    strncpy(caLoginName, pdu->caData, 32);
    strncpy(caDirName, pdu->caData + 32, 32);
    QString strNewPath = strCurPath + "/" + caDirName;
    ret = dir.exists(strNewPath);
    // 如果要新建的文件夹已经存在了
    if(ret)
    {
        strcpy(retPdu->caData, FILE_NAME_EXIST);
        write((char*)retPdu, retPdu->uiPDULen);
        free(retPdu);
        retPdu = NULL;
        return;
    }
    ret = dir.mkdir(strNewPath);
    // 如果单纯的创建文件夹失败了
    if(!ret)
    {
        strcpy(retPdu->caData, CREATE_DIR_ERROR);
        write((char*)retPdu, retPdu->uiPDULen);
        free(retPdu);
        retPdu = NULL;
        return ;
    }
    strcpy(retPdu->caData, CREATE_DIR_OK);
    write((char*)retPdu, retPdu->uiPDULen);
    free(retPdu);
    retPdu = NULL;
}

void MyTcpSocket::flushDir(PDU *pdu)
{
    char *pCurPath = new char[pdu->uiMsgLen];
    memcpy(pCurPath, (char*)pdu->caMsg, pdu->uiMsgLen);
    PDU *retPdu = getDirFilePDU(pCurPath);
    retPdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_DIR_RESPOND;
    write((char*)retPdu, retPdu->uiPDULen);
    free(retPdu);
    retPdu = NULL;
}

void MyTcpSocket::delDir(PDU *pdu)
{
    char caName[32] = {'\0'};
    strcpy(caName, pdu->caData);
    char *curPath = new char[pdu->uiMsgLen];
    memcpy(curPath, pdu->caMsg, pdu->uiMsgLen);
    QString strPath = QString("%1/%2").arg(curPath).arg(caName);
    // 文件对象
    QFileInfo fileInfo(strPath);
    PDU *retPdu = NULL;
    // 如果不是文件夹的话
    if(!fileInfo.isDir())
    {
        retPdu = mkPDU(strlen(DEL_DIR_TYPE_ERROR) + 1);
        retPdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPOND;
        memcpy(retPdu->caData, DEL_DIR_TYPE_ERROR, strlen(DEL_DIR_TYPE_ERROR));
        write((char*)retPdu, retPdu->uiPDULen);
        free(retPdu);
        retPdu = NULL;
        return ;
    }
    QDir dir;
    dir.setPath(strPath);
    // 删文件包括其中的内容
    bool ret = dir.removeRecursively();
    // 如果删除失败
    if(!ret)
    {
        retPdu = mkPDU(sizeof(DEL_DIR_SYSTEM_ERROR) + 1);
        memcpy(retPdu->caData, DEL_DIR_SYSTEM_ERROR, strlen(DEL_DIR_SYSTEM_ERROR));
    }
    else
    {
        retPdu = mkPDU(sizeof(DEL_DIR_OK) + 1);
        memcpy(retPdu->caData, DEL_DIR_OK, strlen(DEL_DIR_OK));
    }
    retPdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPOND;
    write((char*)retPdu, retPdu->uiPDULen);
    free(retPdu);
    retPdu = NULL;
}

void MyTcpSocket::renameDir(PDU *pdu)
{
    char caOldName[32] = {'\0'};
    char caNewName[32] = {'\0'};
    strncpy(caOldName, pdu->caData, 32);
    strncpy(caNewName, pdu->caData + 32, 32);

    char *curPath = new char[pdu->uiMsgType];
    memcpy(curPath, (char*)pdu->caMsg, pdu->uiMsgLen);
    QString strOldPath = QString("%1/%2").arg(curPath).arg(caOldName);
    QString strNewPath = QString("%1/%2").arg(curPath).arg(caNewName);
    QDir dir;
    bool ret = dir.rename(strOldPath, strNewPath);
    PDU *retPdu = mkPDU(0);
    retPdu->uiMsgType = ENUM_MSG_TYPE_RENAME_DIR_RESPOND;
    // 如果重命名成功
    if(ret)
    {
        strcpy(retPdu->caData, RENAME_FILE_OK);
    }
    else
    {
        strcpy(retPdu->caData, RENAME_FILE_FIAL);
    }
    write((char*)retPdu, retPdu->uiPDULen);
    free(retPdu);
    retPdu = NULL;
}

void MyTcpSocket::enterDir(PDU *pdu)
{
    char caEnterName[32] = {'\0'};
    strncpy(caEnterName, pdu->caData, 32);

    char *caCurPath = new char[pdu->uiMsgLen];
    memcpy(caCurPath, (char*)pdu->caMsg, pdu->uiMsgLen);

    QString strPath = QString("%1/%2").arg(caCurPath).arg(caEnterName);
    QFileInfo fileInfo(strPath);
    if(fileInfo.isFile())
    {
        PDU *retPdu = mkPDU(0);
        retPdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_RESPOND;
        strcpy(retPdu->caData, ENTER_DIR_FAIL);
        write((char*)retPdu, retPdu->uiPDULen);
        free(retPdu);
        retPdu = NULL;
    }
    else if(fileInfo.isDir())
    {
        PDU *retPdu = getDirFilePDU(strPath);
        retPdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_RESPOND;
        strcpy(retPdu->caData, ENTER_DIR_OK);
        write((char*)retPdu, retPdu->uiPDULen);
        free(retPdu);
        retPdu = NULL;
    }
}

PDU * MyTcpSocket::getDirFilePDU(QString curPath)
{
    QDir dir(curPath);
    QFileInfoList fileInfoList = dir.entryInfoList();
    int iFileCount = fileInfoList.size();
    PDU *retPdu = mkPDU(sizeof(FileInfo) * iFileCount);
    FileInfo *pFileInfo = NULL;
    for(int i = 0; i < iFileCount; i++)
    {
        // 获取返回PDU中第i块FileInfo的内存区域，向里面塞入数据
        pFileInfo = (FileInfo *)retPdu->caMsg + i;
        strcpy(pFileInfo->caFileName, fileInfoList[i].fileName().toStdString().c_str());
        if(fileInfoList[i].isDir())
        {
            pFileInfo->iFileType = 0;
        }
        else if(fileInfoList[i].isFile())
        {
            pFileInfo->iFileType = 1;
        }
        else
        {
            pFileInfo->iFileType = 2;
        }
    }
    return retPdu;
}

void MyTcpSocket::uploadFile(PDU *pdu)
{
    char caFileName[32] = {'\0'};
    qint64 fileSize = 0;
    sscanf(pdu->caData, "%s %lld", caFileName, &fileSize);
    char *curPath = new char[pdu->uiMsgLen];
    memcpy(curPath, (char*)pdu->caMsg, pdu->uiMsgLen);
    QString strPath = QString("%1/%2").arg(curPath).arg(caFileName);
    delete []curPath;
    curPath = NULL;
    m_file.setFileName(strPath);
    // 以只写的方式打开文件，如果文件不存在，则自动创建文件
    if(m_file.open(QIODevice::WriteOnly))
    {
        m_bUpload = true;
        m_iTotal = fileSize;
        m_iRecved = 0;
        m_iCount = 1;
    }

}

void MyTcpSocket::delFile(PDU *pdu)
{
    char caFileName[32] = {'\0'};
    strcpy(caFileName, pdu->caData);
    char *curPath = new char[pdu->uiMsgLen];
    memcpy(curPath, pdu->caMsg, pdu->uiMsgLen);
    QString strPath = QString("%1/%2").arg(curPath).arg(caFileName);
    delete []curPath;
    curPath = NULL;
    // 文件对象
    QFileInfo fileInfo(strPath);
    PDU *retPdu = NULL;
    // 如果不是文件夹的话
    if(fileInfo.isDir())
    {
        retPdu = mkPDU(strlen(DEL_FILE_TYPE_ERROR) + 1);
        retPdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_RESPOND;
        memcpy(retPdu->caData, DEL_FILE_TYPE_ERROR, strlen(DEL_FILE_TYPE_ERROR));
        write((char*)retPdu, retPdu->uiPDULen);
        free(retPdu);
        retPdu = NULL;
        return ;
    }
    QDir dir;
    // 删除文件目录中的该文件
    bool ret = dir.remove(strPath);
    // 如果删除失败
    if(!ret)
    {
        retPdu = mkPDU(sizeof(DEL_FILE_SYSTEM_ERROR) + 1);
        memcpy(retPdu->caData, DEL_FILE_SYSTEM_ERROR, strlen(DEL_FILE_SYSTEM_ERROR));
    }
    else
    {
        retPdu = mkPDU(sizeof(DEL_FILE_OK) + 1);
        memcpy(retPdu->caData, DEL_FILE_OK, strlen(DEL_FILE_OK));
    }
    retPdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_RESPOND;
    write((char*)retPdu, retPdu->uiPDULen);
    free(retPdu);
    retPdu = NULL;
}

void MyTcpSocket::downloadFile(PDU *pdu)
{
    char caFileName[32] = {'\0'};
    strcpy(caFileName, pdu->caData);
    char *curPath = new char[pdu->uiMsgLen];
    memcpy(curPath, pdu->caMsg, pdu->uiMsgLen);
    QString strPath = QString("%1/%2").arg(curPath).arg(caFileName);
    delete []curPath;
    curPath = NULL;
    QFileInfo fileInfo(strPath);
    qint64 fileSize = fileInfo.size();
    PDU *retPdu = mkPDU(0);
    retPdu->uiMsgType = ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND;
    sprintf(retPdu->caData, "%s %lld", caFileName, fileSize);
    write((char*)retPdu, retPdu->uiPDULen);
    free(retPdu);
    retPdu = NULL;
    // 借用 m_file对象进行下载文件，因为目前是单线程阻塞的，所以不影响
    m_file.setFileName(strPath);
    m_file.open(QIODevice::ReadOnly);
    // 定时器设置1s后发送数据
    m_pTimer->start(1000);
}

void MyTcpSocket::shareFile(PDU *pdu)
{
    char caSendName[32] = {'\0'};
    int num = 0;
    sscanf(pdu->caData, "%s %d", caSendName, &num);
    int size = num * 32;
    PDU *retPdu = mkPDU(pdu->uiMsgLen - size);//下载文件的路径的大小
    retPdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_NOTE;
    strcpy(retPdu->caData, caSendName);
    memcpy((char*)retPdu->caMsg, (char*)(pdu->caMsg) + size, pdu->uiMsgLen - size);
    char caRecvName[32] = {'\0'};
    for(int i = 0; i < num ; i++)
    {
        memcpy(caRecvName, (char*)(pdu->caMsg) + i * 32, 32);
        qDebug() << "要转发给该好友文件:"
                 <<caRecvName;
        MyTcpServer::getInstance().resend(caRecvName, retPdu);
    }
    free(retPdu);
    retPdu = NULL;
    retPdu = mkPDU(0);
    retPdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_RESPOND;
    strcpy(retPdu->caData, "send share file msg ok");
    write((char*)retPdu, retPdu->uiPDULen);
}

void MyTcpSocket::shareFileNote(PDU *pdu)
{
    QString strRecvPath = QString("./%1").arg(pdu->caData);
    QString strShareFilePath = QString("%1").arg((char*)pdu->caMsg);
    int index = strShareFilePath.lastIndexOf('/');
    QString fileName = strShareFilePath.right(strShareFilePath.size() - index - 1);
    strRecvPath = strRecvPath + "/" + fileName;
    QFileInfo qFileInfo(strShareFilePath);
    if(qFileInfo.isFile())
    {
        QFile::copy(strShareFilePath, strRecvPath);
    }
    else if(qFileInfo.isDir())
    {
        copyDir(strShareFilePath, strRecvPath);
    }
    PDU *retPdu = mkPDU(0);
    retPdu->uiMsgLen = ENUM_MSG_TYPE_SHARE_FILE_NOTE_RESPOND;
    strcpy(retPdu->caData, "copy file ok");
    write((char*)retPdu, retPdu->uiPDULen);
    free(retPdu);
    retPdu = NULL;
}

void MyTcpSocket::copyDir(QString sourceDir, QString targetDir)
{
    QDir dir;
    dir.mkdir(targetDir);//创建目标文件夹，防止文件夹不存在
    dir.setPath(sourceDir);
    QFileInfoList fileInfoList = dir.entryInfoList();
    QString sourceTemp;
    QString targetTemp;
    for(int i = 0; i < fileInfoList.size(); i++)
    {
        sourceTemp = sourceDir + "/" + fileInfoList.at(i).fileName();
        targetTemp = targetDir + "/" + fileInfoList.at(i).fileName();
        if(fileInfoList.at(i).isFile())
        {
            QFile::copy(sourceTemp, targetTemp);
        }
        else if(fileInfoList.at(i).isDir())
        {
            // 不复制 . 和 ..目录
            if(QString(".") == fileInfoList.at(i).fileName()
            || QString("..") == fileInfoList.at(i).fileName())
            {
                continue;
            }
            copyDir(sourceTemp, targetTemp);
        }
    }

}

void MyTcpSocket::moveFile(PDU *pdu)
{
    char caFileName[32] = {'\0'};
    int srcLen = 0;
    int destLen = 0;
    sscanf(pdu->caData, "%d %d %s", &srcLen, &destLen, caFileName);

    char *pSrcPath = new char[srcLen + 1];
    char *pDestPath = new char[destLen +1 + 32];

    memset(pSrcPath, '\0', srcLen + 1);
    memset(pDestPath, '\0', destLen +1 + 32);

    memcpy(pSrcPath, pdu->caMsg,  srcLen);
    memcpy(pDestPath, (char*)(pdu->caMsg) + (srcLen + 1),  destLen);


    PDU *respdu = mkPDU(0);
    respdu->uiMsgType = ENUM_MSG_TYPE_MOVE_FILE_RESPOND;
    QFileInfo fileInfo(pDestPath);
    // 如果目标目录是目录才能移动
    if(fileInfo.isDir())
    {
        strcat(pDestPath, "/");
        strcat(pDestPath, caFileName);

        bool ret = QFile::rename(pSrcPath, pDestPath);
        if(ret)
        {
            strcpy(respdu->caData, MOVE_FILE_OK);
        }
        else
        {
            strcpy(respdu->caData, MOVE_FILE_FAIL);
        }
    }
    // 如果目标目录是文件夹，那么不能移动
    else if(fileInfo.isFile())
    {
        strcpy(respdu->caData, MOVE_FILE_FAIL);
    }
    // 其他文件类型目前不做处理
    else
    {
        strcpy(respdu->caData, MOVE_FILE_FAIL);
    }

    write((char*)respdu, respdu->uiPDULen);
    free(respdu);
    respdu = NULL;
}

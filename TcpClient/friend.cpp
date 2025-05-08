#include "friend.h"
#include"tcpclient.h"
#include"QInputDialog"
#include"privatechat.h"

Friend::Friend(QWidget *parent) : QWidget(parent)
{
    m_pShowMsgTE = new QTextEdit;
    m_pFriendListWidget = new QListWidget;
    m_pInputMsgLE = new QLineEdit;
    m_pDelFriendPB = new QPushButton("删除好友");
    m_pFlushFriendPB = new QPushButton("刷新好友");
    m_pShowOnlineUserPB = new QPushButton("显示在线用户");
    m_pSearchUserPB = new QPushButton("查找用户");
    m_pMsgSendPB = new QPushButton("信息发送");
    m_pPrivateChatPB = new QPushButton("私聊");
    m_pOnline = new Online;

    QVBoxLayout *pRightPBVBL = new QVBoxLayout;
    pRightPBVBL->addWidget(m_pDelFriendPB);
    pRightPBVBL->addWidget(m_pFlushFriendPB);
    pRightPBVBL->addWidget(m_pShowOnlineUserPB);
    pRightPBVBL->addWidget(m_pSearchUserPB);
    pRightPBVBL->addWidget(m_pPrivateChatPB);

    QHBoxLayout *pTopHBL = new QHBoxLayout;
    pTopHBL->addWidget(m_pShowMsgTE);
    pTopHBL->addWidget(m_pFriendListWidget);
    pTopHBL->addLayout(pRightPBVBL);

    QHBoxLayout *pMsgHBL = new QHBoxLayout;
    pMsgHBL->addWidget(m_pInputMsgLE);
    pMsgHBL->addWidget(m_pMsgSendPB);

    QVBoxLayout *pMain = new QVBoxLayout;
    pMain->addLayout(pTopHBL);
    pMain->addLayout(pMsgHBL);
    pMain->addWidget(m_pOnline);
    // 默认隐藏在线用户
    m_pOnline->hide();
    // 显示该布局
    setLayout(pMain);
    // 创建信号链接
    // 点击按钮事件
    connect(m_pShowOnlineUserPB, SIGNAL(clicked(bool)), this, SLOT(showOnline()));
    connect(m_pSearchUserPB, SIGNAL(clicked(bool)), this, SLOT(searchUser()));
    connect(m_pFlushFriendPB, SIGNAL(clicked(bool)), this, SLOT(flushFriend()));
    connect(m_pDelFriendPB, SIGNAL(clicked(bool)), this, SLOT(deleteFriend()));
    connect(m_pPrivateChatPB, SIGNAL(clicked(bool)), this, SLOT(privateChat()));
    connect(m_pMsgSendPB, SIGNAL(clicked(bool)), this, SLOT(groupChat()));
}

void Friend::showOnline()
{
    if(m_pOnline->isHidden())
    {
        m_pOnline->show();
        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINE_REQUEST;
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
        m_pShowOnlineUserPB->setText("隐藏在线用户");
    }
    else
    {
        m_pOnline->hide();
        m_pShowOnlineUserPB->setText("显示在线用户");
    }
}

void Friend::searchUser()
{
    m_strSearchName = QInputDialog::getText(this, "搜索", "用户名：");
    // 输入不为空
    if(!m_strSearchName.isEmpty())
    {
        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_SEARCH_USER_REQUEST;
        strcpy(pdu->caData, m_strSearchName.toStdString().c_str());
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
}

void Friend::flushFriend()
{
    QString loginName = TcpClient::getInstance().loginName();
    PDU *pdu = mkPDU(0);
    pdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST;
    strcpy(pdu->caData, loginName.toStdString().c_str());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

void Friend::deleteFriend()
{
    QListWidgetItem *item = m_pFriendListWidget->currentItem();
    if(NULL == item)
    {
        return ;
    }
    QString name = item->text();
    PDU *pdu = mkPDU(0);
    pdu->uiMsgType = ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST;
    memcpy(pdu->caData, name.toStdString().c_str(), name.size());
    memcpy(pdu->caData + 32, TcpClient::getInstance().loginName().toStdString().c_str(), TcpClient::getInstance().loginName().size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

void Friend::privateChat()
{
    QListWidgetItem *item = m_pFriendListWidget->currentItem();
    if(NULL == item)
    {
        QMessageBox::warning(this, "私聊", "请选择私聊对象");
        return ;
    }
    else
    {
        QString friendName = item->text();
        PrivateChat::getInstace().setChatName(friendName);
        if(PrivateChat::getInstace().isHidden())
        {
            PrivateChat::getInstace().show();
        }
    }
}

void Friend::groupChat()
{
    QString msg = m_pInputMsgLE->text();
    if(msg.isEmpty())
    {
        QMessageBox::warning(this, "群聊", "发送消息不能为空");
        return;
    }
    m_pInputMsgLE->clear();
    PDU *pdu = mkPDU(msg.size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_GROUP_CHAT_REQUEST;
    strncpy(pdu->caData, TcpClient::getInstance().loginName().toStdString().c_str(), TcpClient::getInstance().loginName().size());
    strncpy((char *)pdu->caMsg, msg.toStdString().c_str(), msg.size());
    TcpClient::getInstance().getTcpSocket().write((char *)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

void Friend::showAllOnlineUser(PDU *pdu)
{
    if(NULL == pdu)
    {
        return ;
    }
    m_pOnline->showUser(pdu);
}

void Friend::updateFriendList(PDU *pdu)
{
    if(NULL == pdu)
    {
        return ;
    }
    m_pFriendListWidget->clear();
    uint uiSize = pdu->uiMsgLen / 32;
    for(int i = 0; i < uiSize; i++)
    {
        char caName[32] = {'\0'};
        // 注意，这里的 pdu->caMsg 也应该是 char * 才对，因为他复制的是 char *的内容
        memcpy(caName, (char*)(pdu->caMsg) + i * 32, 32);
        m_pFriendListWidget->addItem(caName);
    }
}

void Friend::updateGroupMsg(PDU *pdu)
{
    char caLoginName[32] = {'\0'};
    memcpy(caLoginName, pdu->caData, 32);
    m_pShowMsgTE->append(QString("%1 says: %2").arg(caLoginName).arg((char*)pdu->caMsg));
}

QListWidget *Friend::getFriendList()
{
    return m_pFriendListWidget;
}

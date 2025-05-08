#include "privatechat.h"
#include "ui_privatechat.h"
#include"protocol.h"
#include"tcpclient.h"
#include<QMessageBox>

PrivateChat::PrivateChat(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PrivateChat)
{
    ui->setupUi(this);
}

PrivateChat::~PrivateChat()
{
    delete ui;
}

PrivateChat &PrivateChat::getInstace()
{
    static PrivateChat instance;
    return instance;
}

void PrivateChat::setChatName(QString strName)
{
    m_strChatName = strName;
    m_strLoginName= TcpClient::getInstance().loginName();
}

void PrivateChat::updateMsg(const PDU *pdu)
{
    if(NULL == pdu)
    {
        return ;
    }
    char caLoginName[32] = {'\0'};
    memcpy(caLoginName, pdu->caData + 32, 32);
    QString msg = QString("%1 says: %2").arg(caLoginName).arg((char*)pdu->caMsg);
    ui->showMsg_te->append(msg);
}

void PrivateChat::on_pushButton_clicked()
{
    QString strMsg = ui->inputMsg_le->text();
    if(!strMsg.isEmpty())
    {
        ui->inputMsg_le->clear();
        PDU *pdu = mkPDU(strMsg.size() + 1);//+1是因为转为char后需要多一个 \0结束符
        pdu->uiMsgType = ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST;
        // 先复制好友的名称
        memcpy(pdu->caData, m_strChatName.toStdString().c_str(), m_strChatName.size());
        memcpy(pdu->caData + 32, m_strLoginName.toStdString().c_str(), m_strLoginName.size());
        strcpy((char*)pdu->caMsg, strMsg.toStdString().c_str());
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    }
    else
    {
        QMessageBox::warning(this, "私聊", "发送的内容不能为空");
    }
}

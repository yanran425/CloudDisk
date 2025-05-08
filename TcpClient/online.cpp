#include "online.h"
#include "ui_online.h"
#include"QDebug"
#include"tcpclient.h"

Online::Online(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Online)
{
    ui->setupUi(this);
}

Online::~Online()
{
    delete ui;
}

void Online::showUser(PDU *pdu)
{
    if(NULL == pdu)
    {
        return ;
    }
    // 用户个数
    uint uiSize = pdu->uiMsgLen / 32;
    char caTmp[32];//缓存数组
    for(uint i = 0; i < uiSize; i++)
    {
        memcpy(caTmp, (char*)(pdu->caMsg) + i * 32, 32);
        ui->online_lw->addItem(caTmp);
    }
}

void Online::on_addFriend_Pb_clicked()
{
    QListWidgetItem *item = ui->online_lw->currentItem();
    if(NULL == item)
    {
        return;
    }
    // 选择的好友名称
    QString friendName = item->text();
    QString loginName = TcpClient::getInstance().loginName();
    if( NULL == friendName || NULL == loginName)
    {
        QMessageBox::critical(this, "添加好友", "添加好友失败：当前用户或选择不能为空");
        return ;
    }
    if(friendName == loginName)
    {
        QMessageBox::critical(this, "添加好友", "添加好友失败：不能添加自己");
        return ;
    }
    PDU *pdu = mkPDU(0);
    pdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_REQUEST;
    memcpy(pdu->caData, friendName.toStdString().c_str(), friendName.size());
    memcpy(pdu->caData + 32, loginName.toStdString().c_str(), loginName.size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

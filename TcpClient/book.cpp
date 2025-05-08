#include "book.h"
#include"tcpclient.h"
#include"QInputDialog"
#include"QMessageBox"
#include<QFileDialog>
#include<QFile>
#include"sharefile.h"

Book::Book(QWidget *parent) : QWidget(parent)
{
    m_pDownload = false;
    m_pTimer = new QTimer;
    m_pBookListW = new QListWidget;
    m_pReturnPB = new QPushButton("返回");
    m_pCreateDirPB = new QPushButton("创建文件夹");
    m_pDelDirPB = new QPushButton("删除文件夹");
    m_pRenameDirPB = new QPushButton("重命名文件夹");
    m_pFlushDirPB = new QPushButton("刷新文件夹");
    m_pUploadFilePB = new QPushButton("上传文件");
    m_pDelFilePB = new QPushButton("删除文件");
    m_pDownloadFilePB = new QPushButton("下载文件");
    m_pShareFilePB = new QPushButton("分享文件");
    m_pMoveFilePB = new QPushButton("移动文件");
    m_pMoveSelectDirPB = new QPushButton("移动目标目录");
    m_pMoveSelectDirPB->setEnabled(false);



    QVBoxLayout *dirLayout = new QVBoxLayout;
    dirLayout->addWidget(m_pReturnPB);
    dirLayout->addWidget(m_pCreateDirPB);
    dirLayout->addWidget(m_pDelDirPB);
    dirLayout->addWidget(m_pRenameDirPB);
    dirLayout->addWidget(m_pFlushDirPB);

    QVBoxLayout *fileLayout = new QVBoxLayout;
    fileLayout->addWidget(m_pUploadFilePB);
    fileLayout->addWidget(m_pDelFilePB);
    fileLayout->addWidget(m_pDownloadFilePB);
    fileLayout->addWidget(m_pShareFilePB);
    fileLayout->addWidget(m_pMoveFilePB);
    fileLayout->addWidget(m_pMoveSelectDirPB);

    QHBoxLayout *hBoxLayout = new QHBoxLayout;
    hBoxLayout->addWidget(m_pBookListW);
    hBoxLayout->addLayout(dirLayout);
    hBoxLayout->addLayout(fileLayout);

    setLayout(hBoxLayout);
    // 槽与信号
    connect(m_pCreateDirPB, SIGNAL(clicked(bool)), this, SLOT(createDir()));
    connect(m_pFlushDirPB, SIGNAL(clicked(bool)), this, SLOT(flushDir()));
    connect(m_pDelDirPB, SIGNAL(clicked(bool)), this, SLOT(delDir()));
    connect(m_pRenameDirPB, SIGNAL(clicked(bool)), this, SLOT(renameDir()));
    connect(m_pBookListW, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(enterDir(QModelIndex)));
    connect(m_pReturnPB, SIGNAL(clicked(bool)),
            this, SLOT(returnPre()));
    connect(m_pUploadFilePB, SIGNAL(clicked(bool)),
            this, SLOT(uploadPre()));
    connect(m_pTimer, SIGNAL(timeout()),
            this, SLOT(uploadFileData()));
    connect(m_pDelFilePB, SIGNAL(clicked(bool)),
            this, SLOT(delFile()));
    connect(m_pDownloadFilePB, SIGNAL(clicked(bool)),
            this, SLOT(downloadFile()));
    connect(m_pShareFilePB, SIGNAL(clicked(bool)),
            this, SLOT(shareFile()));
    connect(m_pMoveFilePB, SIGNAL(clicked(bool)),
            this, SLOT(moveFile()));
    connect(m_pMoveSelectDirPB, SIGNAL(clicked(bool)),
            this, SLOT(selectDestDir()));

}

void Book::updateDirList(const PDU *pdu)
{
    if(NULL == pdu)
    {
        return ;
    }
    // 清除之前的列表，避免这里的add导致重复，也可以使用for循环 m_pBookListW->item(rowIdx)来进行删除每一行
    m_pBookListW->clear();
    FileInfo *pFileInfo = NULL;
    int iFileCount = pdu->uiMsgLen / sizeof(FileInfo);
    for(int i = 0; i < iFileCount; i++)
    {
        pFileInfo = (FileInfo *)(pdu->caMsg) + i;
        QListWidgetItem *pItem = new QListWidgetItem;
        if(0 == pFileInfo->iFileType)
        {
            pItem->setIcon(QIcon(QPixmap(":/map/dir.png")));
        }
        else if(1 == pFileInfo->iFileType)
        {
            pItem->setIcon(QIcon(QPixmap(":/map/reg.jpg")));
        }
        pItem->setText(pFileInfo->caFileName);
        m_pBookListW->addItem(pItem);
    }
}

QString Book::getEnterPath()
{
    return m_enterPath;
}

void Book::setDownloadStatus(bool status)
{
    m_pDownload = status;
}

bool Book::getDownloadStatus()
{
    return m_pDownload;
}

QString Book::getSaveFilePath()
{
    return m_strSaveFilePath;
}

void Book::updateLocalDownloadFileName()
{
    m_pFile.setFileName(m_strSaveFilePath);
}

QString Book::getShareFileName()
{
    return m_shareFileName;
}

void Book::createDir()
{
    QString strDirName = QInputDialog::getText(this, "新建文件夹", "新文件夹名称");
    if(strDirName.isEmpty())
    {
        QMessageBox::warning(this, "新建文件夹", "新文件夹名称不能为空");
        return ;
    }
    if(strDirName.size() > 32)
    {
        QMessageBox::warning(this, "新建文件夹", "新文件夹名称不能超过32个字符");
        return ;
    }
    QString strLoginName = TcpClient::getInstance().loginName();
    QString strCurPath = TcpClient::getInstance().curPath();
    PDU *pdu = mkPDU(strCurPath.size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_REQUEST;
    // 用户名、新文件夹名称放在caData， 当前路径放在caMsg
    strncpy(pdu->caData, strLoginName.toStdString().c_str(), strLoginName.size());
    strncpy(pdu->caData + 32, strDirName.toStdString().c_str(), strDirName.size());
    memcpy((char *)pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

void Book::flushDir()
{
    QString strCurPath = TcpClient::getInstance().curPath();
    PDU *pdu = mkPDU(strCurPath.size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_DIR_REQUEST;
    strncpy((char*)pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

void Book::delDir()
{
    QString strCurPath = TcpClient::getInstance().curPath();
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if(NULL == pItem)
    {
        QMessageBox::warning(this, "删除文件", "请选择要删除的文件");
        return ;
    }
    QString strDelName = pItem->text();
    PDU *pdu = mkPDU(strCurPath.size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_REQUEST;
    strncpy(pdu->caData, strDelName.toStdString().c_str(), strDelName.size());
    memcpy((char*)pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

void Book::renameDir()
{
    QString strCurPath = TcpClient::getInstance().curPath();
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if(NULL == pItem)
    {
        QMessageBox::warning(this, "重命名文件", "请选择要重命名的文件");
        return ;
    }
    QString strOldName = pItem->text();
    QString strNewName = QInputDialog::getText(this, "重命名文件", "请输入新的文件名");
    if(strNewName.isEmpty())
    {
        QMessageBox::warning(this, "重命名文件", "新文件名不能为空");
        return ;
    }
    PDU *pdu = mkPDU(strCurPath.size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_RENAME_DIR_REQUEST;
    strncpy(pdu->caData, strOldName.toStdString().c_str(), strOldName.size());
    strncpy(pdu->caData + 32, strNewName.toStdString().c_str(), strNewName.size());
    memcpy((char*)pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

void Book::enterDir(const QModelIndex &index)
{
    QString strDirName = index.data().toString();
    QString strCurPath = TcpClient::getInstance().curPath();
    // 缓存数据
    m_enterPath = QString("%1/%2").arg(strCurPath).arg(strDirName);
    PDU *pdu = mkPDU(strCurPath.size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_REQUEST;
    strncpy(pdu->caData, strDirName.toStdString().c_str(), strDirName.size());
    memcpy((char*)pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

void Book::returnPre()
{
    // 当前目录
    QString strCurPath = TcpClient::getInstance().curPath();
    // 用户根目录
    QString strRootPath = "./" + TcpClient::getInstance().loginName();
    // 如果当前目录就是用户根目录，代表没有上级目录了
    if(strCurPath == strRootPath)
    {
        QMessageBox::warning(this, "返回", "返回上一级失败：当前已经在用户根目录了");
        return ;
    }
    // ./alice/aa/bb -> ./alice/aa
    int idx = strCurPath.lastIndexOf('/');
    strCurPath.remove(idx, strCurPath.size()-idx);
    // 更新当前所在目录位置
    TcpClient::getInstance().setCurPath(strCurPath);
    // 刷新目录文件为当前目录位置的文件
    flushDir();
}

void Book::uploadPre()
{
    // 当前目录
    QString strCurPath = TcpClient::getInstance().curPath();
    m_strUploadFilePath = QFileDialog::getOpenFileName();
    if(m_strUploadFilePath.isEmpty())
    {
        QMessageBox::warning(this, "上传文件", "上传文件名称不能为空");
        return ;
    }
    int idx = m_strUploadFilePath.lastIndexOf('/');
    QString fileName = m_strUploadFilePath.right(m_strUploadFilePath.size() - idx - 1);
    QFile file(m_strUploadFilePath);
    qint64 fileSize = file.size();//文件的大小
    PDU *pdu = mkPDU(strCurPath.size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST;
    memcpy((char*)pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
    sprintf(pdu->caData, "%s %lld", fileName.toStdString().c_str(), fileSize);
    qDebug() << pdu->caData;
    qDebug() << (char*)pdu->caMsg;
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
    // 1s后上传文件，防止粘包
    m_pTimer->start(1000);
}

void Book::uploadFileData()
{
    m_pTimer->stop();
    QFile file(m_strUploadFilePath);
    // 如果打开文件失败
    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "上传文件", "打开文件失败");
        return;
    }
    // 有人做过实验，4096传输数据时效率最高
    char *pBuffer = new char[4096];
    qint64 ret = 0;
    while(true)
    {
        // 读取文件中的数据
        ret = file.read(pBuffer, 4096);
        // 如果读取到了文件中的数据，则发送给服务器
        if(ret > 0 && ret <= 4096)
        {
            TcpClient::getInstance().getTcpSocket().write(pBuffer, ret);
        }
        // 如果文件读取结束了，就结束循环
        else if(ret == 0)
        {
            break;
        }
        // 如果ret < 0 或者 ret > 4096,则代表读取错误了
        else
        {
            QMessageBox::warning(this, "上传文件", "上传文件失败：读取文件内容失败");
            break;
        }
    }
    delete []pBuffer;
    pBuffer = NULL;
}

void Book::delFile()
{
    QString strCurPath = TcpClient::getInstance().curPath();
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if(NULL == pItem)
    {
        QMessageBox::warning(this, "删除文件", "请选择要删除的文件");
        return ;
    }
    QString strDelName = pItem->text();
    PDU *pdu = mkPDU(strCurPath.size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_REQUEST;
    strncpy(pdu->caData, strDelName.toStdString().c_str(), strDelName.size());
    memcpy((char*)pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

void Book::downloadFile()
{
    QString strCurPath = TcpClient::getInstance().curPath();
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if(NULL == pItem)
    {
        QMessageBox::warning(this, "下载文件", "请选择要下载的文件");
        return ;
    }
    QString strDownloadName = pItem->text();
    QString strSaveFilePath = QFileDialog::getSaveFileName();
    if(strSaveFilePath.isEmpty())
    {
        QMessageBox::warning(this, "下载文件", "请指定要保存的位置");
        m_strSaveFilePath.clear();
        return ;
    }
    m_strSaveFilePath = strSaveFilePath;
    PDU *pdu = mkPDU(strCurPath.size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST;
    strcpy(pdu->caData, strDownloadName.toStdString().c_str());
    memcpy((char*)pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

void Book::shareFile()
{
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if(NULL == pItem)
    {
        QMessageBox::warning(this, "分享文件", "请选择要分享的文件");
        return ;
    }
    m_shareFileName = pItem->text();
    Friend *pFriend = OpeWidget::getInstance().getFriend();
    QListWidget *pFriendList = pFriend->getFriendList();
    ShareFile::getInstance().updateFriend(pFriendList);
    if(ShareFile::getInstance().isHidden())
    {
        ShareFile::getInstance().show();
    }
}

void Book::moveFile()
{
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if(NULL == pItem)
    {
        QMessageBox::warning(this, "移动文件", "请选择要移动的文件");
        return ;
    }
    m_strMoveFileName = pItem->text();
    QString strCurPath = TcpClient::getInstance().curPath();
    m_strMoveFilePath = strCurPath + "/" +m_strMoveFileName;
    m_pMoveSelectDirPB->setEnabled(true);
}

void Book::selectDestDir()
{
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if(NULL == pItem)
    {
        QMessageBox::warning(this, "选择移动目录", "请选择目标目录");
        return ;
    }
    QString strDestDir = pItem->text();
    QString strCurPath = TcpClient::getInstance().curPath();
    m_strSelectDestDirPath = strCurPath + "/" + strDestDir;
    m_pMoveSelectDirPB->setEnabled(false);

    int srcLen = m_strMoveFilePath.size();
    int destLen = m_strSelectDestDirPath.size();
    PDU *pdu = mkPDU(srcLen + destLen + 2);
    pdu->uiMsgType = ENUM_MSG_TYPE_MOVE_FILE_REQUEST;
    sprintf(pdu->caData, "%d %d %s", srcLen, destLen, m_strMoveFileName.toStdString().c_str());

    memcpy(pdu->caMsg, m_strMoveFilePath.toStdString().c_str(), srcLen);
    memcpy((char*)(pdu->caMsg) + (srcLen + 1), m_strSelectDestDirPath.toStdString().c_str(), destLen);
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

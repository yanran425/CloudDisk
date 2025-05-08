#include "opewidget.h"

OpeWidget::OpeWidget(QWidget *parent) : QWidget(parent)
{
    m_pFriend = new Friend;
    m_pBook = new Book;
    m_pSW = new QStackedWidget;
    // 创建
    m_pSW->addWidget(m_pFriend);
    m_pSW->addWidget(m_pBook);
    //
    m_pListW = new QListWidget(this);
    m_pListW->addItem("好友");
    m_pListW->addItem("图书");
    // 显示界面
    QHBoxLayout *pMain = new QHBoxLayout;
    pMain->addWidget(m_pListW);
    pMain->addWidget(m_pSW);
    setLayout(pMain);
    // 添加信号
    // tab的切换
    connect(m_pListW, SIGNAL(currentRowChanged(int)), m_pSW, SLOT(setCurrentIndex(int)));
}

OpeWidget &OpeWidget::getInstance()
{
    static OpeWidget instance;
    return instance;
}

Friend *OpeWidget::getFriend()
{
    return m_pFriend;
}

Book *OpeWidget::getBook()
{
    return m_pBook;
}

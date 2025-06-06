#ifndef SHAREFILE_H
#define SHAREFILE_H

#include <QWidget>
#include<QPushButton>
#include<QHBoxLayout>
#include<QVBoxLayout>
#include<QButtonGroup>
#include<QScrollArea>
#include<QCheckBox>
#include<QListWidget>

class ShareFile : public QWidget
{
    Q_OBJECT
public:
    explicit ShareFile(QWidget *parent = 0);
    static ShareFile &getInstance();
    void updateFriend(QListWidget *pFriendList);

signals:

public slots:
    void cancelSelect();
    void selectAll();
    void okShare();
    void cancelShare();
private:
    QPushButton *m_pSelectAllPB;
    QPushButton *m_pCancleSelectPB;

    QPushButton *m_pOKPB;
    QPushButton *m_pCanclePB;

    QScrollArea *m_pSA;
    QWidget     *m_pFriendW;
    QVBoxLayout *m_pFriendWVBL;//专属给m_pFriendW使用的布局
    QButtonGroup *m_pButtonGroup;
};

#endif // SHAREFILE_H

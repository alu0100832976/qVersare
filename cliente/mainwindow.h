#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QMainWindow>
#include <QRegExp>
#include <QDebug>
#include <QPixmap>
#include <QFile>
#include <QPair>
#include <QList>
#include <QDateTime>
#include <QTimer>

#include "avatarmanager.h"
#include "client.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void emitUpdateUserList(QString username, QDateTime time);



private slots:
    void on_exitButton_clicked();

    void on_connectButton_clicked();

    void on_aboutButton_clicked();

    void on_SendTextEdit_returnPressed();

    void on_confButton_clicked();

    void readyToRead(QString aux);

    void on_imageButton_clicked();

public slots:
    void sendLogin(QString username, QString password);

    void refreshAvatar(QString filename);

    void addUser(QString username, QDateTime time);

    void refreshLocalUser(QString username, QDateTime time);

    void needAvatar(QString username, QDateTime time);

    void updateAvatar(QString username, QDateTime time, QPixmap image, bool same);

    void setAvatar(QString username);

    void onErrorLoadingAvatar();



private:
    Ui::MainWindow *ui;
    bool isConnectedButton_;
    bool isConnectedToServer_;
    Client *client_;

    AvatarManager myAvatarManager_;
    QTimer myTimer_;
    QString paddingHtmlWithSpaces(int number);

};

#endif // MAINWINDOW_H

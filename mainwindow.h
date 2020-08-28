#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QtSerialPort/QSerialPort>
#include<QtSerialPort/QSerialPortInfo>
#include<QTimer>
#include<QDebug>
#include<QMessageBox>
#include<QLineEdit>
#include<QList>
#include<QRadioButton>
#include <QElapsedTimer>
#include<QTime>
#include<math.h>
#include<time.h>
#include <QCloseEvent>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QByteArray StringToHex(QString str);
    char ConvertHexChar(char ch);


private slots:
//    void on_sendBottom_clicked();

    void on_openButtom_clicked();

    void readPosition();

    void on_closeButton_clicked();

//    void on_enableButtom_toggled(bool checked);

    void on_stopButton_clicked();

    void isZero();

    void isEnd();

    void on_openButton_clicked();

//    void on_closeButton_2_clicked();

    void on_routineButton_clicked();

    void refreshTime();

private:
    Ui::MainWindow *ui;
    QSerialPort *my_serialPort;
    QByteArray requestData;
    QTimer *timer1;
    QTimer *timer2;
    int limit;
    int zeroPoint;
    bool isStop;
    bool isOpen;//端口是否打开
    bool isFlag;//是否找到零点
    void checkAvailablePorts();
    void setTextColor();
    int current_position;
    QByteArray setSpeed(QString speed);
    QByteArray setAcc(QString  acc);
    QByteArray setNAcc(QString nacc);
    QByteArray setPos(QString pos);
    QByteArray switchHL(QString str);
    QByteArray setInterval();
    uint16_t crc_16(uint8_t *data);
    void Delay_MSec_Suspend(unsigned int msec);
    void Delay_MSec(unsigned int msec);
    int byteAraryToInt(QByteArray arr);
    void zeroTest();
    void digitalOutput();
    bool isOnZero();
    int lengthToPmw(QString length);
    void closeEvent(QCloseEvent *event);
    QString getTime();
    QString getSpeed();
    QString getAcc();
    QString getInterval();
};


#endif // MAINWINDOW_H

#include "mainwindow.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    my_serialPort=new QSerialPort(this);
    ui->setupUi(this);
    this->setWindowTitle(tr("变形翼运动控制软件"));
    timer1=new QTimer(this);
    timer2=new QTimer(this);
    setFixedSize(this->width(), this->height());
    emit checkAvailablePorts();
    setTextColor();
    isFlag=false;
    isStop=false;
    isOpen=false;
    zeroPoint=0;
    current_position=0;
    connect(timer2,SIGNAL(timeout()),this,SLOT(refreshTime()));
    timer2->start(59);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::checkAvailablePorts()//检查可用端口
{
    foreach( const QSerialPortInfo &Info,QSerialPortInfo::availablePorts())
    {
        QSerialPort serial;
        serial.setPort(Info);
        ui->portBox->addItem( Info.portName() );
    }
    ui->statusLine->setText("请打开端口");
}

void MainWindow::setTextColor()//设置gui界面字体颜色
{
    QPalette palette;
    palette.setColor(QPalette::Text,Qt::black);
//    ui->posLine->setPalette(palette);
    ui->statusLine->setPalette(palette);
//    ui->readPosLine->setPalette(palette);
}


QByteArray MainWindow::setSpeed(QString speed)//读入十进制速度
{
    QString tmp,tmp1,crcCode;
    QByteArray byte;
    uint8_t data[10];
    int speedInt=speed.toInt();
    if(speedInt<0 || speedInt>2300)
    {
        QMessageBox::about(this,tr("Warning!"),tr("速度度范围为0~2300"));
        return NULL;
    }
    tmp="110360810100";
    tmp1=QString("%1").arg(speedInt,4,16,QChar('0'));//将速度变成十六进制并加入
    tmp.append(tmp1);
    tmp.append("0000");//构建速度检测串
    byte=StringToHex(tmp);//变成十六进制QByteArray
    for(int i=0;i<10;i++)
    {
        data[i]=byte[i];
    }
    crcCode=QString("%1").arg(crc_16(data),4,16,QLatin1Char('0'));
    tmp.append(crcCode);//将crc拼接到原字符串中
    return switchHL(tmp);//高低位交换
}

QByteArray MainWindow::setAcc(QString acc)
{
    QString tmp,tmp1,crcCode;
    QByteArray byte;
    uint8_t data[10];
    int accInt=acc.toInt();
    tmp="110360830100";
    tmp1=QString("%1").arg(accInt,4,16,QChar('0'));
    tmp.append(tmp1);
    tmp.append("0000");
    byte=StringToHex(tmp);
    for(int i=0;i<10;i++)
    {
        data[i]=byte[i];
    }
    crcCode=QString("%1").arg(crc_16(data),4,16,QLatin1Char('0'));
    tmp.append(crcCode);
    return switchHL(tmp);
}

QByteArray MainWindow::setNAcc(QString nacc)
{
    QString tmp,tmp1,crcCode;
    QByteArray byte;
    uint8_t data[10];
    int naccInt=nacc.toInt();
    tmp="110360840100";
    tmp1=QString("%1").arg(naccInt,4,16,QChar('0'));
    tmp.append(tmp1);
    tmp.append("0000");
    byte=StringToHex(tmp);
    for(int i=0;i<10;i++)
    {
        data[i]=byte[i];
    }
    crcCode=QString("%1").arg(crc_16(data),4,16,QLatin1Char('0'));
    tmp.append(crcCode);
    return switchHL(tmp);
}

QByteArray MainWindow::setPos(QString pos)
{
    QString tmp,tmp1,tmp2,crcCode;
    QByteArray byte;
    uint8_t data[10];
    int posInt=pos.toInt();
    tmp="1103607A0100";
    tmp1=QString("%1").arg(posInt,8,16,QChar('0'));
    tmp2=tmp1.right(8).left(4);
    tmp.append(tmp1.right(4));
    tmp.append(tmp2);
    byte=StringToHex(tmp);
    for(int i=0;i<10;i++)
    {
        data[i]=byte[i];
    }
    crcCode=QString("%1").arg(crc_16(data),4,16,QLatin1Char('0'));
    tmp.append(crcCode);
    return switchHL(tmp);
}


void MainWindow::Delay_MSec(unsigned int msec)
{
    QTime _Timer = QTime::currentTime().addMSecs(msec);

    while( QTime::currentTime() < _Timer )

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

int MainWindow::byteAraryToInt(QByteArray arr)
{
    if (arr.size() < 4)
        return 0;

    int res = 0;
    res = (arr.at(0) << 24) & 0xFF000000;
    res |= (arr.at(1) << 16) & 0x00FF0000;
    res |= arr.at(2) << 8 & 0x0000FF00;
    res |= arr.at(3) & 0x000000FF;
    return res;
}

void MainWindow::on_openButtom_clicked()//打开串口
{
    my_serialPort->setPortName( ui->portBox->currentText());
    if(my_serialPort->open( QIODevice::ReadWrite ))
    {
        isOpen=true;
        ui->statusLine->setText("打开端口成功");
//        ui->closeButton->setEnabled(true);
        qDebug() << ui->portBox->currentText();
        my_serialPort->setBaudRate(  ui->baudBox->currentText().toInt() );
        my_serialPort->setDataBits( QSerialPort::Data8 );//数据字节 8字节
        my_serialPort->setParity( QSerialPort::NoParity );//无校验
        my_serialPort->setFlowControl( QSerialPort::NoFlowControl );//无数据流控制
        my_serialPort->setStopBits( QSerialPort::OneStop );//一个停止位
        ui->openButtom->setEnabled(false);
        my_serialPort->write(StringToHex("11 03 40 60 00 01 80 00 00 00 39 F8"));//清楚错误码
        Delay_MSec(510);
        my_serialPort->write(setInterval());//设置串口控制延迟
        Delay_MSec(60);
        my_serialPort->write(StringToHex("11 03 40 60 00 01 06 00 00 00 C3 71"));//关闭使能
        zeroTest();
        connect( timer1, SIGNAL( timeout() ), this, SLOT( isZero() ) );
        timer1->start(200);
    }
    else
    {
        ui->statusLine->setText("打开端口失败");
        return;
    }
}


void MainWindow::zeroTest()
{
    QList<QByteArray> instructs;
    my_serialPort->write(StringToHex("11 03 60 60 00 01 01 00 00 00 A5 9A"));//位置模式
            Delay_MSec(55);
    my_serialPort->write(StringToHex("11 03 40 60 00 01 0F 00 00 00 52 EF"));//使能打开
            Delay_MSec(55);
    my_serialPort->write(setAcc("200"));
            Delay_MSec(55);
    my_serialPort->write(setNAcc("800"));
            Delay_MSec(55);
    my_serialPort->write(setSpeed("15"));
            Delay_MSec(55);
    my_serialPort->write(setPos("4000"));
            Delay_MSec(55);
    my_serialPort->write(StringToHex("1103406000017F0000005A37"));//相对
    Delay_MSec(3000);
    my_serialPort->write(setPos("-100000"));
            Delay_MSec(55);
    my_serialPort->write(StringToHex("1103406000017F0000005A37"));//相对
    ui->statusLine->setText("正在找零点");
}

void MainWindow::isZero()
{
//    qDebug()<<"isZero"<<endl;
//    QString posRead("10 01 71 20 01 01 B3 4B 4F");
//    QByteArray sendData=StringToHex(posRead);
//    my_serialPort->write(sendData);
//    QByteArray requestData = my_serialPort->readAll();
//    if(requestData.length()==14)
//    {
//        if(requestData.toHex().at(17)=='1')
//        {
//           qDebug()<<"找到零点"<<endl;
//           timer1->stop();
//           my_serialPort->write(StringToHex("11 03 40 60 00 01 0F 01 00 00 E6 99"));
//           Delay_MSec(55);
//           my_serialPort->write(setPos("300"));
//           Delay_MSec(55);
//           my_serialPort->write(StringToHex("1103406000017F0000005A37"));
//           Delay_MSec(1500);
//           requestData=my_serialPort->readAll();
//           requestData.clear();
//           readPosition();
//        }
//    }
//    requestData.clear();
    QString posRead("10 01 64 60 00 01 9D 9D 4F");//读取posRead
    QByteArray sendData=StringToHex(posRead);
    QByteArray requestData;
    my_serialPort->write(sendData);
    Delay_MSec(100);
    requestData = my_serialPort->readAll();
    if(requestData.length()==14)
    {
        QByteArray tmp;
        tmp.append(requestData[11]);
        tmp.append(requestData[10]);
        tmp.append(requestData[9]);
        tmp.append(requestData[8]);
        int num=byteAraryToInt(tmp);
        if(current_position==num)
        {
            timer1->stop();
            my_serialPort->write(StringToHex("11 03 40 60 00 01 80 00 00 00 39 F8"));//清楚错误码
            Delay_MSec(520);
            my_serialPort->write(setInterval());//设置串口控制延迟
            Delay_MSec(60);
            my_serialPort->write(StringToHex("11 03 40 60 00 01 06 00 00 00 C3 71"));//关闭使能
            Delay_MSec(55);
            my_serialPort->write(StringToHex("11 03 60 60 00 01 01 00 00 00 A5 9A"));//位置模式
            Delay_MSec(55);
            my_serialPort->write(StringToHex("11 03 40 60 00 01 0F 00 00 00 52 EF"));//使能打开
            Delay_MSec(60);
            my_serialPort->write(setPos("500"));
            Delay_MSec(60);
            my_serialPort->write(StringToHex("1103406000017F0000005A37"));//相对
            zeroPoint=num+500;
            ui->statusLine->setText("找到零点");
            isFlag=true;
            ui->routineButton->setEnabled(true);
            ui->stopButton->setEnabled(true);
 //           ui->closeButton_2->setEnabled(true);
            ui->openButton->setEnabled(true);
        }
        else
        {
            current_position=num;
        }
    }
}

void MainWindow::readPosition()
{
    QString posRead("10 01 64 60 00 01 9D 9D 4F");//读取posRead
    QByteArray sendData=StringToHex(posRead);
    QByteArray requestData;
    while(true)
    {
        my_serialPort->write(sendData);
        Delay_MSec(100);
        requestData = my_serialPort->readAll();
        if(requestData.length()==14)
        {
            QByteArray tmp;
            tmp.append(requestData[11]);
            tmp.append(requestData[10]);
            tmp.append(requestData[9]);
            tmp.append(requestData[8]);
            int num=byteAraryToInt(tmp);
                ui->routineButton->setEnabled(true);
                ui->stopButton->setEnabled(true);
     //           ui->closeButton_2->setEnabled(true);
                ui->openButton->setEnabled(true);
                zeroPoint=num;
                qDebug()<<"零点是："<<zeroPoint<<endl;
                isFlag=true;
                ui->statusLine->setText("找到零点");
                connect( timer2, SIGNAL( timeout() ), this, SLOT( isEnd() ) );
                timer2->start(500);
                break;
        }
    }
    requestData.clear();
}

void MainWindow::isEnd()
{
    qDebug()<<"isEnd"<<endl;
    QString posRead("10 01 71 20 01 01 B3 4B 4F");
    QByteArray sendData=StringToHex(posRead);
    my_serialPort->write(sendData);
    requestData = my_serialPort->readAll();
    if(requestData.length()==14)
    {
        if(requestData.toHex().at(17)=='2'||requestData.toHex().at(17)=='1')
        {
           ui->statusLine->setText("触发限位开关");
           my_serialPort->write(StringToHex("1103406000010B0000009233"));
        }
    }
}


void MainWindow::on_closeButton_clicked()
{
    timer1->stop();
    my_serialPort->write(StringToHex("11 03 40 60 00 01 06 00 00 00 C3 71"));
    Delay_MSec(55);
    //    my_serialPort->close();
    qDebug()<<"我要关闭了！"<<endl;
    my_serialPort->close();
    ui->statusLine->setText("已关闭端口");
//    ui->closeButton->setEnabled(false);
    ui->openButtom->setEnabled(true);
//    ui->closeButton->setEnabled(false);
    ui->openButton->setEnabled(false);
//    ui->closeButton_2->setEnabled(false);
    ui->routineButton->setEnabled(false);
    ui->stopButton->setEnabled(false);
}

QByteArray MainWindow::StringToHex(QString str) //字符串转换为十六进制数据0-F
{
    QByteArray senddata;
    int hexdata,lowhexdata;
    int hexdatalen = 0;
    int len = str.length();
    senddata.resize(len / 2);
    char lstr,hstr;
    for (int i = 0; i < len; ) {
        hstr = str[i].toLatin1();
        if (hstr == ' ') {
            ++i;
            continue;
        }
        ++i;
        if (i  >= len) break;
        lstr = str[i].toLatin1();
        hexdata = ConvertHexChar(hstr);
        lowhexdata = ConvertHexChar(lstr);
        if ((hexdata == 16) || (lowhexdata == 16))
            break;
        else
            hexdata = hexdata*16 + lowhexdata;
        ++i;
        senddata[hexdatalen] = (char)hexdata;
        ++hexdatalen;
    }
    senddata.resize(hexdatalen);
    return senddata;
}


char MainWindow::ConvertHexChar(char ch)
{
    if ((ch >= '0') && (ch <= '9'))
        return ch - 0x30;
    else if ((ch >= 'A') && (ch <= 'F'))
        return ch - 'A' + 10;
    else if ((ch >= 'a') && (ch <= 'f'))
        return ch - 'a' + 10;
    else return ch -  ch;
}


uint16_t MainWindow::crc_16(uint8_t *data)//crc检验
{
    uint16_t crc16 = 0x0000;
    uint16_t len=10;
    while( len-- ) {
        for(uint8_t i=0x80; i!=0; i>>=1) {
            if((crc16 & 0x8000) != 0) {
                crc16 = crc16 << 1;
                crc16 = crc16 ^ 0x1021;
            }
            else {
                crc16 = crc16 << 1;
            }
            if((*data & i) != 0) {
                crc16 = crc16 ^ 0x1021;  //crc16 = crc16 ^ (0x10000 ^ 0x11021)
            }
        }
        data++;
    }
    return crc16;
}


QByteArray MainWindow::switchHL(QString str)//高低位交换
{
    QList<QString> list;
    for(int i=4;i<=23;i+=4)
    {
        list<<str.mid(i,4);
    }
    QList<QByteArray>list2;
    for(int i=0;i<5;i++)
    {
        QByteArray tmp=StringToHex(list[i]);
        list2.append(tmp);
    }
    for(int i=0;i<5;i++)
    {
        char tmp=list2[i][0];
        list2[i][0]=list2[i][1];
        list2[i][1]=tmp;
    }
    QByteArray res=StringToHex(str.left(4));
    for(int i=0;i<5;i++)
    {
        res.append(list2[i]);
    }
    return res;
}

QByteArray MainWindow::setInterval()
{
    QString tmp;
    QByteArray byte;
    QString crcCode;
    uint8_t data[10];
    tmp="11032005010000320000";
    byte=StringToHex(tmp);
    for(int i=0;i<10;i++)
    {
        data[i]=byte[i];
    }
    crcCode=QString("%1").arg(crc_16(data),4,16,QLatin1Char('0'));
    tmp.append(crcCode);
    return switchHL(tmp);
}

void MainWindow::on_stopButton_clicked()
{
//    ui->closeButton->setEnabled(true);
    ui->openButton->setEnabled(true);
//    ui->closeButton_2->setEnabled(true);
    ui->routineButton->setEnabled(true);
//    my_serialPort->write(StringToHex("11 03 40 60 00 01 0B 00 00 00 92 33"));//快停
//    Delay_MSec(55);
//    my_serialPort->write(StringToHex("11 03 40 60 00 01 0F 01 00 00 E6 99"));//位置模式停止
    ui->statusLine->setText("已停止");
    isStop=true;
}



void MainWindow::on_openButton_clicked()
{
    QString speed,acc,nacc,pos;
    pos=QString::number(lengthToPmw(ui->positionLine->text()));//这里调用转换函数
    qDebug()<<pos.toLong()<<endl;
    if(pos.toLong()<0||pos.toLong()>60000)
    {
        QMessageBox::about(this,tr("Warning!"),tr("机翼伸出长度为1~128mm"));
        return;
    }
    my_serialPort->setBaudRate(  ui->baudBox->currentText().toInt() );
//    speed=ui->speedLine->text();
    speed=getSpeed();
    acc=getAcc();
    nacc=getAcc();
//    acc=ui->paccLine->text();
//    nacc=ui->naccLine->text();
    my_serialPort->write(setPos(QString::number(zeroPoint+pos.toLong())));
    Delay_MSec(55);
    my_serialPort->write(setAcc(acc));
    Delay_MSec(55);
    my_serialPort->write(setNAcc(nacc));
    Delay_MSec(55);
    my_serialPort->write(setSpeed(speed));
    Delay_MSec(55);
    my_serialPort->write(StringToHex("1103406000013F000000F72A"));
}

//void MainWindow::on_closeButton_2_clicked()
//{
//    QString speed,acc,nacc;
//    speed=ui->speedLine->text();
////    acc=ui->paccLine->text();
////    nacc=ui->naccLine->text();
//    my_serialPort->write(setPos(QString::number(zeroPoint)));
//    Delay_MSec(55);
//    my_serialPort->write(setAcc(acc));
//    Delay_MSec(55);
//    my_serialPort->write(setNAcc(nacc));
//    Delay_MSec(55);
//    my_serialPort->write(setSpeed(speed));
//    Delay_MSec(55);
//    my_serialPort->write(StringToHex("1103406000013F000000F72A"));
//}

void MainWindow::on_routineButton_clicked()
{
    QString speed,acc,nacc,interval,routine;
    routine=QString::number(0);
    ui->openButton->setEnabled(false);
    ui->routineButton->setEnabled(false);
    isStop=false;
    if(ui->routineLine->text().toLong()<0)
    {
        QMessageBox::about(this,tr("Warning!"),tr("循环次数不得低于0次"));
        return;
    }
    else
        routine=ui->routineLine->text();
    speed=getSpeed();
    acc=getAcc();
    nacc=acc;
    interval=getInterval();
    my_serialPort->write(setPos(QString::number(zeroPoint+100)));
    Delay_MSec(55);
    my_serialPort->write(setAcc("200"));
    Delay_MSec(55);
    my_serialPort->write(setNAcc("200"));
    Delay_MSec(55);
    my_serialPort->write(setSpeed("350"));
    Delay_MSec(55);
    my_serialPort->write(StringToHex("1103406000013F000000F72A"));
    ui->statusLine->setText("正在回零");
    Delay_MSec(4000);
    int tmp=0;
    while(isStop==false&&tmp<routine.toLong())
    {
        tmp++;
        ui->statusLine->setText("循环次数"+QString::number(tmp));
        my_serialPort->write(setPos(QString::number(zeroPoint+60000)));
        Delay_MSec(55);
        my_serialPort->write(setAcc(acc));
        Delay_MSec(55);
        my_serialPort->write(setNAcc(nacc));
        Delay_MSec(55);
        my_serialPort->write(setSpeed(speed));
        Delay_MSec(55);
        my_serialPort->write(StringToHex("1103406000013F000000F72A"));
        Delay_MSec(interval.toInt()*1000);
        if(isStop==false)
        {
            ui->statusLine->setText("循环次数"+QString::number(tmp));
            my_serialPort->write(setPos(QString::number(zeroPoint+200)));
            Delay_MSec(55);
            my_serialPort->write(setPos(QString::number(zeroPoint+200)));
            Delay_MSec(55);
            my_serialPort->write(StringToHex("1103406000013F000000F72A"));
            Delay_MSec(55);
            my_serialPort->write(StringToHex("1103406000013F000000F72A"));
            Delay_MSec(interval.toInt()*1000);
        }
    }
    ui->statusLine->setText("已停止");
    ui->routineButton->setEnabled(true);
    ui->openButton->setEnabled(true);
}

bool MainWindow::isOnZero()
{
    time_t begin,now;
    begin=clock();
    now=clock();
    double last=(double)(now-begin)/CLOCKS_PER_SEC;
    while(last<=3)
    {
        QString posRead("10 01 71 20 01 01 B3 4B 4F");
        QByteArray sendData=StringToHex(posRead);
        my_serialPort->write(sendData);
        requestData = my_serialPort->readAll();
        if(requestData.length()==14)
        {
            QByteArray tmp;
            tmp.append(requestData[11]);
            tmp.append(requestData[10]);
            tmp.append(requestData[9]);
            tmp.append(requestData[8]);
            requestData.clear();
            int num=byteAraryToInt(tmp);
            if(num==38913)
            {
                return true;
            }
        }
        requestData.clear();
    }
    return false;
}

int MainWindow::lengthToPmw(QString length)
{
    //找到安装好机翼后找到函数
    //单词运动时候调用该函数
    //找到0-60000所对应的极限 进行约束
    long x=length.toLong();
    int result=0.000003*pow(x,5)-0.0008*pow(x,4)+0.0978*pow(x,3)-3.4185*pow(x,2)+168.7*x-2.153;
    return result;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(isOpen)
    {
        emit on_stopButton_clicked();
        Delay_MSec(55);
        my_serialPort->write(StringToHex("11 03 60 60 00 01 01 00 00 00 A5 9A"));//位置模式
        Delay_MSec(55);
        if(isFlag==true)
            my_serialPort->write(setPos(QString::number(zeroPoint+100)));
        else
            my_serialPort->write(setPos("-100000"));
        Delay_MSec(55);
        my_serialPort->write(setAcc("100"));
        Delay_MSec(55);
        my_serialPort->write(setNAcc("100"));
        Delay_MSec(55);
        my_serialPort->write(setSpeed("70"));
        Delay_MSec(55);
        my_serialPort->write(StringToHex("1103406000013F000000F72A"));
        Delay_MSec(6000);
        if(isFlag==true)
            my_serialPort->write(StringToHex("11 03 40 60 00 01 06 00 00 00 C3 71"));
        else
                my_serialPort->write(StringToHex("1103406000017F0000005A37"));//相对
        Delay_MSec(60);
        my_serialPort->close();
    }
}

QString MainWindow::getTime()
{
    double speed,acc,position,time1,time2;
    speed=getSpeed().toInt();
    acc=getAcc().toInt();
    if(ui->positionLine->text().toInt()==0)
        position=60000/10000;
    else
        position=ui->positionLine->text().toInt();
    time1=speed/acc;
    if(speed*time1/60>=position)
    {
        qDebug()<<"三角形"<<time1*2<<endl;
        time1=sqrt(60*position/acc);
        return QString::number(sqrt(60*position/acc)*2,'g',2);
    }
    else
    {
        time2=(position-speed*time1/60)/(speed/60);
        return QString::number(time1*2+time2,'g',2);
    }
}

QString MainWindow::getSpeed()
{
    QString speed;
    switch(ui->speedBox->currentText().toInt())
    {
    case 1:speed=QString::number(300);break;
    case 2:speed=QString::number(700);break;
    case 3:speed=QString::number(1000);break;
    }
    return speed;
}

QString MainWindow::getAcc()
{
    QString acc;
    switch(ui->accBox->currentText().toInt())
    {
    case 1:acc=QString::number(1200);break;
    case 2:acc=QString::number(2000);break;
    case 3:acc=QString::number(2500);break;
    }
    return acc;
}

QString MainWindow::getInterval()
{
    QString interval;
    switch(ui->intervalBox->currentText().toInt())
    {
    case 3:interval='3';break;
    case 4:interval='4';break;
    case 5:interval='5';break;
    }
    return interval;
}

void MainWindow::refreshTime()
{
    ui->timeLine->setText(getTime()+'s');
}

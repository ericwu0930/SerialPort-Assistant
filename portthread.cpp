#include "portthread.h"

portthread::portthread()
{
}

void portthread::run()
{
    t_2 = new QTimer(this);
    connect( t_2, SIGNAL( timeout() ), this, SLOT( readPosition() ) );
    timer->start(100);
    QObject::connect(my_serialPort, &QSerialPort::readyRead, this, &MainWindow::Read_Data);
}

